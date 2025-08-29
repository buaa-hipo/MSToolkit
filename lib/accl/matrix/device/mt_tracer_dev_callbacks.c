
#include <stdint.h>

#include "hthread_device.h"
#include "record/mt_buffer.h"
#include "record/mt_callback_defs.h"
#include "record/mt_dev_types.h"
#include "record/wrap_defines_macro.h"
#include "instrument/MT_PMU_collector.h"

/// todo: pass event ids and length from host
uint32_t *pmu_events;
uint32_t  pmu_num;

extern bool enable_pmu;

typedef uint32_t bt_type;

int32_t bt_get() {
    return -1;
}

#define PRINT(...)

#define EXIT_GET_PMU                                                     \
    {                                                                    \
        uint64_t *counters = (uint64_t *)(rec + 1);                      \
        for (int i = 0; i < pmu_num; ++i) {                              \
            counters[i] = data->BufferNode.pmu[pmu_events[i]];           \
        }                                                                \
        simple_pmu_read(data->BufferNode.pmu);                           \
        for (int i = 0; i < pmu_num; ++i) {                              \
            counters[pmu_num + i] = data->BufferNode.pmu[pmu_events[i]]; \
        }                                                                \
    }                                                                    \
    while (0);

void dev_api_callback_impl(uint32_t domain, uint32_t cid, void *callback_data, void *arg) {
    matrix_api_data_t *data = (matrix_api_data_t *)(callback_data);

    uint64_t ts = get_clk();  // todo: not timestamp
    if (data->phase == ACCL_API_ENTER) {
        data->BufferNode.timestamp = ts;
        if (enable_pmu) {
            simple_pmu_read(data->BufferNode.pmu);
        }
    } else {
        uint64_t enter_ts = data->BufferNode.timestamp;  // todo: not timestamp
#ifdef ENABLE_BACKTRACE
        bt_type ctxt;
        ctxt = bt_get();
#endif
        if (domain == ACCL_DOMAIN_HTHREAD_OPS) {
            mt_record_kernel_t *rec;
            if (enable_pmu)
                rec =
                    (mt_record_kernel_t *)cb_buffer_alloc(sizeof(mt_record_kernel_t) + sizeof(uint64_t) * 2 * pmu_num);
            else
                rec = (mt_record_kernel_t *)cb_buffer_alloc(sizeof(mt_record_kernel_t));

            if (rec == NULL) {
                PRINT("KERNEL RECORD DISCARDED!\n");
                return;
            }
            if (enable_pmu) {
                EXIT_GET_PMU
            }

            rec[0].correlation_id          = data->correlation_id;
            rec[0].record.MsgType          = event_ACCL_ACTIVITY_kernel;
            rec[0].record.timestamps.enter = enter_ts;
            rec[0].record.timestamps.exit  = ts;
#ifdef ENABLE_BACKTRACE
            rec[0].record.ctxt = ctxt;
#endif
            rec[0].domain = ACCL_DOMAIN_HTHREAD_OPS;
            rec[0].op     = data->kernel.op;
            rec[0].kind   = data->kernel.kind;
#ifdef DEBUG
            PRINT("Kernel:\n");
            PRINT("Message type: %u\n", rec[0].record.MsgType);
            PRINT("Correlation ID: %llu\n", rec[0].correlation_id);
            PRINT("Enter TS: %llu\n", rec[0].record.timestamps.enter);
            PRINT("Exit TS: %llu\n", rec[0].record.timestamps.exit);
            PRINT("DOMAIN-OP-KIND: %u-%u-%u\n", rec[0].domain, rec[0].op, rec[0].kind);
#endif
            return;
        }

        return;

        switch (cid) {
            case ACCL_USER_FUNC: {
                record_activity_t *rec;
                if (enable_pmu)
                    rec = (record_activity_t *)cb_buffer_alloc(sizeof(record_activity_t)
                                                               + sizeof(uint64_t) * 2 * pmu_num);
                else
                    rec = (record_activity_t *)cb_buffer_alloc(sizeof(record_activity_t));
                if (rec == NULL) {
                    PRINT("USER FUNCTION RECORD DISCARDED!\n");
                    return;
                }
                rec->record.MsgType          = event_ACCL_USER_FUNC;
                rec->record.timestamps.enter = ts;
                rec->record.timestamps.exit  = 0;
                rec[0].correlation_id        = data->correlation_id;
                if (enable_pmu) {
                    EXIT_GET_PMU
                }

                rec[0].correlation_id = data->correlation_id;
                rec[0].record.MsgType = event_ACCL_USER_FUNC;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef DEBUG
                PRINT("USER FUNCTION:\n");
                PRINT("Correlation ID: %llu\n", rec[0].correlation_id);
                PRINT("Enter TS: %llu\n", rec[0].record.timestamps.enter);
                PRINT("Exit TS: %llu\n", rec[0].record.timestamps.exit);
#endif
                break;
            }
            case ACCL_API_vector_load:
            case ACCL_API_vector_store:
            case ACCL_API_scalar_load:
            case ACCL_API_scalar_store: {
                mt_record_memcpy_t *rec;
                if (enable_pmu)
                    rec = (mt_record_memcpy_t *)cb_buffer_alloc(sizeof(mt_record_memcpy_t)
                                                                + sizeof(uint64_t) * 2 * pmu_num);
                else
                    rec = (mt_record_memcpy_t *)cb_buffer_alloc(sizeof(mt_record_memcpy_t));
                if (rec == NULL) {
                    PRINT("MEMCPY RECORD DISCARDED!\n");
                    return;
                }
                if (enable_pmu) {
                    EXIT_GET_PMU
                }
                if (cid == ACCL_API_scalar_load)
                    rec[0].record.MsgType = event_ACCL_API_scalar_load;
                else if (cid == ACCL_API_scalar_store)
                    rec[0].record.MsgType = event_ACCL_API_scalar_store;
                else if (cid == ACCL_API_vector_load)
                    rec[0].record.MsgType = event_ACCL_API_vector_load;
                else if (cid == ACCL_API_vector_store)
                    rec[0].record.MsgType = event_ACCL_API_vector_store;

                rec[0].correlation_id          = data->correlation_id;
                rec[0].src                     = data->mem_sync.src;
                rec[0].dst                     = data->mem_sync.dst;
                rec[0].bytes                   = data->mem_sync.size_bytes;
                rec[0].kind                    = data->mem_sync.kind;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
#ifdef DEBUG
                PRINT("\nMemcpy:\n");
                PRINT("Correlation ID: %llu\n", rec[0].correlation_id);
                PRINT("SRC: %p\n", rec[0].src);
                PRINT("DST: %p\n", rec[0].dst);
                PRINT("Bytes: %d\n", rec[0].bytes);
                PRINT("Kind: %p\n", rec[0].kind);
                PRINT("Enter TS: %llu\n", rec[0].record.timestamps.enter);
                PRINT("Exit TS: %llu\n", rec[0].record.timestamps.exit);
#endif
                break;
            }

            case ACCL_API_scalar_load_async:
            case ACCL_API_scalar_store_async:
            case ACCL_API_vector_load_async:
            case ACCL_API_vector_store_async:
            case ACCL_API_dma_p2p:
            case ACCL_API_dma_broadcast:
            case ACCL_API_dma_segment:
            case ACCL_API_dma_sg: {
                mt_record_memcpy_async_t *rec;
                if (enable_pmu)
                    rec = (mt_record_memcpy_async_t *)cb_buffer_alloc(sizeof(mt_record_memcpy_async_t)
                                                                      + sizeof(uint64_t) * 2 * pmu_num);
                else
                    rec = (mt_record_memcpy_async_t *)cb_buffer_alloc(sizeof(mt_record_memcpy_async_t));
                if (rec == NULL) {
                    PRINT("MEMCPY ASYNC RECORD DISCARDED!\n");
                    return;
                }
                if (enable_pmu) {
                    EXIT_GET_PMU
                }

                if (cid == ACCL_API_scalar_load_async)
                    rec[0].record.MsgType = event_ACCL_API_scalar_load_async;
                else if (cid == ACCL_API_scalar_store_async)
                    rec[0].record.MsgType = event_ACCL_API_scalar_store_async;
                else if (cid == ACCL_API_vector_load_async)
                    rec[0].record.MsgType = event_ACCL_API_vector_load_async;
                else if (cid == ACCL_API_vector_store_async)
                    rec[0].record.MsgType = event_ACCL_API_vector_store_async;
                else if (cid == ACCL_API_dma_p2p)
                    rec[0].record.MsgType = event_ACCL_API_dma_p2p;
                else if (cid == ACCL_API_dma_broadcast)
                    rec[0].record.MsgType = event_ACCL_API_dma_broadcast;
                else if (cid == ACCL_API_dma_segment)
                    rec[0].record.MsgType = event_ACCL_API_dma_segment;
                else if (cid == ACCL_API_dma_sg)
                    rec[0].record.MsgType = event_ACCL_API_dma_sg;
                // #ifdef ENABLE_BACKTRACE
                //                 rec[0].record.ctxt = ctxt;
                // #endif
                rec[0].correlation_id          = data->correlation_id;
                rec[0].src                     = data->mem_async.src;
                rec[0].dst                     = data->mem_async.dst;
                rec[0].bytes                   = data->mem_async.size_bytes;
                rec[0].kind                    = data->mem_async.kind;
                rec[0].dma_channel             = data->mem_async.dma_channel;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
#ifdef DEBUG
                PRINT("\nMemcpy Async:\n");
                PRINT("Correlation ID: %llu\n", rec[0].correlation_id);
                PRINT("SRC: %p\n", rec[0].src);
                PRINT("DST: %p\n", rec[0].dst);
                PRINT("Bytes: %d\n", rec[0].bytes);
                PRINT("Kind: %p\n", rec[0].kind);
                PRINT("DMA Channel: %d\n", rec[0].dma_channel);
                PRINT("Enter TS: %llu\n", rec[0].record.timestamps.enter);
                PRINT("Exit TS: %llu\n", rec[0].record.timestamps.exit);
#endif
                break;
            }

            case ACCL_API_vector_malloc:
            case ACCL_API_scalar_malloc:
            case ACCL_API_hbm_malloc: {
                mt_record_malloc_t *rec;
                if (enable_pmu)
                    rec = (mt_record_malloc_t *)cb_buffer_alloc(sizeof(mt_record_malloc_t)
                                                                + sizeof(uint64_t) * 2 * pmu_num);
                else
                    rec = (mt_record_malloc_t *)cb_buffer_alloc(sizeof(mt_record_malloc_t));
                if (rec == NULL) {
                    PRINT("DEV MALLOC API SYNCHRONOUS RECORD DISCARDED!\n");
                    return;
                }
                if (enable_pmu) {
                    EXIT_GET_PMU
                }

                if (cid == ACCL_API_vector_malloc)
                    rec[0].record.MsgType = event_ACCL_API_vector_malloc;
                else if (cid == ACCL_API_scalar_malloc)
                    rec[0].record.MsgType = event_ACCL_API_scalar_malloc;
                else if (cid == ACCL_API_hbm_malloc)
                    rec[0].record.MsgType = event_ACCL_API_hbm_malloc;

                rec[0].correlation_id          = data->correlation_id;
                rec[0].cluster_id              = data->mem_alloc.cluster_id;
                rec[0].bytes                   = data->mem_alloc.bytes;
                rec[0].mode                    = data->mem_alloc.mode;
                rec[0].kind                    = data->mem_alloc.kind;
                rec[0].address                 = data->mem_alloc.address;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
#ifdef DEBUG
                PRINT("\nMalloc:\n");
                PRINT("Correlation ID: %llu\n", rec[0].correlation_id);
                PRINT("Cluster ID: %d\n", rec[0].cluster_id);
                PRINT("Bytes: %d\n", rec[0].bytes);
                PRINT("Mode: %p\n", rec[0].mode);
                PRINT("Kind: %p\n", rec[0].kind);
                PRINT("Enter TS: %llu\n", rec[0].record.timestamps.enter);
                PRINT("Exit TS: %llu\n", rec[0].record.timestamps.exit);
#endif
                break;
            }

            case ACCL_API_vector_free:
            case ACCL_API_scalar_free:
            case ACCL_API_hbm_free: {
                mt_record_free_t *rec;
                if (enable_pmu)
                    rec =
                        (mt_record_free_t *)cb_buffer_alloc(sizeof(mt_record_free_t) + sizeof(uint64_t) * 2 * pmu_num);
                else
                    rec = (mt_record_free_t *)cb_buffer_alloc(sizeof(mt_record_free_t));
                if (rec == NULL) {
                    PRINT("DEV MEM FREE RECORD DISCARDED!\n");
                    return;
                }
                if (enable_pmu) {
                    EXIT_GET_PMU
                }

                if (cid == ACCL_API_vector_free)
                    rec[0].record.MsgType = event_ACCL_API_vector_free;
                else if (cid == ACCL_API_scalar_free)
                    rec[0].record.MsgType = event_ACCL_API_scalar_free;
                else if (cid == ACCL_API_hbm_free)
                    rec[0].record.MsgType = event_ACCL_API_hbm_free;

                rec[0].correlation_id          = data->correlation_id;
                rec[0].address                 = data->mem_alloc.address;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                break;
            }

            case ACCL_API_dma_wait:
            case ACCL_API_dma_wait_p2p:
            case ACCL_API_dma_wait_sg: {

                mt_record_dma_wait_t *rec;
                if (enable_pmu)
                    rec = (mt_record_dma_wait_t *)cb_buffer_alloc(sizeof(mt_record_dma_wait_t)
                                                                  + sizeof(uint64_t) * 2 * pmu_num);
                else
                    rec = (mt_record_dma_wait_t *)cb_buffer_alloc(sizeof(mt_record_dma_wait_t));
                if (rec == NULL) {
                    PRINT("DEV DMA WAIT RECORD DISCARDED!\n");
                    return;
                }
                if (enable_pmu) {
                    EXIT_GET_PMU
                }

                if (cid == ACCL_API_dma_wait)
                    rec[0].record.MsgType = event_ACCL_API_dma_wait;
                else if (cid == ACCL_API_dma_wait_p2p)
                    rec[0].record.MsgType = event_ACCL_API_dma_wait_p2p;
                else if (cid == ACCL_API_dma_wait_sg)
                    rec[0].record.MsgType = event_ACCL_API_dma_wait_sg;

                rec[0].correlation_id          = data->correlation_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
                rec[0].cluster_id              = data->mem_wait.cluster_id;
                rec[0].dma_channel             = data->mem_wait.dma_channel;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                break;
            }

            case ACCL_API_group_barrier:
            case ACCL_API_core_barrier:
            case ACCL_API_core_barrier_wait: {
                mt_record_dev_barrier_t *rec;
                if (enable_pmu)
                    rec = (mt_record_dev_barrier_t *)cb_buffer_alloc(sizeof(mt_record_dev_barrier_t)
                                                                     + sizeof(uint64_t) * 2 * pmu_num);
                else
                    rec = (mt_record_dev_barrier_t *)cb_buffer_alloc(sizeof(mt_record_dev_barrier_t));
                if (rec == NULL) {
                    PRINT("DEV BARRIER RECORD DISCARDED!\n");
                    return;
                }
                if (enable_pmu) {
                    EXIT_GET_PMU
                }

                if (cid == ACCL_API_group_barrier)
                    rec[0].record.MsgType = event_ACCL_API_group_barrier;
                else if (cid == ACCL_API_core_barrier)
                    rec[0].record.MsgType = event_ACCL_API_core_barrier;
                else if (cid == ACCL_API_core_barrier_wait)
                    rec[0].record.MsgType = event_ACCL_API_core_barrier_wait;

                rec[0].barrier_id              = data->dev_barrier.barrier_id;
                rec[0].core_num                = data->dev_barrier.core_num;
                rec[0].timeout                 = data->dev_barrier.timeout;
                rec[0].correlation_id          = data->correlation_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                break;
            }

            case ACCL_API_rwlock_try_rdlock:
            case ACCL_API_rwlock_try_wrlock:
            case ACCL_API_rwlock_rdlock:
            case ACCL_API_rwlock_wrlock:
            case ACCL_API_rwlock_unlock: {
                mt_record_dev_rwlock_t *rec;
                if (enable_pmu)
                    rec = (mt_record_dev_rwlock_t *)cb_buffer_alloc(sizeof(mt_record_dev_rwlock_t)
                                                                    + sizeof(uint64_t) * 2 * pmu_num);
                else
                    rec = (mt_record_dev_rwlock_t *)cb_buffer_alloc(sizeof(mt_record_dev_rwlock_t));
                if (rec == NULL) {
                    PRINT("DEV RWLOCK RECORD DISCARDED!\n");
                    return;
                }
                if (enable_pmu) {
                    EXIT_GET_PMU
                }

                if (cid == ACCL_API_rwlock_try_rdlock)
                    rec[0].record.MsgType = event_ACCL_API_rwlock_try_rdlock;
                else if (cid == ACCL_API_rwlock_try_wrlock)
                    rec[0].record.MsgType = event_ACCL_API_rwlock_try_wrlock;
                else if (cid == ACCL_API_rwlock_rdlock)
                    rec[0].record.MsgType = event_ACCL_API_rwlock_rdlock;
                else if (cid == ACCL_API_rwlock_wrlock)
                    rec[0].record.MsgType = event_ACCL_API_rwlock_wrlock;
                else if (cid == ACCL_API_rwlock_unlock)
                    rec[0].record.MsgType = event_ACCL_API_rwlock_unlock;

                rec[0].lock_id                 = data->dev_rw_lock.lock_id;
                rec[0].op_kind                 = data->dev_rw_lock.op_kind;
                rec[0].correlation_id          = data->correlation_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                break;
            }

            case ACCL_API_intr_handler_register:
            case ACCL_API_cpu_interrupt: {
                mt_record_dev_intr_t *rec;
                if (enable_pmu)
                    rec = (mt_record_dev_intr_t *)cb_buffer_alloc(sizeof(mt_record_dev_intr_t)
                                                                  + sizeof(uint64_t) * 2 * pmu_num);
                else
                    rec = (mt_record_dev_intr_t *)cb_buffer_alloc(sizeof(mt_record_dev_intr_t));
                if (rec == NULL) {
                    PRINT("DEV INTERRUPT RECORD DISCARDED!\n");
                    return;
                }
                if (enable_pmu) {
                    EXIT_GET_PMU
                }

                if (cid == ACCL_API_intr_handler_register)
                    rec[0].record.MsgType = event_ACCL_API_intr_handler_register;
                else if (cid == ACCL_API_cpu_interrupt)
                    rec[0].record.MsgType = event_ACCL_API_cpu_interrupt;

                rec[0].func                    = data->intr.func;
                rec[0].intr_id                 = data->intr.intr_id;
                rec[0].correlation_id          = data->correlation_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                break;
            }

            default: {
                record_activity_t *rec;
                if (enable_pmu)
                    rec = (record_activity_t *)cb_buffer_alloc(sizeof(record_activity_t)
                                                               + sizeof(uint64_t) * 2 * pmu_num);
                else
                    rec = (record_activity_t *)cb_buffer_alloc(sizeof(record_activity_t));
                if (rec == NULL) {
                    PRINT("DEV COMMON RECORD DISCARDED!\n");
                    return;
                }
                if (enable_pmu) {
                    EXIT_GET_PMU
                }

                rec[0].record.MsgType = event_ACCL_API_UNKNOWN;

                rec[0].correlation_id          = data->correlation_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
#ifdef DEBUG
                PRINT("\nDefault:\n");
                PRINT("Correlation ID: %llu\n", rec[0].correlation_id);
                PRINT("Enter TS: %llu\n", rec[0].record.timestamps.enter);
                PRINT("Exit TS: %llu\n", rec[0].record.timestamps.exit);
#endif
            }
        }
    }
}

void (*dev_api_callback)(uint32_t domain, uint32_t cid, void *callback_data, void *arg) = dev_api_callback_impl;
