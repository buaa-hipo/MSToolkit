#include "dependancy_graph.h"
#include "../timeline/alignment.h"
#include <unordered_map>
#include <list>
#include <queue>
#include <string>
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
DependancyGraph* CreateDependancyGraph(RecordTraceCollection& collection, RankMetaCollection& metas, bool need_serial_link) {
    DependancyGraph* graph = new DependancyGraph();
    unordered_map<int/*rank*/, LastTrack_t*> track;
    /* Currently, we regard all traces are fully traced without sampling */
    /* FIXME: try to extract sampling information from meta data */
    /* add nodes and track all comm records */
    for(auto it=collection.begin(), ie=collection.end(); it!=ie; ++it) {
        int rank = it->first;
        RecordTrace& rtrace = *(it->second);
        auto num_pmu_events = rtrace.num_pmu_events();
        LastTrack_t* ltrack = new LastTrack_t;
        RecordNode* last = NULL;
        uint64_t offset = rtrace.offset;
        size_t event_id = 0;
        for(auto ri=rtrace.begin(), re=rtrace.end(); ri!=re; ri=ri.next()) {
            record_t* r = ri.val();
            RecordNode* node;
            if (num_pmu_events > 0) {
                std::vector<std::pair<uint64_t, uint64_t>> pmu_values;
                for (int i = 0; i < num_pmu_events; i++) {
                    pmu_values.emplace_back(RecordHelper::counter_val_enter(r, i, num_pmu_events),
                                            RecordHelper::counter_val_exit(r, i, num_pmu_events));
                }
                node = new RecordNode(rank, r, offset, pmu_values);
            } else {
                node = new RecordNode(rank, r, offset);
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
                            JSI_WARN("No rq matches rec %s\n", RecordHelper::dump_string(sn->data).c_str());
#endif
                        } else {
                            RecordNode* rn = rq.front();
                            graph->add_edge(new DependancyEdge(sn, rn, DependencyType::COMM));
                            rq.pop();
                        }
                    }
                }
                sq.pop();
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

void DependancyGraph::dump_json(const char* filename, BacktraceCollection* backtrace, const std::vector<std::string>& event_list) {
    auto mhz = get_tsc_freq_mhz();
    ChromeTraceWriter writer(filename);
    int n = _nodes.size();
    for (int i = 0; i < n; ++i) {
        int rank = _nodes[i]->rank;
        record_t* r = _nodes[i]->data;
        uint64_t off = _nodes[i]->offset;
        vector<ChromeTraceWriter::Arg> args;
        extract_args_from_record(r, &args);
        if (backtrace != nullptr) {
            extract_backtrace_args_from_record(r, &args, (*backtrace)[rank]);
        }
        extract_pmu_args_from_record(r, &args, event_list);
        writer.add_duration_event(
                RecordHelper::get_record_name(r),
                "Event",
                tsc_duration_us(r->timestamps.enter-off, mhz),
                tsc_duration_us(r->timestamps.exit-off, mhz),
                0,
                rank,
                args);
    }
    int m = _edges.size();
    for (int i = 0; i < m; ++i) {
        vector<ChromeTraceWriter::Arg> args;
        record_t* rs = _edges[i]->src->data;
        record_t* re = _edges[i]->dst->data;
        uint64_t off_s = _edges[i]->src->offset;
        uint64_t off_e = _edges[i]->dst->offset;
        uint32_t type = _edges[i]->type;
        if (type == DependencyType::COMM) {
            record_comm_t* r = reinterpret_cast<record_comm_t*>(rs);
            { ChromeTraceWriter::Arg a("communication size", r->count); args.push_back(a); }
        }
        writer.add_flow_start_event(
                "dep_flow_",
                DependencyType::to_string(type),
                tsc_duration_us(type == DependencyType::COMM ? rs->timestamps.enter-off_s : rs->timestamps.exit-off_s, mhz),
                0,
                _edges[i]->src->rank,
                i,
                args);
        writer.add_flow_end_event(
                "dep_flow_",
                DependencyType::to_string(type),
                tsc_duration_us(re->timestamps.enter-off_e, mhz),
                0,
                _edges[i]->dst->rank,
                i,
                args);
    }
}

void DependancyGraph::dump_edges(const char* filename) {
    auto fp = fopen(filename, "w");

    auto m = _edges.size();
    for (auto i = 0; i < m; ++i) {
        fprintf(fp, "<%d,%d>-><%d,%d>(%s)\n",
                _edges[i]->src->rank,
                _edges[i]->src->event_id,
                _edges[i]->dst->rank,
                _edges[i]->dst->event_id,
                DependencyType::to_string(_edges[i]->type));
    }

}