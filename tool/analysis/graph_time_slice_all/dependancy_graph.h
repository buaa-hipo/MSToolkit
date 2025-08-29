#pragma once

#include "record/record_reader.h"


#include <set>
namespace DependencyType {

const uint32_t NONE = 0;       /* Only serialized control dependency for each process */
const uint32_t COMM = 1;       /*Send/Recv/Alltoall[/Gather/Scatter]/(ISend/IRecv+Wait)*/
const uint32_t SYNC = 2;       /*Wait*/
// TODO: not implemented
const uint32_t COLLECTIVE = 4; /*Reduce*/
const uint32_t HIP = 8;        /*Host to Device*/

// env
const uint32_t HOST = 0;
const uint32_t DEVICE = 1;
 
// TODO: add new dependency type for communicator synchronization event like MPI_Barrier.
const char* to_string(uint32_t type);

};// namespace DependencyType

class DependancyEdge;
struct RecordNode {
    RecordNode(int rank, record_t* r, uint64_t offset, const std::string& id)
        : rank(rank),
          data(static_cast<record_t*>(malloc(record_utils::get_record_size(r)))),
          offset(offset),
          id(id) {   
        if (data) {
            memcpy(data, r, record_utils::get_record_size(r)); 
        } else {
            throw std::bad_alloc(); 
        }
    }

    RecordNode(int rank, record_t* r, uint64_t offset, const std::vector<std::pair<uint64_t, uint64_t>>& pmu_values, const std::string& id)
        : rank(rank),
          data(static_cast<record_t*>(malloc(record_utils::get_record_size(r)))),
          offset(offset),
          pmu_values(pmu_values),
          id(id) {   
        if (data) {
            memcpy(data, r, record_utils::get_record_size(r)); 
        } else {
            throw std::bad_alloc(); 
        }
    }

    RecordNode(int rank, int ts_id, bool enclosed, record_t* r, uint64_t offset, const std::string& id)
        : rank(rank),
          ts_id(ts_id),
          enclosed(enclosed),
          data(static_cast<record_t*>(malloc(record_utils::get_record_size(r)))),
          offset(offset),
          id(id) {  
        if (data) {
            memcpy(data, r, record_utils::get_record_size(r)); 
        } else {
            throw std::bad_alloc(); 
        }
    }

    RecordNode(int rank, int ts_id, bool enclosed, record_t* r, uint64_t offset, const std::vector<std::pair<uint64_t, uint64_t>>& pmu_values, const std::string& id)
        : rank(rank),
          ts_id(ts_id),
          enclosed(enclosed),
          data(static_cast<record_t*>(malloc(record_utils::get_record_size(r)))),
          offset(offset),
          pmu_values(pmu_values),
          id(id) {  
        if (data) {
            memcpy(data, r, record_utils::get_record_size(r)); 
        } else {
            throw std::bad_alloc(); 
        }
    }

    RecordNode(int rank, int ts_id, bool enclosed, record_t* r, uint64_t offset, uint64_t key, const std::string& id)
        : rank(rank),
          ts_id(ts_id),
          enclosed(enclosed),
          data(static_cast<record_t*>(malloc(record_utils::get_record_size(r)))),
          offset(offset),
          key(key),
          id(id) {   
        if (data) {
            memcpy(data, r, record_utils::get_record_size(r)); 
        } else {
            throw std::bad_alloc(); 
        }
    }

    RecordNode(int rank, int ts_id, bool enclosed, record_t* r, uint64_t offset, char* name, const std::string& id)
        : rank(rank),
          ts_id(ts_id),
          enclosed(enclosed),
          data(static_cast<record_t*>(malloc(record_utils::get_record_size(r)))),
          offset(offset),
          name(name),
          id(id) {   
        if (data) {
            memcpy(data, r, record_utils::get_record_size(r)); 
        } else {
            throw std::bad_alloc(); 
        }
    }

    int rank;
    int event_id;
    uint64_t ts_id;
    bool enclosed;
    record_t* data;
    uint64_t offset;
    std::vector<DependancyEdge*> in_edges;
    std::vector<DependancyEdge*> out_edges;

    // DEVICE
    uint64_t key;
    char* name;

    // PMU
    std::vector<std::pair<uint64_t, uint64_t>> pmu_values{};

    // id
    std::string id;
};

struct EdgeKey {
    int src_rank;
    uint64_t src_tsid; 
    int dst_rank;
    uint64_t dst_tsid; 
    uint32_t type;

    bool operator==(const EdgeKey& other) const {
        return src_rank == other.src_rank && 
               dst_rank == other.dst_rank && 
               type == other.type &&
               src_tsid == other.src_tsid &&
               dst_tsid == other.dst_tsid;
    }
};

namespace std {
    template<>
    struct hash<EdgeKey> {
        size_t operator()(const EdgeKey& key) const {
            size_t h1 = std::hash<int>()(key.src_rank);
            size_t h2 = std::hash<int>()(key.dst_rank);
            size_t h3 = std::hash<uint32_t>()(key.type);
            size_t h4 = std::hash<uint64_t>()(key.src_tsid); 
            size_t h5 = std::hash<uint64_t>()(key.dst_tsid); 
            
            return h1 ^ (h2 + 0x9e3779b9 + (h2 << 6) + (h2 >> 2)) ^
                   (h3 + 0x9e3779b9 + (h3 << 6) + (h3 >> 2)) ^
                   (h4 + 0x9e3779b9 + (h4 << 6) + (h4 >> 2)) ^
                   (h5 + 0x9e3779b9 + (h5 << 6) + (h5 >> 2));
        }
    };
}

struct DependancyEdge {
    DependancyEdge(RecordNode* src, RecordNode* dst, uint32_t type)
        : type(type),
          src(src),
          dst(dst),
          freq(1) {
        src->out_edges.push_back(this);
        dst->in_edges.push_back(this);
    }
    uint32_t type;
    RecordNode* src;
    RecordNode* dst;
    uint32_t freq;
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
        
        // printf("add node %d %d %d\n", rank, n->ts_id, n->rank);
        _nodes.push_back(n);
    }
    void add_edge(DependancyEdge* e) {
        // printf("add edge %d %d\n",e->src->rank,e->dst->rank);
        EdgeKey key{e->src->rank, e->src->ts_id, e->dst->rank, e->dst->ts_id, e->type};
        auto it = _edgeMap.find(key);
        if (it != _edgeMap.end()) {
            ++(it->second->freq);
            //delete e; 
        } else {
            _edges.push_back(e);
            _edgeMap[key] = e; 
        }
    }
    std::vector<RecordNode*>& nodes() {
        return _nodes;
    }
    std::vector<DependancyEdge*>& edges() {
        return _edges;
    }

    int get_nodes_size() {
        return _nodes.size();
    }
    int get_edges_size() {
        return _edges.size();
    }
    /* dump chrome trace format JSON file */
    void dump_nodes(FILE* fp, const std::vector<std::string>& event_list, RecordReader* reader);

    void dump_backtrace(FILE* fp, BacktraceCollection* backtrace, std::set<std::pair<int, int>> filter_set, RecordReader* reader);
    void dump_backtrace_without_filter(FILE* fp, BacktraceCollection* backtrace, RecordReader* reader);

    // void dump_backtrace_inner(FILE* fp, BacktraceCollection* backtrace, std::set<std::pair<int, int>> filter_set , RecordReader* reader);

    void dump_edges(FILE* fp, RecordReader* reader);

    void merge_nodes(std::vector<RecordNode*> nodes);

    void merge_edges(std::vector<DependancyEdge*> edges);
    
    /* dump with omp parallel */
    void dump_nodes_omp(FILE* fp, const std::vector<std::string>& event_list, RecordReader* reader);
    void dump_edges_omp(FILE* fp, RecordReader* reader);

private:
    std::vector<RecordNode*> _nodes;
    std::vector<DependancyEdge*> _edges;
    std::unordered_map<EdgeKey, DependancyEdge*> _edgeMap;
};
                                            
void CreateDependancyGraphSliceAll(RecordReader* reader, bool need_serial_link, uint64_t interval, uint64_t duration, const char* filename,
                                   uint64_t start, uint64_t end,  bool bt_only, const char* input_file, bool if_bt);

// DependancyGraph* CreateDependancyGraphSliceAll_OMP_ONLY_FOR_DUMPBT(RecordReader* reader, bool need_serial_link, uint64_t interval, uint64_t duration, const char* filename, BacktraceCollection* backtrace, const char* input_file,  uint64_t start, uint64_t end);