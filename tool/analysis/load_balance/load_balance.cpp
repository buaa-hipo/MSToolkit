#include "load_balance.h"
#include <cstdio>

#include <algorithm>
#include <fstream>
#include <chrono>
#include "utils/tsc_timer.h"

#include "tbb/parallel_for.h"

namespace {

uint64_t get_comm_duration(const RecordNode &comm_node) {
    return comm_node.data->timestamps.exit - comm_node.data->timestamps.enter;
}

inline bool record_is_backtraceable(record_t *r) {
    return !RecordHelper::is_process_exit(r) && !RecordHelper::is_process_start(r) &&
           !RecordHelper::is_profiler(r);
}

inline std::string encode_backtrace(backtrace_context_t ctx, const BacktraceTree &tree) {
    std::stringbuf sb;
    while (ctx != BACKTRACE_ROOT_NODE) {
        const auto ip = tree.backtrace_context_ip(ctx);
        sb.sputn(reinterpret_cast<const char *>(&ip), sizeof(uint64_t));
        ctx = tree.get_parent(ctx);
    }
    return sb.str();
}

}// namespace

LoadBalanceAnalyzer::LoadBalanceAnalyzer(DependancyGraph *graph, BacktraceCollection *backtraces,
                                         double threshold_imbalance, int num_top_k)
    : _graph{*graph},
      _threshold_imbalance{threshold_imbalance},
      _backtraces{*backtraces},
      _num_top_k{num_top_k} {

    _node_aux.resize(_graph.nodes().size());
    _derive_clusters_from_backtrace();
    auto t1 = std::chrono::steady_clock::now();
    _merge_clusters();
    auto t2 = std::chrono::steady_clock::now();
    printf("Time _merge_clusters: %f\n", std::chrono::duration<double>(t2 - t1).count());
}

void LoadBalanceAnalyzer::_derive_clusters_from_backtrace() {
    // std::unordered_map<std::string, LoadBalanceCluster *> bt_cluster_map;
    typedef tbb::concurrent_hash_map<std::string, LoadBalanceCluster *> cmap;
    cmap bt_cluster_map;

    auto &nodes = _graph.nodes();

    auto t1 = std::chrono::steady_clock::now();
#pragma omp parallel for
    for (int i = 0; i < nodes.size(); i++) {

        auto *node = nodes[i];
        if (!record_is_backtraceable(node->data)) {
            continue;
        }

        auto node_ctx = (backtrace_context_t) node->data->ctxt;
        const auto &node_bt_tree = *_backtraces[node->rank];

        // auto &cluster = bt_cluster_map[encode_backtrace(node_ctx, node_bt_tree)];
        LoadBalanceCluster *cluster;
        {
            cmap::accessor accessor;
            auto found = bt_cluster_map.find(accessor, encode_backtrace(node_ctx, node_bt_tree));
            if (!found) {
                auto insert = bt_cluster_map.insert(accessor, encode_backtrace(node_ctx, node_bt_tree));
                if (insert) {
                    accessor->second = new LoadBalanceCluster;
                }
            }
            cluster = accessor->second;
        }
        _add_node_to_cluster(cluster, i);
    }

    auto t2 = std::chrono::steady_clock::now();
    printf("Time creating clusters: %f\n", std::chrono::duration<double>(t2 - t1).count());

    typedef tbb::concurrent_hash_map<int, LoadBalanceCluster *> icmap;

    _clusters.clear();
    tbb::parallel_for(bt_cluster_map.range(),
                      [this](const cmap::range_type &r) {
                          for(const auto & it : r){
                              auto *cluster = it.second;
                              {
                                  icmap::accessor accessor;
                                  auto insert = _clusters.insert(accessor, cluster->nodes[0]);
                                  if (insert) {
                                      accessor->second = cluster;
                                  }
                              }
                          }
                      }
    );

    auto t3 = std::chrono::steady_clock::now();
    printf("Time annotating clusters: %f\n", std::chrono::duration<double>(t3 - t2).count());
}

void LoadBalanceAnalyzer::_merge_clusters() {
    auto &nodes = _graph.nodes();
    auto &edges = _graph.edges();

    std::unordered_map<RecordNode *, int> reverse_idx_map;
    for (int i = 0; i < nodes.size(); i++) {
        reverse_idx_map[nodes[i]] = i;
    }

#pragma omp parallel for
    for (int i = 0; i < edges.size(); i++) {
        auto &e = edges[i];
        if ((e->type & (DependencyType::COMM | DependencyType::COLLECTIVE)) == 0x0) {
            continue;
        }
        if (e->dst->rank == e->src->rank) {
            continue;
        }
        if (!(e->dst->data->MsgType == event_MPI_Recv && e->src->data->MsgType == event_MPI_Send)) {
            continue;
        }
        int u = reverse_idx_map[e->src];
        int v = reverse_idx_map[e->dst];
#pragma omp critical
        {
            if (_node_aux[u].cluster != _node_aux[v].cluster) {
                auto *cluster = _node_aux[v].cluster;
                for (const auto &idx: cluster->nodes) {
                    _add_node_to_cluster(_node_aux[u].cluster, idx);
                }
                cluster->removed = true;
            }
        }
    }
}

void LoadBalanceAnalyzer::_add_node_to_cluster(LoadBalanceAnalyzer::LoadBalanceCluster *cluster,
                                               int node_idx) {
    cluster->nodes.push_back(node_idx);
    _node_aux[node_idx].cluster = cluster;
}

void LoadBalanceAnalyzer::analyze() {
    JSI_INFO("Analyzing...");
    std::vector<std::pair<int, LoadBalanceCluster*>> clusters;
    clusters.reserve(clusters.size());
    for (auto &it: _clusters) {
        if (!it.second->removed) {
            clusters.emplace_back(it);
        }
    }
    _data.comm_lbs.resize(clusters.size());
#pragma omp parallel for
    for (int i = 0; i < clusters.size(); i++) {
        auto &it = clusters[i];
        auto &comm_lb_data = _data.comm_lbs[i];

        uint64_t duration_sum = 0;
        uint64_t duration_max = 0;
        uint64_t duration_min = UINT64_MAX;

        auto &cluster = it.second;
        for (const auto &node: cluster->nodes) {
            uint64_t duration = get_comm_duration(*_graph.nodes()[node]);
            duration_sum += duration;
            duration_max = std::max(duration_max, duration);
            duration_min = std::min(duration_min, duration);
        }
        uint64_t duration_avg = duration_sum / cluster->nodes.size();
        cluster->duration_avg = duration_avg;
        cluster->duration_max = duration_max;
        cluster->duration_min = duration_min;

        for (int node_idx: cluster->nodes) {
            const auto *node = _graph.nodes()[node_idx];
            uint64_t duration = get_comm_duration(*node);
            double time_diff_normed =
                    (double) std::abs((int64_t) duration - (int64_t) duration_avg) / duration_avg;
            _node_aux[node_idx].time_diff_normed = time_diff_normed;

            comm_lb_data.items.emplace_back(node, time_diff_normed);
        }

        std::sort(comm_lb_data.items.begin(), comm_lb_data.items.end(),
                  [](const LoadBalanceItem &i1, const LoadBalanceItem &i2) {
                      return i1.second > i2.second;
                  });

        comm_lb_data.duration_max = duration_max;
        comm_lb_data.duration_min = duration_min;
        comm_lb_data.duration_avg = duration_avg;
    }

    std::sort(_data.comm_lbs.begin(), _data.comm_lbs.end(),
              [](const CommLoadBalanceRecord &i1, const CommLoadBalanceRecord &i2) {
                  return i1.items[0].second > i2.items[0].second;
      });
}

void LoadBalanceAnalyzer::dump_report(const char *filename) {
    auto mhz = get_tsc_freq_mhz();
    auto f = fopen(filename, "w");
//    std::ofstream f_out;
//    f_out.open(filename);

    for (int r = 0; r < std::min((int) _data.comm_lbs.size(), _num_top_k); r++) {
        auto &comm_lb_record = _data.comm_lbs[r];
        fprintf(f,
                "====== Communication Point ======\n"
                "%s"
                "====== Duration Details ======\n"
                "Duration (avg, min, max) (us): \n"
                "%f, %f, %f\n",
                RecordHelper::dump_string(comm_lb_record.items[0].first->data, _backtraces[comm_lb_record.items[0].first->rank]).c_str(),
                tsc_duration_us(comm_lb_record.duration_avg, mhz),
                tsc_duration_us(comm_lb_record.duration_min, mhz),
                tsc_duration_us(comm_lb_record.duration_max, mhz));

        // Nodes exceeding the threshold
        // TopK nodes
        fprintf(f, "====== Top %d Variance Ranks Exceeding The Threshold %f======\n", _num_top_k, _threshold_imbalance);
        for (int i = 0, n = std::min((int) comm_lb_record.items.size(), _num_top_k); i < n; i++) {
            auto &it = comm_lb_record.items[i];
            if (it.second <= _threshold_imbalance) {
                break;
            }
            fprintf(f, "    rank: %5d, normalized variance: %f\n", it.first->rank, it.second);
        }
    }
}
LoadBalanceAnalyzer::~LoadBalanceAnalyzer() {
    for (const auto &it: _clusters) {
        delete it.second;
    }
}
