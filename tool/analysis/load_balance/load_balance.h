#ifndef JSITOOLKIT_LOAD_BALANCE_H
#define JSITOOLKIT_LOAD_BALANCE_H

#include <unordered_set>
#include <vector>

#include "tbb/concurrent_hash_map.h"
#include "tbb/concurrent_vector.h"

#include "../graph/dependancy_graph.h"

class LoadBalanceAnalyzer {
public:
    using LoadBalanceItem = std::pair<const RecordNode*, double>;
    struct CommLoadBalanceRecord {
        std::vector<LoadBalanceItem> items;
        uint64_t duration_max;
        uint64_t duration_min;
        uint64_t duration_avg;
    };

    struct LoadBalanceData {
        std::vector<CommLoadBalanceRecord> comm_lbs;
    };

    struct LoadBalanceCluster {
        tbb::concurrent_vector<int> nodes;
//        std::vector<int> nodes;
        bool removed = false;
        uint64_t duration_max;
        uint64_t duration_min;
        uint64_t duration_avg;
    };

    struct RecordNodeAuxData {
        LoadBalanceCluster* cluster;
        double time_diff_normed;
    };

    LoadBalanceAnalyzer(DependancyGraph* graph, BacktraceCollection* backtraces,
                        double threshold_imbalance, int num_top_k = 5);

    ~LoadBalanceAnalyzer();

    void dump_report(const char* filename);

    void analyze();

private:
    void _add_node_to_cluster(LoadBalanceCluster* cluster, int node_idx);

    void _derive_clusters_from_backtrace();
    void _merge_clusters();

    DependancyGraph& _graph;
    BacktraceCollection& _backtraces;

    tbb::concurrent_hash_map<int, LoadBalanceCluster *> _clusters;
    std::vector<RecordNodeAuxData> _node_aux;

    double _threshold_imbalance;
    int _num_top_k;

    LoadBalanceData _data;
};

#endif//JSITOOLKIT_LOAD_BALANCE_H
