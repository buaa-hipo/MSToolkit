#include "pbg.hpp"

#include <cmath>
#include <deque>
#include <iostream>

namespace PBG {

uint64_t encode(record_t* rec, BacktraceTree* bt) {
    // printf("Encoding %lu %p: %s\n", rec->ctxt, bt,
    // RecordHelper::dump_string(rec).c_str());
    // backtrace_context_t ctxt  = rec->ctxt;
    // backtrace_context_t pctxt = bt->get_parent(ctxt);
    // if (pctxt != BACKTRACE_UNKNOWN_NODE) {
    //     return reinterpret_cast<uint64_t>(bt->backtrace_context_ip(ctxt))
    //            ^ reinterpret_cast<uint64_t>(bt->backtrace_context_ip(pctxt));
    // }
    // return reinterpret_cast<uint64_t>(bt->backtrace_context_ip(ctxt));
    // backtrace_context_t ctxt = rec->ctxt;
    // auto fingerprint_string = std::string(bt->backtrace_get_context_string(ctxt));
    // auto fingerprint = std::hash<std::string>()(fingerprint_string);
    auto fingerprint = rec->ctxt;
    return fingerprint;
}

std::string encode_comm_info(int dest, int tag) {
    return std::to_string(dest) + std::string("/") + std::to_string(tag);
}

std::pair<int, int>* decode_comm_info(std::string info) {
    auto pos    = info.find('/');
    auto res    = new std::pair<int, int>;
    res->first  = std::stoi(info.substr(0, pos));
    res->second = std::stoi(info.substr(pos + 1));
    return res;
}

// Welford one-pass variance algorithm.
double calculate_standard_deviation(std::vector<uint64_t>& times) {
    uint64_t count = 0;
    double   mean  = 0.0;
    double   m2    = 0.0;

    for (uint64_t time : times) {
        ++count;
        double delta = static_cast<double>(time) - mean;
        mean += delta / count;
        double delta2 = static_cast<double>(time) - mean;
        m2 += delta * delta2;
    }

    // 总体标准差 (除以n)
    double variance = m2 / count;
    return std::sqrt(variance);
}

}  // namespace PBG

static bool is_hip_kernel_launch(record_t* record) {
    return RecordHelper::is_event(record, event_hipLaunchKernel)
           || RecordHelper::is_event(record, event_hipExtLaunchKernel)
           || RecordHelper::is_event(record, event_hipExtLaunchMultiKernelMultiDevice)
           || RecordHelper::is_event(record, event_hipLaunchCooperativeKernelMultiDevice)
           || RecordHelper::is_event(record, event_hipExtModuleLaunchKernel)
           || RecordHelper::is_event(record, event_hipHccModuleLaunchKernel)
           || RecordHelper::is_event(record, event_hipLaunchCooperativeKernel)
           || RecordHelper::is_event(record, event_hipLaunchKernel_internal)
           || RecordHelper::is_event(record, event_hipModuleLaunchKernel);
    //    || RecordHelper::is_event(record,
    //    event_hipModuleLaunchCooperativeKernelMultiDevice)
    //    || RecordHelper::is_event(record,
    //    event_hipModuleLaunchCooperativeKernel);
}

static bool is_mt_kernel_launch(record_t* record) {
    return RecordHelper::is_event(record, event_ACCL_API_group_create_launch)
           || RecordHelper::is_event(record, event_ACCL_API_group_create_masked_launch)
           || RecordHelper::is_event(record, event_ACCL_API_group_exec);
}

static bool is_hip_memcpy_async(record_t* record) {
    return RecordHelper::is_event(record, event_hipMemcpyAsync)
           || RecordHelper::is_event(record, event_hipDrvMemcpy3DAsync)
           || RecordHelper::is_event(record, event_hipMemcpy2DAsync)
           || RecordHelper::is_event(record, event_hipMemcpy2DFromArrayAsync)
           || RecordHelper::is_event(record, event_hipMemcpy2DToArrayAsync)
           || RecordHelper::is_event(record, event_hipMemcpy3DAsync)
           || RecordHelper::is_event(record, event_hipMemcpyDtoDAsync)
           || RecordHelper::is_event(record, event_hipMemcpyDtoHAsync)
           || RecordHelper::is_event(record, event_hipMemcpyFromSymbolAsync)
           || RecordHelper::is_event(record, event_hipMemcpyHtoDAsync)
           || RecordHelper::is_event(record, event_hipMemcpyParam2DAsync)
           || RecordHelper::is_event(record, event_hipMemcpyPeerAsync)
           || RecordHelper::is_event(record, event_hipMemcpyToSymbolAsync)
           || RecordHelper::is_event(record, event_hipMemcpyWithStream);
}

static bool is_mpi_trackable_sync_collective(record_t* record) {
    return RecordHelper::is_event(record, event_MPI_Bcast);
    // return RecordHelper::is_event(record, event_MPI_Scatter) ||
    // RecordHelper::is_event(record, event_MPI_Scatterv)
    //        || RecordHelper::is_event(record, event_MPI_Bcast);
}

static bool is_mpi_trackable_async_collective(record_t* record) {
    return RecordHelper::is_event(record, event_MPI_Ibcast);
    // return RecordHelper::is_event(record, event_MPI_Ibcast) ||
    // RecordHelper::is_event(record, event_MPI_Iscatterv)
    //        || RecordHelper::is_event(record, event_MPI_Iscatterv);
}

// {<unique backtrace key>, node}
// For intra-process PBG compress.
std::unordered_map<uint64_t, PBG::BehaviorNode*> local_node_map;

// {<dest/tag>, <nodes>}
std::unordered_map<int, std::unordered_map<std::string, std::deque<PBG::BehaviorNode*>>> senders_map;
std::unordered_map<int, std::unordered_map<std::string, std::deque<PBG::BehaviorNode*>>> receivers_map;

// ! comm cannot be used to identify one collective mpi operation
// {<comm>, <nodes>}
// std::unordered_map<uint64_t, std::deque<PBG::BehaviorNode*>>              collective_senders_map;
// std::unordered_map<uint64_t, std::deque<std::vector<PBG::BehaviorNode*>>> collective_receivers_map;
// ! suppose all collective operations are for MPI_COMM_WORLD
std::deque<PBG::BehaviorNode*>              collective_senders;
std::deque<std::vector<PBG::BehaviorNode*>> collective_receivers;

// {request, <nodes>}
std::unordered_map<uint64_t, PBG::BehaviorNode*> mpi_waiters_map;

// {<stream or event pointer>, <nodes>}
std::unordered_map<uint64_t, PBG::BehaviorNode*> hip_waiters_map;
std::unordered_map<uint64_t, PBG::BehaviorNode*> mt_waiters_map;

/*
    For intra-process dependency, add sync dependent to the wait node from map.
    For inter-process p2p send-receive dependency, receivers find their senders
   from senders_map by encoded dest/tag. For inter-process collective
   send-receive dependency: [Scatter/Broadcast]
*/
void PBG::ProgramBehaviorGraph::build_graph_for_processes(RecordTraceCollection& traces, RankMetaCollection& metas,
                                                          BacktraceCollection& backtraces) {
    for (auto trace_it = traces.begin(); trace_it != traces.end(); ++trace_it) {
        auto  str_id                   = trace_it->first;
        auto& trace                    = *(trace_it->second);
        auto  backtrace                = backtraces[str_id];
        int   rank                     = trace.rank();
        int   index_for_collective_map = 0;

        auto event_list = trace.pmu_event_list();
        if (event_list.size() == 0) {
            JSI_WARN("Trace of rank %d has no pmu events\n", rank);
            continue;
        }

        auto pos = std::find(event_list.begin(), event_list.end(), "PAPI_TOT_INS");
        if (pos == event_list.end()) JSI_ERROR("Cannot find PAPI_TOT_INS\n");
        size_t ins_index = std::distance(pos, event_list.begin());

        local_node_map.clear();
        mpi_waiters_map.clear();
        hip_waiters_map.clear();
        mt_waiters_map.clear();

        PBG::BehaviorNode* father = nullptr;

        for (auto record_it = trace.begin(); record_it != trace.end(); record_it = record_it.next()) {
            auto record = record_it.val();
            if (!RecordHelper::is_valid_ctxt(record)) continue;
            auto pmus =
                reinterpret_cast<uint64_t*>(reinterpret_cast<char*>(record) + record_utils::get_record_size(record));
            auto                unique_key = PBG::encode(record, backtrace);
            backtrace_context_t bt         = record->ctxt;

            uint64_t time_duration = record->timestamps.exit - record->timestamps.enter;
            uint64_t ins_duration  = pmus[event_list.size() + ins_index] - pmus[ins_index];

            PBG::BehaviorNode* node = nullptr;
            if (fingerprint2time_.contains(unique_key)) {
                fingerprint2time_.at(unique_key).push_back(time_duration);
                fingerprint2ins_.at(unique_key).push_back(ins_duration);
                fingerprint2tot_time_.at(unique_key) += time_duration;
                fingerprint2tot_ins_.at(unique_key) += ins_duration;
            } else {
                fingerprint2time_[unique_key].push_back(time_duration);
                fingerprint2ins_[unique_key].push_back(ins_duration);
                fingerprint2tot_time_[unique_key] = time_duration;
                fingerprint2tot_ins_[unique_key]  = ins_duration;
            }
            if (local_node_map.contains(unique_key)) {
                local_node_map[unique_key]->add_time(time_duration);
                local_node_map[unique_key]->add_ins(ins_duration);
                local_node_map[unique_key]->add_call_count();
                // Dependency should have been dealt with.
                father = local_node_map[unique_key];
                continue;
            }
            // MPI p2p.
            if (RecordHelper::is_event(record, event_MPI_Recv)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::COMM_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }
                auto comm_record = reinterpret_cast<record_comm_t*>(record);
                auto info        = PBG::encode_comm_info(comm_record->dest, comm_record->tag);
                receivers_map[rank][info].push_back(node);
            } else if (RecordHelper::is_event(record, event_MPI_Send)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::COMM_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }
                auto comm_record = reinterpret_cast<record_comm_t*>(record);
                auto info        = PBG::encode_comm_info(comm_record->dest, comm_record->tag);
                senders_map[rank][info].push_back(node);
            } else if (RecordHelper::is_event(record, event_MPI_Isend)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::COMM_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }
                auto comm_record = reinterpret_cast<record_comm_async_t*>(record);
                auto info        = PBG::encode_comm_info(comm_record->dest, comm_record->tag);
                senders_map[rank][info].push_back(node);
                mpi_waiters_map[static_cast<uint64_t>(comm_record->request)] = node;
            } else if (RecordHelper::is_event(record, event_MPI_Irecv)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::COMM_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }
                auto comm_record = reinterpret_cast<record_comm_async_t*>(record);
                auto info        = PBG::encode_comm_info(comm_record->dest, comm_record->tag);
                receivers_map[rank][info].push_back(node);
                mpi_waiters_map[static_cast<uint64_t>(comm_record->request)] = node;
            } else if (RecordHelper::is_event(record, event_MPI_Wait)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::SYNC_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }
                auto comm_record = reinterpret_cast<record_comm_wait_t*>(record);
                auto request     = comm_record->request;
                if (mpi_waiters_map.contains(request)) {
                    auto dependent = mpi_waiters_map.at(request);
                    node->add_sync_dependent(dependent);
                    mpi_waiters_map.erase(request);
                } else {
                    std::cout << "Unable to find synchronize dependent for MPI_Wait" << std::endl;
                }
            }
            // HThread kernel launch.
            else if (is_mt_kernel_launch(record)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::SIMPLE_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }
                auto kernel_launch_record = reinterpret_cast<mt_record_kernel_launch_t*>(record);
                mt_waiters_map.insert_or_assign(kernel_launch_record->group_id, node);
            } else if (RecordHelper::is_event(record, event_ACCL_API_group_wait)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::SYNC_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }
                auto group_wait_record = reinterpret_cast<mt_record_group_wait_t*>(record);
                auto thread_group_id   = group_wait_record->group_id;
                if (mt_waiters_map.contains(thread_group_id)) {
                    node->add_sync_dependent(mt_waiters_map.at(thread_group_id));
                    mt_waiters_map.erase(thread_group_id);
                } else {
                    std::cout << "Unable to find synchronize dependent for "
                                 "hthread_group_wait"
                              << std::endl;
                }
            }
            // HIP async.
            else if (is_hip_kernel_launch(record)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::SIMPLE_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }
                auto kernel_launch_record = reinterpret_cast<record_activity_launch_t*>(record);
                auto stream               = kernel_launch_record->stream;
                hip_waiters_map.insert_or_assign(reinterpret_cast<uint64_t>(stream), node);
            } else if (is_hip_memcpy_async(record)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::SIMPLE_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }
                auto memcpy_async_record = reinterpret_cast<record_activity_memcpy_async_t*>(record);
                auto stream              = memcpy_async_record->stream;
                hip_waiters_map.insert_or_assign(reinterpret_cast<uint64_t>(stream), node);
            } else if (RecordHelper::is_event(record, event_hipEventRecord)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::SIMPLE_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }

                auto event_record = reinterpret_cast<record_activity_event_t*>(record);
                auto event        = event_record->event;
                hip_waiters_map.insert_or_assign(reinterpret_cast<uint64_t>(event), node);
            } else if (RecordHelper::is_event(record, event_hipEventSynchronize)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::SYNC_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }
                auto event_record = reinterpret_cast<record_activity_event_t*>(record);
                auto event        = event_record->event;
                if (hip_waiters_map.contains(reinterpret_cast<uint64_t>(event))) {
                    node->add_sync_dependent(hip_waiters_map.at(reinterpret_cast<uint64_t>(event)));
                    hip_waiters_map.erase(reinterpret_cast<uint64_t>(event));
                }
            }
            // MPI collective.
            else if (is_mpi_trackable_sync_collective(record)) {
                node = new PBG::BehaviorNode(bt, unique_key, time_duration, ins_duration, str_id,
                                             PBG::NodeType::COLLECTIVE_NODE);
                if (node == nullptr) {
                    JSI_ERROR("Node initialization failed!\n");
                }

                auto collective_record = reinterpret_cast<record_bcast_t*>(record);
                auto root              = collective_record->root;
                auto comm              = collective_record->comm;

                // To make sure that corresponding relationships between
                // collective nodes are valid. Nodes at the same position of
                // collective sender/receivers map are for the same collective
                // operation.
                // if (root == rank) {
                //     if (index_for_collective_map >= collective_senders_map[comm].size()) {
                //         collective_senders_map[comm].push_back(node);
                //     } else {
                //         // Should not enter this branch in current version of
                //         // JSI-Toolkit.
                //         collective_senders_map[comm][index_for_collective_map] = node;
                //     }
                // } else {
                //     if (index_for_collective_map >= collective_receivers_map[comm].size()) {
                //         collective_receivers_map[comm].push_back({node});
                //     } else {
                //         collective_receivers_map[comm][index_for_collective_map].push_back(node);
                //     }
                // }
                if (root == rank) {
                    if (index_for_collective_map >= collective_senders.size()) {
                        collective_senders.push_back(node);
                    } else {
                        // Should not enter this branch in current version of
                        // JSI-Toolkit.
                        collective_senders[index_for_collective_map] = node;
                    }
                } else {
                    if (index_for_collective_map >= collective_receivers.size()) {
                        collective_receivers.push_back({node});
                    } else {
                        collective_receivers[index_for_collective_map].push_back(node);
                    }
                }
                ++index_for_collective_map;
            }
            // !
            // !
            // ?
            /*
            else if (is_mpi_trackable_async_collective(record)) {
                node = new PBG::BehaviorNode(bt, unique_key,
            record->timestamps.enter, record->timestamps.exit, time_duration,
            ins_duration, PBG::NodeType::COLLECTIVE_NODE);

                auto collective_record = reinterpret_cast<record_bcast_t
            *>(record); auto root              = collective_record->root; auto
            comm              = collective_record->comm;

                // To make sure that corresponding relationships between
            collective nodes are valid.
                // Nodes at the same position of collective sender/receivers map
            are for the same collective operation. if (root == rank) { if
            (index_for_collective_map >= collective_senders_map[comm].size()) {
                        collective_senders_map[comm].push_back(node);
                    } else {
                        // Should not enter this branch in current version of
            JSI-Toolkit. collective_senders_map[comm][index_for_collective_map]
            = node;
                    }
                } else {
                    if (index_for_collective_map >=
            collective_receivers_map[comm].size()) {
                        collective_receivers_map[comm].push_back({node});
                    } else {
                        collective_receivers_map[comm][index_for_collective_map].push_back(node);
                    }
                }
                ++index_for_collective_map;
            }
            */
            if (node != nullptr) {
                local_node_map[unique_key] = node;
                if (father != nullptr) node->add_data_dependent(father);
                behavior_graph_[rank].push_back(node);
            }
            father = node;
        }
    }
}

void PBG::ProgramBehaviorGraph::build_graph_for_program() {
    add_inter_dependency();
}

void PBG::ProgramBehaviorGraph::add_inter_dependency() {
    // Deal with p2p comm dependency.
    for (auto process_it = receivers_map.begin(); process_it != receivers_map.end(); ++process_it) {
        int         rank      = process_it->first;
        const auto& receivers = process_it->second;
        for (auto receiver_it = receivers.begin(); receiver_it != receivers.end(); ++receiver_it) {
            auto        sender_info    = decode_comm_info(receiver_it->first);
            int         sender_rank    = sender_info->first;
            int         tag            = sender_info->second;
            std::string sender_key     = encode_comm_info(rank, tag);
            auto        sender_nodes   = senders_map.at(sender_rank).at(sender_key);
            auto        receiver_nodes = receiver_it->second;
            while (sender_nodes.size() > 0) {
                receiver_nodes.front()->add_comm_dependent(sender_nodes.front());
                receiver_nodes.pop_front();
                sender_nodes.pop_front();
            }
        }
    }

    // Deal with collective comm dependency.
    for (auto comm_it = collective_receivers.begin(); comm_it != collective_receivers.end(); ++comm_it) {
        auto receivers = *comm_it;
        for (auto node : receivers) {
            node->add_comm_dependent(collective_senders.front());
        }
        collective_senders.pop_front();
    }
}

void PBG::ProgramBehaviorGraph::differentiate(ProgramBehaviorGraph& another_pbg, double anomaly_threshhold,
                                              double time_ratio, double ins_ratio, double var_ratio) {
    for (auto [rank, process_graph] : behavior_graph_) {
        for (auto node : process_graph) {
            auto fingerprint        = node->get_unique_key();
            auto bigger_total_time  = fingerprint2tot_time_.at(fingerprint);
            auto bigger_total_ins   = fingerprint2tot_ins_.at(fingerprint);
            auto smaller_total_time = another_pbg.get_fingerprint2tot_time().at(fingerprint);
            auto smaller_total_ins  = another_pbg.get_fingerprint2tot_ins().at(fingerprint);
            if (!fingerprint2time_var_.contains(fingerprint)) {
                fingerprint2time_var_[fingerprint] =
                    PBG::calculate_standard_deviation(fingerprint2time_.at(fingerprint));
            }
            if (!another_pbg.get_fingerprint2time_var().contains(fingerprint)) {
                another_pbg.get_fingerprint2time_var()[fingerprint] =
                    PBG::calculate_standard_deviation(another_pbg.get_fingerprint2time().at(fingerprint));
            }
            auto bigger_time_var  = fingerprint2time_var_.at(fingerprint);
            auto smaller_time_var = another_pbg.get_fingerprint2time_var().at(fingerprint);

            std::cout << "bigger_total_time: " << bigger_total_time << std::endl;
            std::cout << "bigger_total_ins: " << bigger_total_ins << std::endl;
            std::cout << "smaller_total_time: " << smaller_total_time << std::endl;
            std::cout << "smaller_total_ins: " << smaller_total_ins << std::endl;

            double anomaly_factor = time_ratio * (1.0 * bigger_total_time / smaller_total_time)
                                    + ins_ratio * (1.0 * bigger_total_ins / smaller_total_ins)
                                    + var_ratio * (bigger_time_var / smaller_time_var);
            std::cout << "Anomaly factor of node is " << anomaly_factor << std::endl;
            node->get_anomaly_factor() = anomaly_factor;
            if (anomaly_factor >= anomaly_threshhold) {
                candidate_anomaly_nodes_.insert(node);
            }
        }
    }
}

void PBG::ProgramBehaviorGraph::backtrack(double decay_factor, double merge_factor) {
    if (decay_factor < 0 || decay_factor > 1 || merge_factor < 0 || merge_factor > 1) {
        JSI_ERROR("Invalid decay_factor or merge_factor!\n");
    }
    // for (auto node : candidate_anomaly_nodes_) {

    // }
    while (candidate_anomaly_nodes_.size() != 0) {
        std::stack<BehaviorNode*> stack;

        auto current_node = *candidate_anomaly_nodes_.begin();
        stack.push(current_node);
        candidate_anomaly_nodes_.erase(candidate_anomaly_nodes_.begin());

        while (!stack.empty()) {
            auto node = stack.top();
            stack.pop();
            if (node->get_accumulated_anomaly_factor() == 0) {
                node->get_accumulated_anomaly_factor() += node->get_anomaly_factor();
            }
            node->get_visit_count() += 1;
            BehaviorNode* sync_dependent = node->get_sync_dependent();
            BehaviorNode* comm_dependent = node->get_comm_dependent();
            BehaviorNode* data_dependent = node->get_data_dependent();
            if (sync_dependent != nullptr) {
                sync_dependent->get_accumulated_anomaly_factor() +=
                    decay_factor * node->get_accumulated_anomaly_factor();
                if (candidate_anomaly_nodes_.contains(sync_dependent)) {
                    candidate_anomaly_nodes_.erase(sync_dependent);
                }
            }
            if (comm_dependent != nullptr) {
                if (comm_dependent->get_node_type() == PBG::NodeType::COLLECTIVE_NODE) {
                    comm_dependent->get_accumulated_anomaly_factor() +=
                        decay_factor * (1.0 - merge_factor) * node->get_accumulated_anomaly_factor();
                } else {
                    comm_dependent->get_accumulated_anomaly_factor() +=
                        decay_factor * node->get_accumulated_anomaly_factor();
                }
                stack.push(comm_dependent);
                if (candidate_anomaly_nodes_.contains(comm_dependent)) {
                    candidate_anomaly_nodes_.erase(comm_dependent);
                }
            }
            if (data_dependent != nullptr) {
                data_dependent->get_accumulated_anomaly_factor() +=
                    decay_factor * node->get_accumulated_anomaly_factor();
                stack.push(data_dependent);
                if (candidate_anomaly_nodes_.contains(data_dependent)) {
                    candidate_anomaly_nodes_.erase(data_dependent);
                }
            }
        }
    }
}

std::vector<PBG::BehaviorNode*> nodes;

void PBG::ProgramBehaviorGraph::sort_nodes() {
    size_t num_nodes = 0;
    for (const auto& pair : behavior_graph_) {
        num_nodes += pair.second.size();
    }
    nodes.reserve(num_nodes);
    for (const auto& pair : behavior_graph_) {
        nodes.insert(nodes.end(), pair.second.begin(), pair.second.end());
    }
    std::sort(nodes.begin(), nodes.end(), [](BehaviorNode* a, BehaviorNode* b) {
        auto a_anomaly_factor = (a->get_accumulated_anomaly_factor() - a->get_anomaly_factor()) / a->get_visit_count()
                                + a->get_anomaly_factor();
        auto b_anomaly_factor = (b->get_accumulated_anomaly_factor() - b->get_anomaly_factor()) / b->get_visit_count()
                                + b->get_anomaly_factor();
        return a_anomaly_factor > b_anomaly_factor;
    });
}

void PBG::ProgramBehaviorGraph::print_result(int top_k, std::ostream& file) {
    file << "Anomaly factor TOP - " << top_k << std::endl;
    for (int i = 0; i < top_k; ++i) {
        file << backtrace_trees[nodes[i]->str_id_]->backtrace_get_context_string(nodes[i]->get_backtrace())
             << std::endl;
        file << nodes[i]->get_accumulated_anomaly_factor() << "\n" << std::endl;
    }
}