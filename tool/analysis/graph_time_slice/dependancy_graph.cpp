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
using namespace std;

namespace DependencyType {
    const char* to_string(uint32_t type) {
      switch(type) {
        case NONE: return "NONE";
        case COMM: return "COMM";
        case SYNC: return "SYNC";
        case COLLECTIVE: return "COLLECTIVE";
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

/* Currently, we do not handle communication domains other than mpi_comm_world. */
/* Note: only support send/recv pairs (DependencyType::COMM) */
DependancyGraph* CreateDependancyGraphSlice(RecordTraceCollection& collection, RankMetaCollection& metas, bool need_serial_link, uint64_t interval, uint64_t duration) {
    DependancyGraph* graph = new DependancyGraph();
    unordered_map<int/*rank*/, LastTrack_t*> track;
    /* Currently, we regard all traces are fully traced without sampling */
    /* FIXME: try to extract sampling information from meta data */
    /* add nodes and track all comm records */
    uint64_t start_time = UINT64_MAX;
    uint64_t exit_time = 0;

    for(auto it=collection.begin(), ie=collection.end(); it!=ie; ++it){
        int rank = it->first;
        RecordTrace& rtrace = *(it->second);
        uint64_t offset = rtrace.offset;
        // uint64_t offset = 0;
        RecordTraceIterator rti = rtrace.find(JSI_PROCESS_START, true /*ignore zoom*/);
        if (RecordTraceIterator::is_invalid(rti)) {
            JSI_ERROR("Error: Could not find PROCESS_START event in Rank %d\n", rank);
        }
        start_time =min(start_time, rti.val()->timestamps.enter - offset);
        rti = rtrace.find(JSI_PROCESS_EXIT, true);
        if (RecordTraceIterator::is_invalid(rti)) {
            JSI_ERROR("Error: Could not find PROCESS_EXIT event in Rank %d\n", rank);
        }
        exit_time =max(exit_time, rti.val()->timestamps.exit - offset);
    }

    if(exit_time - start_time > 9000000000000){
        JSI_ERROR("Error: Cannot handle programs run for more than 1 hour\n");
    }
    for(uint64_t ts = start_time; ts < exit_time; ts += interval){
        for(auto it=collection.begin(), ie=collection.end(); it!=ie; ++it) {
            int rank = it->first;
            int ts_id = (ts - start_time) / interval;
            RecordTrace& rtrace = *(it->second);
            auto num_pmu_events = rtrace.num_pmu_events();
            
            LastTrack_t* ltrack = new LastTrack_t;
            RecordNode* last = NULL;
            uint64_t offset = rtrace.offset;
            size_t event_id = 0;
            bool success = rtrace.zoom(ts, ts + duration, offset); 
            if (!success) {
                // ts_id++;
                continue;
            }
            for(auto ri=rtrace.begin(), re=rtrace.end(); ri!=re; ri=ri.next()) {
                record_t* r = ri.val();
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
                    node = new RecordNode(rank, ts_id, true, r, offset, pmu_values);
                } else {
                    node = new RecordNode(rank, ts_id, true, r, offset);
                }
                node->event_id = event_id++;
                graph->add_node(rank, node);
                if(need_serial_link && last!=NULL) {
                    graph->add_edge(new DependancyEdge(last, node, DependencyType::NONE));
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
                        graph->add_edge(new DependancyEdge(comm_node, node, DependencyType::SYNC));
                        ltrack->wait_list[rec->request] = NULL;
                    }
                    continue;
                }
            }
            track[rank] = ltrack;
            // delete ltrack;
            ts_id++;
        }
        /* generate dependancy linkages via tracked communication nodes */
        for(auto it=track.begin(), ie=track.end(); it!=ie; ++it) {
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
                    auto ttrack = track.find(target);
                    if(ttrack!=track.end()) {
                        auto trq = ttrack->second->recv_list.find(enc);
                        if(trq!=ttrack->second->recv_list.end()) {
                            queue<RecordNode*>& rq = trq->second;
                            if (rq.empty()) {
    #ifndef NDEBUG
                                // JSI_WARN("No rq matches rec %s\n", RecordHelper::dump_string(sn->data).c_str());
    #endif
                            } else {
                                RecordNode* rn = rq.front();
                                graph->add_edge(new DependancyEdge(sn, rn, DependencyType::COMM));
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
    }
    /* Return analyzed dependancy graph */
    return graph;
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

struct hash_pair {
    template <class T1, class T2>
    size_t operator()(const pair<T1, T2>& p) const {
        auto hash1 = hash<T1>{}(p.first);
        auto hash2 = hash<T2>{}(p.second);
        return hash1 ^ hash2;
    }
};

void DependancyGraph::dump_backtrace(const char* filename, BacktraceCollection* backtrace, const char* input_file) {
    if (backtrace == nullptr) {
        return;
    }
    std::set<std::pair<int, int>> filter_set;
    std::ifstream infile(input_file);
    if (!infile.is_open()) {
        perror("Failed to open input file");
        return;
    }

    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        int ts_id, tid;
        if (iss >> tid >> ts_id) {
            printf("%d %d\n", ts_id, tid);
            filter_set.emplace(ts_id, tid);
        }
    }
    infile.close();


    auto fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open file");
        return;
    }
    fprintf(fp, "ts_id,tid,backtrace\n");
    int n = _nodes.size();
    for (int i = 0; i < n; i++) {
        int ts_id = _nodes[i]->ts_id;
        int rank = _nodes[i]->rank;
        record_t* r = _nodes[i]->data;
        BacktraceTree* bt_tree = (*backtrace)[rank];
        if (filter_set.count({ts_id, rank}) > 0 && bt_tree != nullptr && RecordHelper::is_mpi(r) || RecordHelper::is_function(r)) {
            const char* s = bt_tree->backtrace_get_context_string((backtrace_context_t)r->ctxt);
            fprintf(fp, "%d,%d,\"%s\"\n", ts_id, rank, s);
        }
    }
}

void DependancyGraph::dump_nodes(const char* filename, const std::vector<std::string>& event_list) {
    auto fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open file");
        return;
    }
    fprintf(fp, "ts_id,tid,name,ts,dur,args_Data_type,args_Dest,args_Tag,args_Count,args_Send_Count,args_Recv_Count,args_Comm,args_Request,args_Op,args_Root,arg_pmu_0,arg_pmu_1,arg_pmu_2,arg_pmu_3\n");
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
        record_t* r = _nodes[i]->data;
        uint64_t off = _nodes[i]->offset;

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

        auto num_events = event_list.size();
        for (int eidx = 0; eidx < num_events; eidx++) {
            const auto enter_val = RecordHelper::counter_val_enter(r, eidx, num_events);
            const auto exit_val = RecordHelper::counter_val_exit(r, eidx, num_events);
            uint64_t pmu_counts = exit_val - enter_val;
            switch (eidx) {
                case 0: pmu0 = std::to_string(pmu_counts); break;
                case 1: pmu1 = std::to_string(pmu_counts); break;
                case 2: pmu2 = std::to_string(pmu_counts); break;
                case 3: pmu3 = std::to_string(pmu_counts); break;
            }
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
    fclose(fp);
}

void DependancyGraph::dump_edges(const char* filename) {
    auto fp = fopen(filename, "w");

    auto m = _edges.size();
    fprintf(fp, "ts_id,src,dst,commsize,type\n");
    for (auto i = 0; i < m; ++i) {
        if (_edges[i]->type == DependencyType::COMM) {
            // record_t* rs = _edges[i]->src->data;
            // record_comm_t* r = reinterpret_cast<record_comm_t*>(rs);
            fprintf(fp, "%d,%d,%d,%d,%s\n",
                _edges[i]->src->ts_id,
                _edges[i]->src->rank,
                _edges[i]->dst->rank,
                reinterpret_cast<record_comm_t*>(_edges[i]->src->data)->count,
                DependencyType::to_string(_edges[i]->type));
        }
    }

}