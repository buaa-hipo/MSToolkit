#include <cstdint>
#include <unordered_map>
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
    NodeType                                  node_type_;
    const uint64_t                            unique_key_;
    const uint64_t                            enter_timestamp_;
    const uint64_t                            exit_timestamp_;
    double                                    anomaly_factor_;
    BehaviorNode                             *data_dependent_;
    BehaviorNode                             *children_;
    std::vector<BehaviorNode *>               comm_dependents_;
    std::vector<BehaviorNode *>               sync_dependents_;
    std::unordered_map<std::string, uint64_t> event_counts_;

public:
    BehaviorNode() = delete;

    BehaviorNode(uint64_t key, uint64_t enter_ts, uint64_t exit_ts, double factor,
                 NodeType type = NodeType::SIMPLE_NODE)
        : node_type_(type),
          unique_key_(key),
          enter_timestamp_(enter_ts),
          exit_timestamp_(exit_ts),
          anomaly_factor_(factor) {
    }

    void embed_event_counts(std::unordered_map<std::string, uint64_t> &&event_counts) {
        event_counts_ = event_counts;
    }

    void add_data_dependent(BehaviorNode *dependent) {
        // data_dependents_.push_back(dependent);
        data_dependent_ = dependent;
    }

    void add_comm_dependent(BehaviorNode *dependent) {
        comm_dependents_.push_back(dependent);
    }

    void add_sync_dependent(BehaviorNode *dependent) {
        sync_dependents_.push_back(dependent);
    }

    void add_child(BehaviorNode *child) {
        // children_.push_back(child);
        children_ = child;
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
    bool                                         pmu_involved_;
    std::unordered_map<uint32_t, BehaviorNode *> process_behavior_graph_;
    std::vector<BehaviorNode *>                  candidate_anomaly_nodes_;
    std::vector<AnomalyPath>                     anomaly_paths_;

public:
    ProgramBehaviorGraph() = default;

    auto get_pbg_size() const {
        return process_behavior_graph_.size();
    }

    void add_graph_by_rank(uint32_t rank, BehaviorNode *root);
    void add_inter_dependency();
    void differentiate(ProgramBehaviorGraph &another_pbg);
    void backtrack(double anomaly_threshold);

    auto get_anomaly_paths() const {
        return anomaly_paths_;
    }

    void operator-(ProgramBehaviorGraph &another_pbg) {
        differentiate(another_pbg);
    }
};

};  // namespace PBG

std::vector<PBG::BehaviorNode *> build_graph_for_processes(RecordTraceCollection &, RankMetaCollection &,
                                                           BacktraceCollection &);
PBG::ProgramBehaviorGraph       *build_graph_for_program(std::vector<PBG::BehaviorNode *> &);