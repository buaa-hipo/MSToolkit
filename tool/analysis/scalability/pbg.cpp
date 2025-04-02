#include "pbg.hpp"

#include <iostream>

namespace PBG {

uint64_t encode(record_t *rec, BacktraceTree *bt) {
    // printf("Encoding %lu %p: %s\n", rec->ctxt, bt, RecordHelper::dump_string(rec).c_str());
    backtrace_context_t ctxt = rec->ctxt;
    std::string         bt_str(bt->backtrace_get_context_string(ctxt));
    while (bt_str.find("libmpi_wrapper.so") != std::string::npos) {
        ctxt = bt->get_parent(ctxt);
        if (ctxt != BACKTRACE_UNKNOWN_NODE) {
            bt_str = std::string(bt->backtrace_get_context_string(ctxt));
        } else {
            JSI_ERROR("May be corrupted backtrace!\n");
        }
    }
    backtrace_context_t pctxt = bt->get_parent(ctxt);
    return ctxt ^ pctxt;
}

std::string encode_comm_info(int dest, int tag) {
    return std::to_string(dest) + std::string("/") + std::to_string(tag);
}

}  // namespace PBG

static bool is_hip_kernel_launch(record_t *record) {
    return RecordHelper::is_event(record, event_hipLaunchKernel)
           || RecordHelper::is_event(record, event_hipExtLaunchKernel)
           || RecordHelper::is_event(record, event_hipExtLaunchMultiKernelMultiDevice)
           || RecordHelper::is_event(record, event_hipLaunchCooperativeKernelMultiDevice)
           || RecordHelper::is_event(record, event_hipExtModuleLaunchKernel)
           || RecordHelper::is_event(record, event_hipHccModuleLaunchKernel)
           || RecordHelper::is_event(record, event_hipLaunchCooperativeKernel)
           || RecordHelper::is_event(record, event_hipLaunchKernel_internal)
           || RecordHelper::is_event(record, event_hipModuleLaunchCooperativeKernelMultiDevice)
           || RecordHelper::is_event(record, event_hipModuleLaunchKernel)
           || RecordHelper::is_event(record, event_hipModuleLaunchCooperativeKernel);
}

static bool is_mt_kernel_launch(record_t *record) {
    return RecordHelper::is_event(record, event_ACCL_API_group_create_launch)
           || RecordHelper::is_event(record, event_ACCL_API_group_create_masked_launch)
           || RecordHelper::is_event(record, event_ACCL_API_group_exec);
}

static bool is_hip_memcpy_async(record_t *record) {
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

// {<unique backtrace key>, node}
std::unordered_map<uint64_t, PBG::BehaviorNode *> node_map;

// {<dest/tag>, <nodes>}
std::unordered_map<std::string, std::deque<PBG::BehaviorNode *>> senders_map;
std::unordered_map<std::string, std::deque<PBG::BehaviorNode *>> receivers_map;

// {<comm>, <nodes>}
std::unordered_map<uint64_t, PBG::BehaviorNode *> collective_senders_map;
std::unordered_map<uint64_t, PBG::BehaviorNode *> collective_receivers_map;

// {request, <nodes>}
std::unordered_map<uint64_t, PBG::BehaviorNode *> mpi_waiters_map;

// {<stream or event pointer>, <nodes>}
std::unordered_map<uint64_t, PBG::BehaviorNode *> hip_waiters_map;
std::unordered_map<uint64_t, PBG::BehaviorNode *> mt_waiters_map;

/*
    For intra-process dependency, add sync dependent to the wait node from map.
    For inter-process p2p send-receive dependency, receivers find their senders from senders_map by encoded dest/tag.
    For inter-process collective send-receive dependency:
        [Gather/]
*/
std::vector<PBG::BehaviorNode *> build_graph_for_processes(RecordTraceCollection &traces, RankMetaCollection &metas,
                                                           BacktraceCollection &backtraces) {
    std::vector<PBG::BehaviorNode *> roots(traces.size());
    for (auto trace_it = traces.begin(); trace_it != traces.end(); ++trace_it) {
        int   rank       = trace_it->first;
        auto &trace      = *(trace_it->second);
        auto  num_events = trace.num_pmu_events();
        auto  backtrace  = backtraces[rank];

        node_map.clear();

        PBG::BehaviorNode *father = nullptr;

        for (auto record_it = trace.begin(); record_it != trace.end(); record_it = record_it.next()) {
            auto record     = record_it.val();
            auto unique_key = PBG::encode(record, backtrace);

            PBG::BehaviorNode *node = nullptr;
            if (node_map.contains(unique_key)) {
                // Dependency should have been dealt with.
                father = node_map[unique_key];
                continue;
            }
            if (RecordHelper::is_event(record, event_MPI_Recv)) {
                node = new PBG::BehaviorNode(unique_key, record->timestamps.enter, record->timestamps.exit, 0.0,
                                             PBG::NodeType::COMM_NODE);
                auto comm_record = reinterpret_cast<record_comm_t *>(record);
                auto info        = PBG::encode_comm_info(comm_record->dest, comm_record->tag);
                receivers_map[info].push_back(node);
            } else if (RecordHelper::is_event(record, event_MPI_Send)) {
                node = new PBG::BehaviorNode(unique_key, record->timestamps.enter, record->timestamps.exit, 0.0,
                                             PBG::NodeType::COMM_NODE);
                auto comm_record = reinterpret_cast<record_comm_t *>(record);
                auto info        = PBG::encode_comm_info(comm_record->dest, comm_record->tag);
                senders_map[info].push_back(node);
            } else if (RecordHelper::is_event(record, event_MPI_Isend)) {
                node = new PBG::BehaviorNode(unique_key, record->timestamps.enter, record->timestamps.exit, 0.0,
                                             PBG::NodeType::COMM_NODE);
                auto comm_record = reinterpret_cast<record_comm_async_t *>(record);
                auto info        = PBG::encode_comm_info(comm_record->dest, comm_record->tag);
                senders_map[info].push_back(node);
                mpi_waiters_map[static_cast<uint64_t>(comm_record->request)] = node;
            } else if (RecordHelper::is_event(record, event_MPI_Irecv)) {
                node = new PBG::BehaviorNode(unique_key, record->timestamps.enter, record->timestamps.exit, 0.0,
                                             PBG::NodeType::COMM_NODE);
                auto comm_record = reinterpret_cast<record_comm_async_t *>(record);
                auto info        = PBG::encode_comm_info(comm_record->dest, comm_record->tag);
                receivers_map[info].push_back(node);
                mpi_waiters_map[static_cast<uint64_t>(comm_record->request)] = node;
            } else if (RecordHelper::is_event(record, event_MPI_Wait)) {
                node = new PBG::BehaviorNode(unique_key, record->timestamps.enter, record->timestamps.exit, 0.0,
                                             PBG::NodeType::SYNC_NODE);
                auto comm_record = reinterpret_cast<record_comm_wait_t *>(record);
                auto request     = comm_record->request;
                if (mpi_waiters_map.contains(request)) {
                    auto dependent = mpi_waiters_map.at(request);
                    node->add_sync_dependent(dependent);
                    mpi_waiters_map.erase(request);
                } else {
                    std::cout << "Unable to find synchronize dependent for MPI_Wait" << std::endl;
                }
            } else if (is_mt_kernel_launch(record)) {
                node = new PBG::BehaviorNode(unique_key, record->timestamps.enter, record->timestamps.exit, 0.0,
                                             PBG::NodeType::SIMPLE_NODE);

                auto kernel_launch_record = reinterpret_cast<mt_record_kernel_launch_t *>(record);
                mt_waiters_map.insert_or_assign(kernel_launch_record->group_id, node);
            } else if (RecordHelper::is_event(record, event_ACCL_API_group_wait)) {
                node = new PBG::BehaviorNode(unique_key, record->timestamps.enter, record->timestamps.exit, 0.0,
                                             PBG::NodeType::SYNC_NODE);
                auto group_wait_record = reinterpret_cast<mt_record_group_wait_t *>(record);
                auto thread_group_id   = group_wait_record->group_id;
                if (mt_waiters_map.contains(thread_group_id)) {
                    node->add_sync_dependent(mt_waiters_map.at(thread_group_id));
                    mt_waiters_map.erase(thread_group_id);
                } else {
                    std::cout << "Unable to find synchronize dependent for hthread_group_wait" << std::endl;
                }
            } else if (is_hip_kernel_launch(record)) {
                node = new PBG::BehaviorNode(unique_key, record->timestamps.enter, record->timestamps.exit, 0.0,
                                             PBG::NodeType::SIMPLE_NODE);
                auto kernel_launch_record = reinterpret_cast<record_activity_launch_t *>(record);
                auto stream               = kernel_launch_record->stream;
                hip_waiters_map.insert_or_assign(reinterpret_cast<uint64_t>(stream), node);
            } else if (is_hip_memcpy_async(record)) {
                node = new PBG::BehaviorNode(unique_key, record->timestamps.enter, record->timestamps.exit, 0.0,
                                             PBG::NodeType::SIMPLE_NODE);
                auto memcpy_async_record = reinterpret_cast<record_activity_memcpy_async_t *>(record);
                auto stream              = memcpy_async_record->stream;
                hip_waiters_map.insert_or_assign(reinterpret_cast<uint64_t>(stream), node);
            } else if (RecordHelper::is_event(record, event_hipEventRecord)) {
                node = new PBG::BehaviorNode(unique_key, record->timestamps.enter, record->timestamps.exit, 0.0,
                                             PBG::NodeType::SIMPLE_NODE);
                auto event_record = reinterpret_cast<record_activity_event_t *>(record);
                auto event        = event_record->event;
                hip_waiters_map.insert_or_assign(reinterpret_cast<uint64_t>(event), node);
            } else if (RecordHelper::is_event(record, event_hipEventSynchronize)) {
                node = new PBG::BehaviorNode(unique_key, record->timestamps.enter, record->timestamps.exit, 0.0,
                                             PBG::NodeType::SYNC_NODE);
                auto event_record = reinterpret_cast<record_activity_event_t *>(record);
                auto event        = event_record->event;
                if (hip_waiters_map.contains(reinterpret_cast<uint64_t>(event))) {
                    node->add_sync_dependent(hip_waiters_map.at(reinterpret_cast<uint64_t>(event)));
                    hip_waiters_map.erase(reinterpret_cast<uint64_t>(event));
                }
            }

            node_map[unique_key] = node;
            if (node != nullptr && father != nullptr) {
                node->add_data_dependent(father);
            }
            father = node;
        }
    }
}
