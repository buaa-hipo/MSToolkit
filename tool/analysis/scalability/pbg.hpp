#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "record/record_reader.h"

namespace PBG {

enum class NodeType {
    SIMPLE_NODE,
    COMM_NODE,
    SYNC_NODE,
    COLLECTIVE_NODE,
};

// No MPI_Waitall record, so wait node would only have one sync dependent.
class BehaviorNode {
private:
    NodeType            node_type_;
    backtrace_context_t backtrace_;
    const uint64_t      unique_key_;
    uint64_t            time_duration_;
    uint64_t            ins_duration_;
    int                 call_count_;
    int                 visit_count_;
    double              anomaly_factor_;
    double              accumulated_anomaly_factor_;
    BehaviorNode       *data_dependent_;
    BehaviorNode       *comm_dependent_;
    BehaviorNode       *sync_dependent_;

    std::unordered_map<std::string, uint64_t> event_counts_;

public:
    const std::string str_id_;
    BehaviorNode() = delete;

    BehaviorNode(backtrace_context_t bt, uint64_t key, uint64_t time, uint64_t ins, std::string id,
                 NodeType type = NodeType::SIMPLE_NODE)
        : node_type_(type),
          backtrace_(bt),
          unique_key_(key),
          time_duration_(time),
          ins_duration_(ins),
          anomaly_factor_(0.0),
          accumulated_anomaly_factor_(0.0),
          visit_count_(0),
          str_id_(id),
          call_count_(1),
          data_dependent_(nullptr),
          comm_dependent_(nullptr),
          sync_dependent_(nullptr) {
    }

    void add_time(uint64_t time) {
        time_duration_ += time;
    }

    void add_ins(uint64_t ins) {
        ins_duration_ += ins;
    }

    void add_call_count() {
        ++call_count_;
    }

    void add_data_dependent(BehaviorNode *dependent) {
        // data_dependents_.push_back(dependent);
        data_dependent_ = dependent;
    }

    void add_comm_dependent(BehaviorNode *dependent) {
        comm_dependent_ = dependent;
    }

    void add_sync_dependent(BehaviorNode *dependent) {
        sync_dependent_ = dependent;
    }

    uint64_t get_unique_key() const {
        return unique_key_;
    }

    auto get_backtrace() {
        return backtrace_;
    }

    auto &get_anomaly_factor() {
        return anomaly_factor_;
    }

    auto &get_accumulated_anomaly_factor() {
        return accumulated_anomaly_factor_;
    }

    auto get_data_dependent() {
        return data_dependent_;
    }

    auto get_comm_dependent() {
        return comm_dependent_;
    }

    auto get_sync_dependent() {
        return sync_dependent_;
    }

    auto &get_visit_count() {
        return visit_count_;
    }

    auto get_node_type() {
        return node_type_;
    }
};

class AnomalyPath {
private:
    std::vector<BehaviorNode *> anomaly_nodes_;
    double                      path_anomaly_factor_;

public:
    AnomalyPath() = default;

    AnomalyPath(double factor) : path_anomaly_factor_(factor) {
    }

    AnomalyPath(std::vector<BehaviorNode *> &&nodes, double factor)
        : anomaly_nodes_(nodes), path_anomaly_factor_(factor) {
    }

    void   print_result() const;
    void   add_node(BehaviorNode *node);
    double get_path_anomaly_factor() const;
};

class ProgramBehaviorGraph {
private:
    bool                                                      pmu_involved_;
    std::unordered_map<uint32_t, std::vector<BehaviorNode *>> behavior_graph_;
    std::unordered_set<BehaviorNode *>                        candidate_anomaly_nodes_;
    std::vector<AnomalyPath>                                  anomaly_paths_;

    BacktraceCollection &backtrace_trees;
    // {<unique backtrace key>, <total_time, total_ins>}
    // Fingerprint of different nodes in differentiation stage.
    // std::unordered_map<uint64_t, PBG::BehaviorNode *> global_node_map;
    // std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> global_node_map_;
    std::unordered_map<uint64_t, std::vector<uint64_t>> fingerprint2time_;
    std::unordered_map<uint64_t, std::vector<uint64_t>> fingerprint2ins_;
    std::unordered_map<uint64_t, uint64_t>              fingerprint2tot_time_;
    std::unordered_map<uint64_t, uint64_t>              fingerprint2tot_ins_;
    std::unordered_map<uint64_t, double>                fingerprint2time_var_;

public:
    ProgramBehaviorGraph() = delete;

    ProgramBehaviorGraph(BacktraceCollection &bt) : backtrace_trees(bt) {
    }

    auto get_pbg_size() const {
        return behavior_graph_.size();
    }

    auto &behavior_graph() const {
        return behavior_graph_;
    }

    void build_graph_for_processes(RecordTraceCollection &, RankMetaCollection &, BacktraceCollection &);

    void build_graph_for_program();
    void add_inter_dependency();
    void differentiate(ProgramBehaviorGraph &, double, double, double, double);
    void backtrack(double decay_factor, double merge_factor);

    auto &get_fingerprint2time() {
        return fingerprint2time_;
    }

    auto &get_fingerprint2ins() {
        return fingerprint2ins_;
    }

    auto &get_fingerprint2tot_time() {
        return fingerprint2tot_time_;
    }

    auto &get_fingerprint2tot_ins() {
        return fingerprint2tot_ins_;
    }

    auto &get_fingerprint2time_var() {
        return fingerprint2time_var_;
    }

    auto get_anomaly_paths() const {
        return anomaly_paths_;
    }

    void sort_nodes();

    void print_result(int, std::ostream&);
};

};  // namespace PBG
