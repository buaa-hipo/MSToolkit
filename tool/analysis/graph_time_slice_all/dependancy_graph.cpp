#include "dependancy_graph.h"
#include "../timeline/alignment.h"
#include <unordered_map>
#include <algorithm> 
#include <list>
#include <queue>
#include <string>
#include <fstream>
#include <sstream>
#include <set>
#include <utility>
#include "json_writer.h"
#include "utils/tsc_timer.h"
#include "utils/demangle.h"

#include <omp.h> 

using namespace std;

namespace DependencyType {
    const char* to_string(uint32_t type) {
      switch(type) {
        case NONE: return "NONE";
        case COMM: return "COMM";
        case SYNC: return "SYNC";
        case COLLECTIVE: return "COLLECTIVE";
        case HIP: return "HIP";
      }
      return "UNKNOWN";
    }
}

typedef struct {
    unordered_map<std::string/*<dst>/<tag>*/,queue<RecordNode*> > send_list;
    unordered_map<std::string/*<src>/<tag>*/,queue<RecordNode*> > recv_list;
    unordered_map<uint64_t/*request*/,RecordNode*> wait_list;
} LastTrack_t;

std::string encode_comm_info(int dest, int tag) {
    return std::to_string(dest) + std::string("/") + std::to_string(tag);
}


void free_local_track(std::unordered_map<int,LastTrack_t*>& local_track) {
    for (auto& entry : local_track) {
        LastTrack_t* track = entry.second;
        if (!track) continue;
        delete track;
    }
    local_track.clear();
}

std::string extract_kernel_name(const std::string& input) {
    //printf("origin %s\n", input.c_str());
    std::istringstream stream(input);
    std::string firstLine;
    std::getline(stream, firstLine);

    std::string prefix = "Asynchronous workload name: ";
    if (firstLine.substr(0, prefix.size()) == prefix) {
        firstLine = firstLine.substr(prefix.size());
    }

    std::string finalString = demangle(firstLine);
    size_t pos = 0;
    while ((pos = finalString.find(',')) != std::string::npos) {
        finalString.replace(pos, 1, "@");
    }
    //printf("after %s\n", finalString.c_str());
    return finalString;
}

void CreateDependancyGraphSliceAll(RecordReader* reader,bool need_serial_link, uint64_t interval, uint64_t duration, const char* filename,
                                                   uint64_t start , uint64_t end, bool bt_only, const char* input_file, bool if_bt){
    RecordTraceCollection& collection = reader->get_all_traces();
    RankMetaCollection& metas = reader->get_all_meta_maps();
    ExtRecordTraceCollection& rte_collection = reader->get_all_etraces();
    RankRecordTraceCollection& rank_record_collection = reader->get_main_thread_traces();
    RankExtRecordTraceCollection& rank_ext_record_collection = reader->get_all_etraces_by_rank();
    
    RankIdExtRecordTraceCollection& rank_id_ext_record_collection = reader->get_all_id_etraces_by_rank();

    BacktraceCollection* backtraces = reader->has_backtrace() ? &reader->get_all_backtraces() : nullptr;
    StringSectionCollection& string_sections = reader->get_all_string_sections();
    std::vector<std::string> pmu_event_list;
    std::set<std::pair<int, int>> filter_set;
    
    // DependancyGraph* graph = new DependancyGraph();

    FILE* node_fp;
    FILE* edge_fp;
    FILE* bt_fp;

    if(bt_only) {
        printf("DUMPIPNG BACKTRACE....\n");
        char bt_filename[256];
        snprintf(bt_filename, sizeof(bt_filename), "%s_bt", filename);
        bt_fp = fopen(bt_filename, "w");
        if (!bt_fp) {
            perror("Failed to open file");
            return ;
        }
        fprintf(bt_fp, "ts_id,tid,backtrace\n");

        std::ifstream infile(input_file);
        if (!infile.is_open()) {
            perror("Failed to open input file");
            return ;
        }
        std::string line;
        while (std::getline(infile, line)) {
            std::istringstream iss(line);
            int ts_id, tid;
            if (iss >> ts_id >> tid) {
                //printf("%d %d\n", ts_id, tid);
                filter_set.emplace(ts_id, tid);
            }
        }
        infile.close(); 
    }
    else {
        node_fp = fopen(filename, "w");
        if (!node_fp) {
            perror("Failed to open file");
            return ;
        }
        fprintf(node_fp, "ts_id,tid,name,ts,dur,args_Data_type,args_Dest,args_Tag,args_Count,args_Send_Count,args_Recv_Count,args_Comm,args_Request,args_Op,args_Root,arg_pmu_0,arg_pmu_1,arg_pmu_2,arg_pmu_3\n");

        char edge_filename[128];
        snprintf(edge_filename, sizeof(edge_filename), "%s_edge", filename);
        edge_fp = fopen(edge_filename, "w");
        if (!edge_fp) {
            perror("Failed to open file");
            return ;
        }
        fprintf(edge_fp, "ts_id,src,dst,freq,commsize,type\n");

        if(if_bt) {
            char bt_filename[256];
            snprintf(bt_filename, sizeof(bt_filename), "%s_bt", filename);
            bt_fp = fopen(bt_filename, "w");
            if (!bt_fp) {
                perror("Failed to open file");
                return ;
            }
            fprintf(bt_fp, "ts_id,tid,backtrace\n");
        }
    }

    uint64_t start_time = UINT64_MAX;
    uint64_t exit_time = 0;
    printf("origin: %llu, %llu\n", start_time, exit_time);
    std::vector<int> ranks;
    std::vector<std::string> ids;
    for (const auto& entry : collection) {
        int rank = entry.second->rank();
        ranks.push_back(rank);
        ids.push_back(entry.first);
        printf("rank %d str_id %s\n",rank,entry.first.c_str());
    }

    printf("rank size is %d \n",ranks.size());
    if(start == 0 && end == 0){
        #pragma omp parallel for reduction(min:start_time) reduction(max:exit_time)
            for (size_t i = 0; i < ranks.size(); ++i) {
                int rank = ranks[i];
                RecordTrace* rtrace = rank_record_collection[rank];
                uint64_t offset = rtrace->offset;
                RecordTraceIterator rti = rtrace->find(event_MPI_Init, true /*ignore zoom*/);
                if (RecordTraceIterator::is_invalid(rti)) {
                    JSI_ERROR("Error: Could not find PROCESS_START event in Rank %d\n", rank);
                }
                uint64_t local_start_time = rti.val_cache()->timestamps.enter - offset;
                #pragma omp critical
                {
                    start_time = min(start_time, local_start_time);
                }
                rti = rtrace->find(event_MPI_Finalize, true);
                if (RecordTraceIterator::is_invalid(rti)) {
                    JSI_ERROR("Error: Could not find PROCESS_EXIT event in Rank %d\n", rank);
                }
                uint64_t local_exit_time = rti.val_cache()->timestamps.exit - offset;
                #pragma omp critical
                {
                    exit_time = max(exit_time, local_exit_time);
                }
                //delete rti;
            }
    }
    else{
        start_time = start; 
        exit_time = end;
    }

    uint64_t middle = exit_time - start_time;
    printf("start: %llu, end: %llu\n", start_time, exit_time);
    omp_set_nested(0);  
    uint64_t ts = static_cast<uint64_t>(std::ceil(static_cast<double>(exit_time - start_time) / interval));
    std::cout << "Total timeslice count: " << ts << std::endl;

    /*get dev_type*/
    bool has_accl = false;
    std::unordered_map<std::string, MetaDataMap *> tmp_metaMap;
    MetaDataMap::MetaValue_t* t;
    for (auto meta_it = metas.begin(); meta_it != metas.end(); ++meta_it) {
        tmp_metaMap = meta_it->second->getMetaMap();
        if (tmp_metaMap.contains("ACCL TRACE META") && tmp_metaMap.contains("HOST INFO")) {
            has_accl = true;
            break;
        }
    }
    if (!has_accl) {
        std::cout << "No accl trace!" << std::endl;
    }
    auto accl_trace_meta_map = tmp_metaMap.at("ACCL TRACE META");
    accl_trace_meta_map->get("ACCL_DEVICE_TYPE", &t);
    const char* dev_type = t->ptr;
    printf("dev_type is %s\n",dev_type);
    std::unordered_map<int, int> rank2pid;
    for (auto it = metas.begin(); it != metas.end(); ++it) {
        auto metaMap = it->second->getMetaMap();
        if (!metaMap.contains("HOST INFO")) continue;
        metaMap.at("HOST INFO")->get("PID", &t);
        int pid = t->i32;
        rank2pid[it->first] = pid;
    }
    //return;
    
// #pragma omp parallel for
//     for (size_t i = 0; i < ranks.size(); ++i) {
//         int rank = ranks[i];
//         RecordTrace* rtrace = rank_record_collection[rank];
//         rtrace->initialize_ts_local_views(ts);
//     }

#pragma omp parallel
{
    DependancyGraph* local_graph = new DependancyGraph();
    #pragma omp for nowait
    for(uint64_t ts = start_time ; ts < exit_time; ts += interval) {
        unordered_map<int, LastTrack_t*> local_track;  
        for (size_t i = 0; i < ranks.size(); ++i) {
            int rank = ranks[i];
            std::string id = ids[i];
            uint64_t ts_id = (ts - start_time) / interval;
            // if (bt_only && filter_set.count({ts_id, rank}) < 1 && filter_set.count({ts_id, -rank-1}) < 1) { 
            //     //printf("Filter {ts_id:%d rank:%d or rank:%d} \n", ts_id, rank, -rank-1);
            //     continue;
            // }
            RecordTrace* rtrace = rank_record_collection[rank];
            auto num_pmu_events = rtrace->num_pmu_events();

            LastTrack_t* ltrack = new LastTrack_t;
            RecordNode* last = NULL;
            uint64_t offset = rtrace->offset;
            size_t event_id = 0;
            std::pair<RecordTraceIterator,RecordTraceIterator> zoom_pair =  rtrace->zoom_ts(ts_id, ts, ts + duration, offset);
           // printf("loop zoom begin\n"); 
            for(auto ri=zoom_pair.first, re=zoom_pair.second; ri!=re; ri=ri.next()) {
                record_t* r = ri.val_cache();
                //printf("begin ts_id %lu ts %lu rank %d str_id %s r->ctxt %d msgtype %lu begin %lu end %lu r* %p\n", ts_id, ts, rank,id.c_str
                // (),(backtrace_context_t)r->ctxt,r->MsgType,r->timestamps.enter,r->timestamps.exit, r);
                if(!RecordHelper::is_valid_ctxt(r)) {
                    continue;
                }
                if(!r) {
                    printf("ERROR r is NULL\n");
                    continue;
                }
                if(r->timestamps.exit - offset >= ts + duration) {
                    continue;
                }
                RecordNode* node;
                if (num_pmu_events > 0) {
                    std::vector<std::pair<uint64_t, uint64_t>> pmu_values;
                    for (int i = 0; i < num_pmu_events; i++) {
                        pmu_values.emplace_back(RecordHelper::counter_val_enter(r, i, num_pmu_events),
                                                RecordHelper::counter_val_exit(r, i, num_pmu_events));
                    }
                    node = new RecordNode(rank, ts_id, true, r, offset, pmu_values, id);
                } else {
                    node = new RecordNode(rank, ts_id, true, r, offset, id);
                }
                node->event_id = event_id++;
                local_graph->add_node(rank, node);
                if(need_serial_link && last!=NULL) {
                    local_graph->add_edge(new DependancyEdge(last, node, DependencyType::NONE));
                }
                if(RecordHelper::is_event(r, event_MPI_Send)) {
                    record_comm_t* rec = reinterpret_cast<record_comm_t*>(r);
                    std::string enc = encode_comm_info(rec->dest, rec->tag);
                    ltrack->send_list[enc].push(node);
                    continue;
                }
                if(RecordHelper::is_event(r, event_MPI_Recv)) {
                    record_comm_t* rec = reinterpret_cast<record_comm_t*>(r);
                    std::string enc = encode_comm_info(rec->dest, rec->tag);
                    ltrack->recv_list[enc].push(node);
                    continue;
                }
                if(RecordHelper::is_event(r, event_MPI_Isend)) {
                    record_comm_async_t* rec = reinterpret_cast<record_comm_async_t*>(r);
                    std::string enc = encode_comm_info(rec->dest, rec->tag);
                    ltrack->send_list[enc].push(node);
                    ltrack->wait_list[rec->request] = node;
                    continue;
                }
                if(RecordHelper::is_event(r, event_MPI_Irecv)) {
                    record_comm_async_t* rec = reinterpret_cast<record_comm_async_t*>(r);
                    std::string enc = encode_comm_info(rec->dest, rec->tag);
                    ltrack->recv_list[enc].push(node);
                    ltrack->wait_list[rec->request] = node;
                    continue;
                }
                if(RecordHelper::is_event(r, event_MPI_Wait)) {
                    record_comm_wait_t* rec = reinterpret_cast<record_comm_wait_t*>(r);
                    auto it = ltrack->wait_list.find(rec->request);
                    if(it!=ltrack->wait_list.end() && it->second!=NULL) {
                        RecordNode* comm_node = it->second;
                        local_graph->add_edge(new DependancyEdge(comm_node, node, DependencyType::SYNC));
                        // delete ltrack->wait_list[rec->request];
                        ltrack->wait_list[rec->request] = NULL;
                    }
                    continue;
                }
                /* add host-device edge*/
                RecordNode* r_node;
                record_activity_launch_t* ral = (record_activity_launch_t*)r;        
                uint64_t cid = ral->correlation_id;
                // std::vector<ExtRecordTrace *> erts = rank_ext_record_collection[rank];
                // ExtRecordTrace* ert = erts[0];
                ExtRecordTrace* ert = rank_id_ext_record_collection[rank][id];
                int cnt = 0;
                for(auto& it: ert->getIteratorList()) {
                    for(auto outer_it = it.begin_iter; outer_it!=it.end_iter; ++outer_it) {
                        auto rec = outer_it.get();
                        AcclRecordTrace accl_record_trace(rec, 0, outer_it.record_size(), "", dev_type, string_sections[id]);
                        for(auto ait = accl_record_trace.begin(); ait!=accl_record_trace.end(); ++ait) {
                            auto r_cid = ait.get_cid();
                            if(r_cid == cid) {
                                char *buffer = new char[1024]();
                                std::string kernel_name = extract_kernel_name(ait.get_name());
                                std::copy(kernel_name.begin(), kernel_name.end(), buffer);
                                buffer[kernel_name.size()] = '\0';
                                r_node =  new RecordNode(- rank - 1, ts_id, true, (record_t*)(r), offset, buffer, id);   
                                r_node->event_id = event_id++;
                                local_graph->add_node(- rank - 1, r_node);
                                local_graph->add_edge(new DependancyEdge(node, r_node,  DependencyType::HIP));
                                local_graph->add_edge(new DependancyEdge(r_node, node,  DependencyType::HIP));
                                break;                    
                            }
                        }
                    }
                }
                // printf("end rank %d str_id %s r->ctxt %d r* %p\n",rank,id.c_str
                // (),(backtrace_context_t)r->ctxt, r);
            }
            if(ltrack != nullptr) {
                local_track[rank] = ltrack;
            }
        }

        for(auto it=local_track.begin(), ie=local_track.end(); it!=ie; ++it) {
            /* generate links from send events */
            int rank = it->first;
            LastTrack_t* ltrack = it->second;
            for(auto qi=ltrack->send_list.begin(), qe=ltrack->send_list.end(); qi!=qe; ++qi) {
                queue<RecordNode*>& sq = qi->second;
                while(!sq.empty()) {
                    RecordNode* sn = sq.front();
                    record_comm_t* rec = reinterpret_cast<record_comm_t*>(sn->data);
                    int target = rec->dest;
                    std::string enc = encode_comm_info(rank, rec->tag);
                    auto ttrack = local_track.find(target);
                    if(ttrack!=local_track.end()) {
                        auto trq = ttrack->second->recv_list.find(enc);
                        if(trq!=ttrack->second->recv_list.end()) {
                            queue<RecordNode*>& rq = trq->second;
                            if (!rq.empty()) {
                                RecordNode* rn = rq.front();
                                local_graph->add_edge(new DependancyEdge(sn, rn, DependencyType::COMM));
                                rq.pop();
                            }
                        }
                        else {
                            sn->enclosed = false;
                        }
                    }
                    else {
                        sn->enclosed = false;
                    }
                    sq.pop();
                }
            }
        }
        free_local_track(local_track);
    }
    #pragma omp critical
    {
        if(!bt_only){
            printf("Thread %d merge nodes&edges&backtrace local graph! node size is %d ; edge size is %d \n",omp_get_thread_num(), local_graph->get_nodes_size(),local_graph->get_edges_size());
            local_graph->dump_nodes(node_fp, pmu_event_list, reader);
            local_graph->dump_edges(edge_fp, reader);
            if(if_bt) local_graph->dump_backtrace_without_filter(bt_fp, backtraces, reader);
        }
        else {
            printf("Thread %d merge bt local graph! node size is %d ; edge size is %d \n",omp_get_thread_num(), local_graph->get_nodes_size(), local_graph->get_edges_size());
            local_graph->dump_backtrace(bt_fp, backtraces, filter_set, reader);
        }
        delete local_graph;
    }
}
    
    if(bt_only) {
        fclose(bt_fp);
    }
    else {
        fclose(node_fp);
        fclose(edge_fp);
        if(if_bt)fclose(bt_fp);
    }
    /* Return analyzed dependancy graph */
    // return graph;   
}


void DependancyGraph::dump_backtrace_without_filter(FILE* fp ,BacktraceCollection* backtrace, RecordReader* reader) {
    RecordTraceCollection& traces = reader->get_all_traces();
    RankMetaCollection& metas = reader->get_all_meta_maps();
    ExtRecordTraceCollection& rte_collection = reader->get_all_etraces();
    RankRecordTraceCollection& rank_record_collection = reader->get_main_thread_traces();
    RankExtRecordTraceCollection& rank_ext_record_collection = reader->get_all_etraces_by_rank();
    if (backtrace == nullptr) {
        JSI_ERROR("Backtrace is nullptr!");
        return;
    }
    int n = _nodes.size();
    for (int i = 0; i < n; i++) {
        int ts_id = _nodes[i]->ts_id;
        int rank = _nodes[i]->rank;
        record_t* r = _nodes[i]->data;
        std::string id = _nodes[i]->id;
        BacktraceTree* bt_tree = (*backtrace)[id];
        if (bt_tree != nullptr && (RecordHelper::is_mpi(r) || RecordHelper::is_function(r))) {
            //printf("in dumpbt:: rank %d str_id %s r->ctxt %d\n",rank,id.c_str(),(backtrace_context_t)r->ctxt);
            const char* s = bt_tree->backtrace_get_context_string((backtrace_context_t)r->ctxt);
            if(rank < 0) {
                char* name = _nodes[i]->name;
                std::string new_str = std::string(name) + "\n" + s;
                fprintf(fp, "%d,%d,\"%s\"\n", ts_id, rank, new_str.c_str());
            }
            else {
                fprintf(fp, "%d,%d,\"%s\"\n", ts_id, rank, s);
            }
        }
    }
    fflush(fp);
}

void DependancyGraph::dump_backtrace(FILE* fp, BacktraceCollection* backtrace,std::set<std::pair<int, int>> filter_set , RecordReader* reader) {
    RecordTraceCollection& traces = reader->get_all_traces();
    RankMetaCollection& metas = reader->get_all_meta_maps();
    ExtRecordTraceCollection& rte_collection = reader->get_all_etraces();
    RankRecordTraceCollection& rank_record_collection = reader->get_main_thread_traces();
    RankExtRecordTraceCollection& rank_ext_record_collection = reader->get_all_etraces_by_rank();
    if (backtrace == nullptr) {
        JSI_ERROR("Backtrace is nullptr!");
        return;
    }
    int n = _nodes.size();
    for (int i = 0; i < n; i++) {
        int ts_id = _nodes[i]->ts_id;
        int rank = _nodes[i]->rank;
        record_t* r = _nodes[i]->data;
        std::string id = _nodes[i]->id;
        BacktraceTree* bt_tree = (*backtrace)[id];
        if (filter_set.count({ts_id, rank}) > 1 && bt_tree != nullptr && (RecordHelper::is_mpi(r) || RecordHelper::is_function(r))) {
            // if(r->ctxt>812){ // misamd4000
            //   continue;
            // }
            // if(r->ctxt>1619){ // lmp16
            //   continue;
            // }
            // if(r->ctxt>1039){ // misamd1000 
            //     continue
            // }
            const char* s = bt_tree->backtrace_get_context_string((backtrace_context_t)r->ctxt);
            if(rank < 0) {
                char* name = _nodes[i]->name;
                std::string new_str = std::string(name) + "\n" + s;
                fprintf(fp, "%d,%d,\"%s\"\n", ts_id, rank, new_str.c_str());
            }
            else {
                fprintf(fp, "%d,%d,\"%s\"\n", ts_id, rank, s);
            }
        }
    }
    fflush(fp);
}

struct hash_pair {
    template <class T1, class T2>
    size_t operator()(const pair<T1, T2>& p) const {
        auto hash1 = hash<T1>{}(p.first);
        auto hash2 = hash<T2>{}(p.second);
        return hash1 ^ hash2;
    }
};


void DependancyGraph::dump_nodes(FILE* fp, const std::vector<std::string>& event_list, RecordReader* reader) {
    RecordTraceCollection& traces = reader->get_all_traces();
    RankMetaCollection& metas = reader->get_all_meta_maps();
    ExtRecordTraceCollection& rte_collection = reader->get_all_etraces();
    RankRecordTraceCollection& rank_record_collection = reader->get_main_thread_traces();
    RankExtRecordTraceCollection& rank_ext_record_collection = reader->get_all_etraces_by_rank();
    auto mhz = get_tsc_freq_mhz();
    unordered_map<pair<int, int>, vector<string>, hash_pair> merged_data;
    int n = _nodes.size();
    for (int i = 0; i < n; i++) {
        string name = "0";
        string ts = "0";
        string dur = "0";
        string datatype = "0";
        string dest = "0";
        string tag = "0";
        string count = "0";
        string sendcnt = "0";
        string recvcnt = "0";
        string comm = "0";
        string request = "0";
        string op = "0";
        string root = "0";
        string pmu0 = "0", pmu1 = "0", pmu2 = "0", pmu3 = "0";

        int ts_id = _nodes[i]->ts_id;
        int rank = _nodes[i]->rank;
        uint64_t off = _nodes[i]->offset;
        if(rank >= 0) {
            record_t* r = _nodes[i]->data;
            name = RecordHelper::get_record_name(r);
            double start_time = tsc_duration_us(r->timestamps.enter - off, mhz);
            double duration = tsc_duration_us(r->timestamps.exit - off, mhz) - start_time;
            ts = std::to_string(start_time);
            dur = std::to_string(duration);

            switch(r->MsgType) {
                case event_MPI_Send:
                case event_MPI_Recv: {
                    record_comm_t *rd = (record_comm_t *) r;
                    datatype = std::to_string(rd->datatype);
                    dest = std::to_string(rd->dest);
                    tag = std::to_string(rd->tag);
                    count = std::to_string(rd->count);
                    comm = std::to_string(rd->comm);
                    break;
                }
                case event_MPI_Isend:
                case event_MPI_Irecv: {
                    record_comm_async_t *rd = (record_comm_async_t *) r;
                    datatype = std::to_string(rd->datatype);
                    dest = std::to_string(rd->dest);
                    tag = std::to_string(rd->tag);
                    count = std::to_string(rd->count);
                    comm = std::to_string(rd->comm);
                    request = std::to_string(rd->request);
                    break;
                }
                case event_MPI_Wait: {
                    record_comm_wait_t *rd = (record_comm_wait_t *) r;
                    request = std::to_string(rd->request);
                    break;
                }
                case event_MPI_Alltoall:
                case event_MPI_Alltoallv: {
                    record_all2all_t *rd = (record_all2all_t *) r;
                    datatype = std::to_string(rd->datatype);
                    sendcnt = std::to_string(rd->sendcnt);
                    recvcnt = std::to_string(rd->recvcnt);
                    comm = std::to_string(rd->comm);
                    break;
                }
                case event_MPI_Allreduce: {
                    record_allreduce_t *rd = (record_allreduce_t *) r;
                    datatype = std::to_string(rd->datatype);
                    count = std::to_string(rd->count);
                    op = std::to_string(rd->op);
                    comm = std::to_string(rd->comm);
                    break;
                }
                case event_MPI_Reduce: {
                    record_reduce_t *rd = (record_reduce_t *) r;
                    datatype = std::to_string(rd->datatype);
                    count = std::to_string(rd->count);
                    root = std::to_string(rd->root);
                    op = std::to_string(rd->op);
                    comm = std::to_string(rd->comm);
                    break;
                }
                case event_MPI_Bcast: {
                    record_bcast_t *rd = (record_bcast_t *) r;
                    datatype = std::to_string(rd->datatype);
                    count = std::to_string(rd->count);
                    root = std::to_string(rd->root);
                    comm = std::to_string(rd->comm);
                    break;
                }
            }
        } else {
            record_t* r = _nodes[i]->data;
            name = _nodes[i]->name; 
            double start_time = tsc_duration_us(r->timestamps.enter - off, mhz);
            double duration = tsc_duration_us(r->timestamps.exit - off, mhz) - start_time;
            ts = std::to_string(start_time);
            dur = std::to_string(duration);
        }
      
        auto key = make_pair(ts_id, rank);        
        if (merged_data.find(key) == merged_data.end()) {
            merged_data[key] = {
                std::to_string(ts_id),
                std::to_string(rank),
                name, 
                ts, 
                dur, 
                datatype, 
                dest, 
                tag, 
                count, 
                sendcnt, 
                recvcnt, 
                comm,
                request, 
                op, 
                root, 
                pmu0, pmu1, pmu2, pmu3};
        } else {
            auto& merged_row = merged_data[key];
            merged_row[2] += ";" + name; // 合并name
            merged_row[3] += ";" + ts;
            merged_row[4] += ";" + dur;
            merged_row[5] += ";" + datatype;
            merged_row[6] += ";" + dest;
            merged_row[7] += ";" + tag;
            merged_row[8] += ";" + count;
            merged_row[9] += ";" + sendcnt;
            merged_row[10] += ";" + recvcnt;
            merged_row[11] += ";" + comm; 
            merged_row[12] += ";" + request; // 合并name
            merged_row[13] += ";" + op;
            merged_row[14] += ";" + root;
            merged_row[15] += ";" + pmu0;
            merged_row[16] += ";" + pmu1;
            merged_row[17] += ";" + pmu2;
            merged_row[18] += ";" + pmu3;
        }
    }
    vector<pair<pair<int, int>, vector<string>>> sorted_data(merged_data.begin(), merged_data.end());

    std::sort(sorted_data.begin(), sorted_data.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    for (const auto& entry : sorted_data) {
        const auto& row = entry.second;
        for (size_t i = 0; i < row.size(); i++) {
            fprintf(fp, "%s", row[i].c_str());
            if (i < row.size() - 1) {
                fprintf(fp, ",");
            }
        }
        fprintf(fp, "\n");
    }
    fflush(fp);
    //fclose(fp);
}

void DependancyGraph::dump_edges(FILE* fp, RecordReader* reader) {
    RecordTraceCollection& traces = reader->get_all_traces();
    RankMetaCollection& metas = reader->get_all_meta_maps();
    ExtRecordTraceCollection& rte_collection = reader->get_all_etraces();
    RankRecordTraceCollection& rank_record_collection = reader->get_main_thread_traces();
    RankExtRecordTraceCollection& rank_ext_record_collection = reader->get_all_etraces_by_rank();
 
    auto m = _edges.size();
  
    for (auto i = 0; i < m; ++i) {
        if (_edges[i]->type == DependencyType::COMM || _edges[i]->type == DependencyType::HIP) {
            // record_t* rs = _edges[i]->src->data;
            // record_comm_t* r = reinterpret_cast<record_comm_t*>(rs);
            fprintf(fp, "%d,%d,%d,%d,%d,%s\n",
                _edges[i]->src->ts_id,
                _edges[i]->src->rank,
                _edges[i]->dst->rank,
                _edges[i]->freq,
                reinterpret_cast<record_comm_t*>(_edges[i]->src->data)->count,
                DependencyType::to_string(_edges[i]->type));
        }
    }
    fflush(fp);
}

void extract_args_from_record(record_t* r, vector<ChromeTraceWriter::Arg>* args) {
    switch(r->MsgType) {
        case event_MPI_Send:
        case event_MPI_Recv: {
            record_comm_t *rd = (record_comm_t *) r;
            { ChromeTraceWriter::Arg a("Data type", rd->datatype); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Dest", rd->dest); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Tag", rd->tag); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Count", rd->count); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Comm", rd->comm); args->push_back(a); }
            break;
        }
        case event_MPI_Isend:
        case event_MPI_Irecv: {
            record_comm_async_t *rd = (record_comm_async_t *) r;
            { ChromeTraceWriter::Arg a("Data type", rd->datatype); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Dest", rd->dest); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Tag", rd->tag); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Count", rd->count); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Comm", rd->comm); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Request", rd->request); args->push_back(a); }
            break;
        }
        case event_MPI_Wait: {
            record_comm_wait_t *rd = (record_comm_wait_t *) r;
            { ChromeTraceWriter::Arg a("Request", rd->request); args->push_back(a); }
            break;
        }
        case event_MPI_Alltoall:
        case event_MPI_Alltoallv: {
            record_all2all_t *rd = (record_all2all_t *) r;
            { ChromeTraceWriter::Arg a("Data type", rd->datatype); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Send Count", rd->sendcnt); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Recv Count", rd->recvcnt); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Comm", rd->comm); args->push_back(a); }
            break;
        }
        case event_MPI_Allreduce: {
            record_allreduce_t *rd = (record_allreduce_t *) r;
            { ChromeTraceWriter::Arg a("Data type", rd->datatype); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Count", rd->count); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Op", rd->op); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Comm", rd->comm); args->push_back(a); }
            break;
        }
        case event_MPI_Reduce: {
            record_reduce_t *rd = (record_reduce_t *) r;
            { ChromeTraceWriter::Arg a("Data type", rd->datatype); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Count", rd->count); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Root", rd->root); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Op", rd->op); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Comm", rd->comm); args->push_back(a); }
            break;
        }
        case event_MPI_Bcast: {
            record_bcast_t *rd = (record_bcast_t *) r;
            { ChromeTraceWriter::Arg a("Data type", rd->datatype); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Count", rd->count); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Root", rd->root); args->push_back(a); }
            { ChromeTraceWriter::Arg a("Comm", rd->comm); args->push_back(a); }
            break;
        }
    }
}

void extract_backtrace_args_from_record(record_t* r, vector<ChromeTraceWriter::Arg>* args, BacktraceTree* bt_tree) {
    if (bt_tree != nullptr && RecordHelper::is_mpi(r) || RecordHelper::is_function(r)) {
        const char* s = bt_tree->backtrace_get_context_string((backtrace_context_t) r->ctxt);
        { ChromeTraceWriter::Arg a("Backtrace", s); args->push_back(a); }
    }
}

void extract_pmu_args_from_record(record_t* r, vector<ChromeTraceWriter::Arg>* args, const std::vector<std::string>& event_list) {
    auto num_events = event_list.size();
    for (int eidx = 0; eidx < num_events; eidx++) {
        auto& event_name = event_list[eidx];
        const auto enter_val = RecordHelper::counter_val_enter(r, eidx, num_events);
        const auto exit_val = RecordHelper::counter_val_exit(r, eidx, num_events);
        std::string name = "PMU " + std::to_string(eidx) + "(" + event_name + ")";
        { ChromeTraceWriter::Arg a(name + " enter", enter_val); args->push_back(a); }
        { ChromeTraceWriter::Arg a(name + " exit", exit_val); args->push_back(a); }
        { ChromeTraceWriter::Arg a(name + " count", exit_val - enter_val); args->push_back(a); }
    }
}

void DependancyGraph::merge_nodes(std::vector<RecordNode*> nodes) {
    _nodes.insert(_nodes.end(), nodes.begin(), nodes.end());
}

void DependancyGraph::merge_edges(std::vector<DependancyEdge*> edges) {
    _edges.insert(_edges.end(), edges.begin(), edges.end());
}


void DependancyGraph::dump_nodes_omp(FILE* fp, const std::vector<std::string>& event_list, RecordReader* reader) {
    RecordTraceCollection& traces = reader->get_all_traces();
    RankMetaCollection& metas = reader->get_all_meta_maps();
    ExtRecordTraceCollection& rte_collection = reader->get_all_etraces();
    RankRecordTraceCollection& rank_record_collection = reader->get_main_thread_traces();
    RankExtRecordTraceCollection& rank_ext_record_collection = reader->get_all_etraces_by_rank();

    fprintf(fp, "ts_id,tid,name,ts,dur,args_Data_type,args_Dest,args_Tag,args_Count,args_Send_Count,args_Recv_Count,args_Comm,args_Request,args_Op,args_Root,arg_pmu_0,arg_pmu_1,arg_pmu_2,arg_pmu_3\n");
    auto mhz = get_tsc_freq_mhz(); 

    struct hash_pair { 
        size_t operator()(const std::pair<int, int>& p) const {
            auto hash1 = std::hash<int>{}(p.first);
            auto hash2 = std::hash<int>{}(p.second);
            return hash1 ^ (hash2 << 1);
        }
    };

    std::unordered_map<std::pair<int, int>, std::vector<std::string>, hash_pair> merged_data;

    #pragma omp parallel
    {
        std::unordered_map<std::pair<int, int>, std::vector<std::string>, hash_pair> local_merged_data;
        #pragma omp for 
        for (int i = 0; i < static_cast<int>(_nodes.size()); ++i) {
            std::string name = "0";
            std::string ts = "0";
            std::string dur = "0";
            std::string datatype = "0";
            std::string dest = "0";
            std::string tag = "0";
            std::string count = "0";
            std::string sendcnt = "0";
            std::string recvcnt = "0";
            std::string comm = "0";
            std::string request = "0";
            std::string op = "0";
            std::string root = "0";
            std::string pmu0 = "0", pmu1 = "0", pmu2 = "0", pmu3 = "0";

            int ts_id = _nodes[i]->ts_id;
            int rank = _nodes[i]->rank;
            uint64_t off = _nodes[i]->offset;
            if(rank >= 0) {
            record_t* r = _nodes[i]->data;
            name = RecordHelper::get_record_name(r);
            double start_time = tsc_duration_us(r->timestamps.enter - off, mhz);
            double duration = tsc_duration_us(r->timestamps.exit - off, mhz) - start_time;
            ts = std::to_string(start_time);
            dur = std::to_string(duration);

            switch(r->MsgType) {
                case event_MPI_Send:
                case event_MPI_Recv: {
                    record_comm_t *rd = (record_comm_t *) r;
                    datatype = std::to_string(rd->datatype);
                    dest = std::to_string(rd->dest);
                    tag = std::to_string(rd->tag);
                    count = std::to_string(rd->count);
                    comm = std::to_string(rd->comm);
                    break;
                }
                case event_MPI_Isend:
                case event_MPI_Irecv: {
                    record_comm_async_t *rd = (record_comm_async_t *) r;
                    datatype = std::to_string(rd->datatype);
                    dest = std::to_string(rd->dest);
                    tag = std::to_string(rd->tag);
                    count = std::to_string(rd->count);
                    comm = std::to_string(rd->comm);
                    request = std::to_string(rd->request);
                    break;
                }
                case event_MPI_Wait: {
                    record_comm_wait_t *rd = (record_comm_wait_t *) r;
                    request = std::to_string(rd->request);
                    break;
                }
                case event_MPI_Alltoall:
                case event_MPI_Alltoallv: {
                    record_all2all_t *rd = (record_all2all_t *) r;
                    datatype = std::to_string(rd->datatype);
                    sendcnt = std::to_string(rd->sendcnt);
                    recvcnt = std::to_string(rd->recvcnt);
                    comm = std::to_string(rd->comm);
                    break;
                }
                case event_MPI_Allreduce: {
                    record_allreduce_t *rd = (record_allreduce_t *) r;
                    datatype = std::to_string(rd->datatype);
                    count = std::to_string(rd->count);
                    op = std::to_string(rd->op);
                    comm = std::to_string(rd->comm);
                    break;
                }
                case event_MPI_Reduce: {
                    record_reduce_t *rd = (record_reduce_t *) r;
                    datatype = std::to_string(rd->datatype);
                    count = std::to_string(rd->count);
                    root = std::to_string(rd->root);
                    op = std::to_string(rd->op);
                    comm = std::to_string(rd->comm);
                    break;
                }
                case event_MPI_Bcast: {
                    record_bcast_t *rd = (record_bcast_t *) r;
                    datatype = std::to_string(rd->datatype);
                    count = std::to_string(rd->count);
                    root = std::to_string(rd->root);
                    comm = std::to_string(rd->comm);
                    break;
                }
              }
            } else {
                record_t* r = _nodes[i]->data;
                name = demangle(_nodes[i]->name); 
                double start_time = tsc_duration_us(r->timestamps.enter - off, mhz);
                double duration = tsc_duration_us(r->timestamps.exit - off, mhz) - start_time;
                ts = std::to_string(start_time);
                dur = std::to_string(duration);
            }

            auto key = std::make_pair(ts_id, rank);
            if (local_merged_data.find(key) == local_merged_data.end()) {
                local_merged_data[key] = {
                    std::to_string(ts_id),
                    std::to_string(rank),
                    name,
                    ts,
                    dur,
                    datatype,
                    dest,
                    tag,
                    count,
                    sendcnt,
                    recvcnt,
                    comm,
                    request,
                    op,
                    root,
                    pmu0, pmu1, pmu2, pmu3};
            } else {
                auto& merged_row = local_merged_data[key];
                merged_row[2] += ";" + name;
                merged_row[3] += ";" + ts;
                merged_row[4] += ";" + dur;
                merged_row[5] += ";" + datatype;
                merged_row[6] += ";" + dest;
                merged_row[7] += ";" + tag;
                merged_row[8] += ";" + count;
                merged_row[9] += ";" + sendcnt;
                merged_row[10] += ";" + recvcnt;
                merged_row[11] += ";" + comm;
                merged_row[12] += ";" + request;
                merged_row[13] += ";" + op;
                merged_row[14] += ";" + root;
                merged_row[15] += ";" + pmu0;
                merged_row[16] += ";" + pmu1;
                merged_row[17] += ";" + pmu2;
                merged_row[18] += ";" + pmu3;
            }
        }

        #pragma omp critical
        {
            for (auto& entry : local_merged_data) {
                auto& global_entry = merged_data[entry.first];
                if (global_entry.empty()) {
                    global_entry = entry.second;
                } else {
                    for(size_t j = 0; j < global_entry.size(); ++j) {
                        if(j >= 2) global_entry[j] += ";" + entry.second[j];  
                    }
                }
            }
        }
    }

    std::vector<std::pair<std::pair<int, int>, std::vector<std::string>>> sorted_data(merged_data.begin(), merged_data.end());
    std::sort(sorted_data.begin(), sorted_data.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    for (const auto& entry : sorted_data) {
        const auto& row = entry.second;
        for (size_t i = 0; i < row.size(); i++) {
            fprintf(fp, "%s", row[i].c_str());
            if (i < row.size() - 1) {
                fprintf(fp, ",");
            }
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void DependancyGraph::dump_edges_omp(FILE* fp, RecordReader* reader) {
    RecordTraceCollection& traces = reader->get_all_traces();
    RankMetaCollection& metas = reader->get_all_meta_maps();
    ExtRecordTraceCollection& rte_collection = reader->get_all_etraces();
    RankRecordTraceCollection& rank_record_collection = reader->get_main_thread_traces();
    RankExtRecordTraceCollection& rank_ext_record_collection = reader->get_all_etraces_by_rank();

    auto m = _edges.size();
    
    #pragma omp parallel
    {
        std::vector<std::string> local_buffer;

        #pragma omp for
        for (int i = 0; i < static_cast<int>(m); ++i) {
            if (_edges[i]->type == DependencyType::COMM || _edges[i]->type == DependencyType::HIP) {
                int ts_id = _edges[i]->src->ts_id;
                int src_rank = _edges[i]->src->rank;
                int dst_rank = _edges[i]->dst->rank;
                int freq = _edges[i]->freq;
                int commsize = reinterpret_cast<record_comm_t*>(_edges[i]->src->data)->count;
                const char* type_str = DependencyType::to_string(_edges[i]->type);

                char buffer[256];
                snprintf(buffer, sizeof(buffer), "%d,%d,%d,%d,%d,%s\n",
                         ts_id, src_rank, dst_rank, freq, commsize, type_str);
                local_buffer.emplace_back(buffer);
            }
        }
        #pragma omp critical
        {
            for (const auto& line : local_buffer) {
                fprintf(fp, "%s", line.c_str());
            }
        }
    }
    fclose(fp);
}