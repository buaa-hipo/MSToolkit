//
// Created by xiaox on 2024/12/15.
//

#include <cstdio>

#define USE_WRITER

#ifndef USE_WRITER
#define ALLOCATE(size)        malloc(size)
#define DEALLOCATE(ptr, size) free(ptr)
int jsi_pmu_num = 4;
#else
#include "record/record_type.h"
#include "record/record_writer.h"
#endif

#include "instrument/MT_PMU_collector.h"
#include "instrument/backtrace.h"
#include "instrument/pmu_collector.h"
#include "record/accl_tracer.h"
#include "record/mt_callback_defs.h"
#include "utils/jsi_log.h"
#include "utils/tsc_timer.h"
#include "utils/safe.hpp"

#ifdef ENABLE_BACKTRACE
#define TRACER_INNER_BT_DEPTH 2
#endif

void write_to_file(void *rec, size_t size) {
#ifndef USE_WRITER
    auto file = fopen("./host_trace.tr", "ab");
    fwrite(rec, size, 1, file);
    fclose(file);
#else
    RecordWriter::traceStore((const record_t *)(rec));
#endif
}

void buffer_callback_impl(const char *begin, const char *end, void *args) {
    for (auto cur = reinterpret_cast<const accl_activity_record_t *>(begin);
         cur != reinterpret_cast<const accl_activity_record_t *>(end); accl_next_record(cur, &cur)) {
        JSI_WARN("Writing device activity trace\n");
        FILE *dev_trace = fopen("./dev_trace.etr", "ab");
        fwrite(cur, sizeof(accl_activity_record_t), 1, dev_trace);
        fclose(dev_trace);
        JSI_WARN("Written device activity trace\n");
    }
}

void api_callback_impl(uint32_t domain, uint32_t cid, const void *callback_data, void *arg) {
    jsi_safe_enter_instr();
    uint64_t ts   = get_tsc_raw();
    auto     data = static_cast<const matrix_api_data_t *>(callback_data);
    if (data->phase == ACCL_API_ENTER) {
#ifdef DEBUG
        JSI_WARN("In the enter callback of %s\n", accl_op_string(domain, cid, 0));
        fflush(stderr);
#endif
        uint64_t ri = data->correlation_id % tracer.ring_size_;
        if (!tracer.buffer_node_valid_[ri]) {
            tracer.ring_buffer_[ri].timestamp = ts;
            if (jsi_pmu_enabled) {
                pmu_collector_get_all(tracer.ring_buffer_[ri].pmu);
            }
            tracer.buffer_node_valid_[ri] = true;
        } else {
            JSI_ERROR("HOST TS BUFF RING COLLISION DETECTED. USE LARGER RING_SIZE!");
        }
    } else {
        size_t   ri       = data->correlation_id % tracer.ring_size_;
        uint64_t enter_ts = tracer.ring_buffer_[ri].timestamp;
        if (!tracer.buffer_node_valid_[ri]) {
            JSI_ERROR("The timestamp in the ring buffer is invalid!");
        } else {
            tracer.buffer_node_valid_[ri] = false;
        }
#ifdef ENABLE_BACKTRACE
        backtrace_context_t ctxt = 0;
        if (jsi_backtrace_enabled) {
            ctxt = backtrace_context_get(TRACER_INNER_BT_DEPTH);
        }
#endif
        switch (cid) {
            case ACCL_API_group_create_masked_launch:
            case ACCL_API_group_create_launch:
            case ACCL_API_group_exec:
            // todo: need to reconsider
            case ACCL_API_group_create:
            case ACCL_API_group_destroy: {
                auto *rec = static_cast<mt_record_kernel_launch_t *>(
                    ALLOCATE(sizeof(mt_record_kernel_launch_t) + sizeof(uint64_t) * 2 * jsi_pmu_num));
                if (jsi_pmu_enabled) {
                    uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
                    memcpy(counters, tracer.ring_buffer_[ri].pmu, jsi_pmu_num * sizeof(uint64_t));
                    pmu_collector_get_all(counters + jsi_pmu_num);
                }

                if (cid == ACCL_API_group_create_masked_launch) {
                    rec[0].record.MsgType = event_ACCL_API_group_create_masked_launch;
                } else if (cid == ACCL_API_group_create_launch) {
                    rec[0].record.MsgType = event_ACCL_API_group_create_launch;
                } else if (cid == ACCL_API_group_exec) {
                    rec[0].record.MsgType = event_ACCL_API_group_exec;
                } else if (cid == ACCL_API_group_create) {
                    rec[0].record.MsgType = event_ACCL_API_group_create;
                } else if (cid == ACCL_API_group_destroy) {
                    rec[0].record.MsgType = event_ACCL_API_group_destroy;
                }
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].correlation_id          = data->correlation_id;
                rec[0].cluster_id              = data->kernel_info.cluster_id;
                rec[0].thread_num              = data->kernel_info.thread_num;
                rec[0].thread_mask             = data->kernel_info.thread_mask;
                rec[0].scalar_args_num         = data->kernel_info.scalar_args_num;
                rec[0].ptr_args_num            = data->kernel_info.ptr_args_num;
                rec[0].group_id                = data->kernel_info.thread_group_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;

                // FILE *host_trace = fopen("./host_trace.tr", "ab");
                // fwrite(rec, sizeof(mt_record_kernel_launch_t), 1, host_trace);
                // fclose(host_trace);
                RecordWriter::traceStore((const record_t *)(rec));

                uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
#ifdef DEBUG
                printf("PMU reads of kernel launch:\n");
                for (int i = 0; i < 2 * jsi_pmu_num; ++i) {
                    printf("%lu ", counters[i]);
                }
                printf("\n");
#endif
#ifdef DEBUG
                printf("\nKernel Launch:\n");
                printf("MsgType: %d\n", rec[0].record.MsgType);
#ifdef ENABLE_BACKTRACE
                printf("BT context: %lu\n", rec[0].record.ctxt);
#endif
                printf("TS: %lu, %lu\n", rec[0].record.timestamps.enter, rec[0].record.timestamps.exit);
                printf("Correlation ID: %lu\n", rec[0].correlation_id);
                printf("cluster_id: %u\n", rec[0].cluster_id);
                printf("thread_num: %u\n", rec[0].thread_num);
                printf("group_id: %u\n", rec[0].group_id);
                printf("thread_mask: %u\n", rec[0].thread_mask);
                printf("scalar_args_num: %u\n", rec[0].scalar_args_num);
                printf("ptr_args_num: %u\n", rec[0].ptr_args_num);
#endif

                DEALLOCATE(rec, sizeof(mt_record_kernel_launch_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
                break;
            }

            case ACCL_API_group_wait: {
                auto *rec = static_cast<mt_record_group_wait_t *>(
                    ALLOCATE(sizeof(mt_record_group_wait_t) + sizeof(uint64_t) * 2 * jsi_pmu_num));
                if (jsi_pmu_enabled) {
                    uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
                    memcpy(counters, tracer.ring_buffer_[ri].pmu, jsi_pmu_num * sizeof(uint64_t));
                    pmu_collector_get_all(counters + jsi_pmu_num);
                }

                rec[0].record.MsgType = event_ACCL_API_group_wait;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].correlation_id          = data->correlation_id;
                rec[0].group_id                = data->kernel_info.thread_group_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;

                RecordWriter::traceStore((const record_t *)(rec));
#ifdef DEBUG
                std::cout << "\nGroup wait: " << std::endl;
                std::cout << "Correlation ID: " << rec[0].correlation_id << std::endl;
                std::cout << "Thread group ID: " << rec[0].group_id << std::endl;
                std::cout << "Enter TS: " << rec[0].record.timestamps.enter << std::endl;
                std::cout << "Exit TS: " << rec[0].record.timestamps.exit << std::endl;
#endif

                DEALLOCATE(rec, sizeof(mt_record_group_wait_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
                break;
            }

            case ACCL_API_malloc: {
                auto *rec = static_cast<mt_record_malloc_t *>(
                    ALLOCATE(sizeof(mt_record_malloc_t) + sizeof(uint64_t) * 2 * jsi_pmu_num));
                if (jsi_pmu_enabled) {
                    uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
                    memcpy(counters, tracer.ring_buffer_[ri].pmu, jsi_pmu_num * sizeof(uint64_t));
                    pmu_collector_get_all(counters + jsi_pmu_num);
                }

                rec[0].record.MsgType = event_ACCL_API_malloc;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].correlation_id          = data->correlation_id;
                rec[0].cluster_id              = data->mem_alloc.cluster_id;
                rec[0].bytes                   = data->mem_alloc.bytes;
                rec[0].mode                    = data->mem_alloc.mode;
                rec[0].kind                    = data->mem_alloc.kind;
                rec[0].address = data->mem_alloc.address;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;

                RecordWriter::traceStore((const record_t *)(rec));

#ifdef DEBUG
                std::cout << "\nDDR Malloc: " << std::endl;
                std::cout << "Correlation ID: " << rec[0].correlation_id << std::endl;
                std::cout << "Cluster ID: " << rec[0].cluster_id << std::endl;
                std::cout << "Bytes: " << rec[0].bytes << std::endl;
                std::cout << "Mode: " << rec[0].mode << std::endl;
                std::cout << "Kind: " << rec[0].kind << std::endl;
                std::cout << "Enter TS: " << rec[0].record.timestamps.enter << std::endl;
                std::cout << "Exit TS: " << rec[0].record.timestamps.exit << std::endl;
#endif

                DEALLOCATE(rec, sizeof(mt_record_malloc_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
                break;
            }

            case ACCL_API_free: {
                auto *rec = static_cast<mt_record_free_t *>(
                    ALLOCATE(sizeof(mt_record_free_t) + sizeof(uint64_t) * 2 * jsi_pmu_num));
                if (jsi_pmu_enabled) {
                    uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
                    memcpy(counters, tracer.ring_buffer_[ri].pmu, jsi_pmu_num * sizeof(uint64_t));
                    pmu_collector_get_all(counters + jsi_pmu_num);
                }

                rec[0].record.MsgType = event_ACCL_API_free;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].correlation_id          = data->correlation_id;
                rec[0].address                 = data->mem_alloc.address;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;

                RecordWriter::traceStore((const record_t *)(rec));

#ifdef DEBUG
                std::cout << "\nDDR Malloc: " << std::endl;
                std::cout << "Correlation ID: " << rec[0].correlation_id << std::endl;
                std::cout << "Cluster ID: " << rec[0].cluster_id << std::endl;
                std::cout << "Bytes: " << rec[0].bytes << std::endl;
                std::cout << "Mode: " << rec[0].mode << std::endl;
                std::cout << "Kind: " << rec[0].kind << std::endl;
                std::cout << "Enter TS: " << rec[0].record.timestamps.enter << std::endl;
                std::cout << "Exit TS: " << rec[0].record.timestamps.exit << std::endl;
#endif
                DEALLOCATE(rec, sizeof(mt_record_free_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
                break;
            }

            case ACCL_API_dat_load:
            case ACCL_API_dat_unload:
            case ACCL_API_dev_open:
            case ACCL_API_dev_close: {
                auto *rec = static_cast<mt_record_driver_t *>(
                    ALLOCATE(sizeof(mt_record_driver_t) + sizeof(uint64_t) * 2 * jsi_pmu_num));
                if (jsi_pmu_enabled) {
                    uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
                    memcpy(counters, tracer.ring_buffer_[ri].pmu, jsi_pmu_num * sizeof(uint64_t));
                    pmu_collector_get_all(counters + jsi_pmu_num);
                }

                if (cid == ACCL_API_dat_load) {
                    rec[0].record.MsgType = event_ACCL_API_dat_load;
                } else if (cid == ACCL_API_dat_unload) {
                    rec[0].record.MsgType = event_ACCL_API_dat_unload;
                } else if (cid == ACCL_API_dev_open) {
                    rec[0].record.MsgType = event_ACCL_API_dev_open;
                } else if (cid == ACCL_API_dev_close) {
                    rec[0].record.MsgType = event_ACCL_API_dev_close;
                }
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].correlation_id          = data->correlation_id;
                rec[0].cluster_id              = data->dev.cluster_id;
                rec[0].kind                    = data->dev.kind;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;

                RecordWriter::traceStore((const record_t *)(rec));

#ifdef DEBUG
                std::cout << "\nDefault: " << std::endl;
                std::cout << "Correlation ID: " << rec[0].correlation_id << std::endl;
                std::cout << "Enter TS: " << rec[0].record.timestamps.enter << std::endl;
                std::cout << "Exit TS: " << rec[0].record.timestamps.exit << std::endl;
#endif
                DEALLOCATE(rec, sizeof(mt_record_driver_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
                break;
            }

            case ACCL_API_barrier_create:
            case ACCL_API_barrier_destroy: {
                auto *rec = static_cast<mt_record_barrier_t *>(
                    ALLOCATE(sizeof(mt_record_barrier_t) + sizeof(uint64_t) * 2 * jsi_pmu_num));
                if (jsi_pmu_enabled) {
                    uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
                    memcpy(counters, tracer.ring_buffer_[ri].pmu, jsi_pmu_num * sizeof(uint64_t));
                    pmu_collector_get_all(counters + jsi_pmu_num);
                }

                if (cid == ACCL_API_barrier_create) {
                    rec[0].record.MsgType = event_ACCL_API_barrier_create;
                } else if (cid == ACCL_API_barrier_destroy) {
                    rec[0].record.MsgType = event_ACCL_API_barrier_destroy;
                }
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].correlation_id          = data->correlation_id;
                rec[0].cluster_id              = data->barrier.cluster_id;
                rec[0].kind                    = data->barrier.kind;
                rec[0].barrier_id              = data->barrier.barrier_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;

                RecordWriter::traceStore((const record_t *)(rec));

#ifdef DEBUG
                std::cout << "\nDefault: " << std::endl;
                std::cout << "Correlation ID: " << rec[0].correlation_id << std::endl;
                std::cout << "Enter TS: " << rec[0].record.timestamps.enter << std::endl;
                std::cout << "Exit TS: " << rec[0].record.timestamps.exit << std::endl;
#endif
                DEALLOCATE(rec, sizeof(mt_record_barrier_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
                break;
            }

            case ACCL_API_rwlock_create:
            case ACCL_API_rwlock_destroy: {
                auto *rec = static_cast<mt_record_rwlock_t *>(
                    ALLOCATE(sizeof(mt_record_rwlock_t) + sizeof(uint64_t) * 2 * jsi_pmu_num));
                if (jsi_pmu_enabled) {
                    uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
                    memcpy(counters, tracer.ring_buffer_[ri].pmu, jsi_pmu_num * sizeof(uint64_t));
                    pmu_collector_get_all(counters + jsi_pmu_num);
                }

                if (cid == ACCL_API_rwlock_create) {
                    rec[0].record.MsgType = event_ACCL_API_rwlock_create;
                } else if (cid == ACCL_API_rwlock_destroy) {
                    rec[0].record.MsgType = event_ACCL_API_rwlock_destroy;
                }
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].correlation_id          = data->correlation_id;
                rec[0].cluster_id              = data->rw_lock.cluster_id;
                rec[0].kind                    = data->rw_lock.kind;
                rec[0].lock_id                 = data->rw_lock.lock_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;

                RecordWriter::traceStore((const record_t *)(rec));

#ifdef DEBUG
                std::cout << "\nDefault: " << std::endl;
                std::cout << "Correlation ID: " << rec[0].correlation_id << std::endl;
                std::cout << "Enter TS: " << rec[0].record.timestamps.enter << std::endl;
                std::cout << "Exit TS: " << rec[0].record.timestamps.exit << std::endl;
#endif
                DEALLOCATE(rec, sizeof(mt_record_rwlock_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
                break;
            }

            case ACCL_API_intr_send:
            case ACCL_API_intr_reg: {
                auto *rec = static_cast<mt_record_intr_t *>(
                    ALLOCATE(sizeof(mt_record_intr_t) + sizeof(uint64_t) * 2 * jsi_pmu_num));
                if (jsi_pmu_enabled) {
                    uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
                    memcpy(counters, tracer.ring_buffer_[ri].pmu, jsi_pmu_num * sizeof(uint64_t));
                    pmu_collector_get_all(counters + jsi_pmu_num);
                }

                if (cid == ACCL_API_intr_send) {
                    rec[0].record.MsgType = event_ACCL_API_intr_send;
                } else if (cid == ACCL_API_intr_reg) {
                    rec[0].record.MsgType = event_ACCL_API_intr_reg;
                }
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].correlation_id          = data->correlation_id;
                rec[0].group_id                = data->intr.group_id;
                rec[0].thread_id               = data->intr.thread_id;
                rec[0].intr_id                 = data->intr.intr_id;
                rec[0].func                    = data->intr.func;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;

                RecordWriter::traceStore((const record_t *)(rec));

#ifdef DEBUG
                std::cout << "\nDefault: " << std::endl;
                std::cout << "Correlation ID: " << rec[0].correlation_id << std::endl;
                std::cout << "Enter TS: " << rec[0].record.timestamps.enter << std::endl;
                std::cout << "Exit TS: " << rec[0].record.timestamps.exit << std::endl;
#endif
                DEALLOCATE(rec, sizeof(mt_record_intr_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
                break;
            }

            case ACCL_API_dev_owner:
            case ACCL_API_group_get_status: {
                auto *rec = static_cast<record_activity_t *>(
                    ALLOCATE(sizeof(record_activity_t) + sizeof(uint64_t) * 2 * jsi_pmu_num));
                if (jsi_pmu_enabled) {
                    uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
                    memcpy(counters, tracer.ring_buffer_[ri].pmu, jsi_pmu_num * sizeof(uint64_t));
                    pmu_collector_get_all(counters + jsi_pmu_num);
                }

                if (cid == ACCL_API_dev_owner) {
                    rec[0].record.MsgType = event_ACCL_API_dev_owner;
                } else if (cid == ACCL_API_group_get_status) {
                    rec[0].record.MsgType = event_ACCL_API_group_get_status;
                }
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].correlation_id          = data->correlation_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;

                RecordWriter::traceStore((const record_t *)(rec));

#ifdef DEBUG
                std::cout << "\nDefault: " << std::endl;
                std::cout << "Correlation ID: " << rec[0].correlation_id << std::endl;
                std::cout << "Enter TS: " << rec[0].record.timestamps.enter << std::endl;
                std::cout << "Exit TS: " << rec[0].record.timestamps.exit << std::endl;
#endif
                DEALLOCATE(rec, sizeof(record_activity_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
                break;
            }

            default: {
                auto *rec = static_cast<record_activity_t *>(
                    ALLOCATE(sizeof(record_activity_t) + sizeof(uint64_t) * 2 * jsi_pmu_num));
                if (jsi_pmu_enabled) {
                    uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
                    memcpy(counters, tracer.ring_buffer_[ri].pmu, jsi_pmu_num * sizeof(uint64_t));
                    pmu_collector_get_all(counters + jsi_pmu_num);
                }

                rec[0].record.MsgType = event_ACCL_API_UNKNOWN;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].correlation_id          = data->correlation_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;

                RecordWriter::traceStore((const record_t *)(rec));

#ifdef DEBUG
                std::cout << "\nDefault: " << std::endl;
                std::cout << "Correlation ID: " << rec[0].correlation_id << std::endl;
                std::cout << "Enter TS: " << rec[0].record.timestamps.enter << std::endl;
                std::cout << "Exit TS: " << rec[0].record.timestamps.exit << std::endl;
#endif
                DEALLOCATE(rec, sizeof(record_activity_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
            }
        }
    }
    jsi_safe_exit_instr();
}

__attribute__((constructor)) void tracer_init() {
    JSI_INFO("ACCL tracer initialization start\n");
    const char              *events_str = getenv("JSI_COLLECT_DEV_PMU_EVENT");
    std::vector<std::string> event_list;
    parse_dev_pmu_events_list(events_str, &event_list);
    if (event_list.size() == 0) {
        JSI_WARN("No device pmu event specified!\n");
    }
    int code;
    for (const auto &name : event_list) {
        if (PMU_event_name_to_code(name.c_str(), &code) != PMU_OK) {
            JSI_ERROR("PMU event %s not found!\n", name.c_str());
        }
        tracer.device_pmu_events.emplace_back(code);
    }
    RecordWriter::metaSectionStart("ACCL TRACE META");
    RecordWriter::metaStore<std::string>("ACCL_DEVICE_TYPE", "MATRIX");
    RecordWriter::metaStore<int>("ACCL_PMU_NUM_EVENTS", (int)tracer.device_pmu_events.size());
    if (events_str != NULL) RecordWriter::metaStore("ACCL_PMU_EVENT_LIST", events_str);
    RecordWriter::metaSectionEnd("ACCL TRACE META");

    tracer.RegisterCallback(api_callback_impl);
    JSI_INFO("Accl tracer initialized\n");
}
