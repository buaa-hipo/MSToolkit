#pragma once

#include "record/record_reader.h"
#include "record/record_utils.h"

namespace DependencyType {

const uint32_t NONE = 0;       /* Only serialized control dependency for each process */
const uint32_t COMM = 1;       /*Send/Recv/Alltoall[/Gather/Scatter]/(ISend/IRecv+Wait)*/
const uint32_t SYNC = 2;       /*Wait*/
// TODO: not implemented
const uint32_t COLLECTIVE = 4; /*Reduce*/
// TODO: add new dependency type for communicator synchronization event like MPI_Barrier.
const char* to_string(uint32_t type);

};// namespace DependencyType

class DependancyEdge;
struct RecordNode {
    RecordNode(int rank, record_t* r, uint64_t offset) : rank(rank), data(static_cast<record_t*>(malloc(record_utils::get_record_size(r)))), offset(offset) {
        if (data) {
            memcpy(data, r, record_utils::get_record_size(r)); 
        } else {
            throw std::bad_alloc(); 
        }
    }
    RecordNode(int rank, record_t* r, uint64_t offset, const std::vector<std::pair<uint64_t, uint64_t>>& pmu_values)
        : rank(rank),
          data(static_cast<record_t*>(malloc(record_utils::get_record_size(r)))),
          offset(offset),
          pmu_values(pmu_values) {
            if (data) {
                memcpy(data, r, record_utils::get_record_size(r)); 
            } else {
                throw std::bad_alloc(); 
            }
          }

    int rank;
    int event_id;
    record_t* data;
    uint64_t offset;
    std::vector<DependancyEdge*> in_edges;
    std::vector<DependancyEdge*> out_edges;

    // PMU
    std::vector<std::pair<uint64_t, uint64_t>> pmu_values{};
};

struct DependancyEdge {
    DependancyEdge(RecordNode* src, RecordNode* dst, uint32_t type)
        : type(type),
          src(src),
          dst(dst) {
        src->out_edges.push_back(this);
        dst->in_edges.push_back(this);
    }
    uint32_t type;
    RecordNode* src;
    RecordNode* dst;
};

class DependancyGraph {
public:
    DependancyGraph() = default;
    ~DependancyGraph() {
        int n = _nodes.size();
        for (int i = 0; i < n; ++i) {
            delete _nodes[i];
        }
        _nodes.clear();
        int m = _edges.size();
        for (int i = 0; i < m; ++i) {
            delete _edges[i];
        }
        _edges.clear();
    }

    void add_node(int rank, RecordNode* n) {
        _nodes.push_back(n);
    }
    void add_edge(DependancyEdge* e) {
        _edges.push_back(e);
    }
    const std::vector<RecordNode*>& nodes() {
        return _nodes;
    }
    const std::vector<DependancyEdge*>& edges() {
        return _edges;
    }
    /* dump chrome trace format JSON file */
    void dump_json(const char* filename, BacktraceCollection* backtrace, const std::vector<std::string>& event_list);

    void dump_edges(const char* filename);

    void merge_nodes(std::vector<RecordNode*> nodes);

    void merge_edges(std::vector<DependancyEdge*> edges);
private:
    std::vector<RecordNode*> _nodes;
    std::vector<DependancyEdge*> _edges;
};

DependancyGraph* CreateDependancyGraph(RecordTraceCollection& collection, RankMetaCollection& metas, bool need_serial_link);
