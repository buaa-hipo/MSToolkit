
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include "record/mt_record_types.h"
#include "hthread_device.h"
#include "instrument/MT_PMU_collector.h"
#include "record/mt_callback_defs.h"
#include "record/mt_dev_types.h"
#include "record/mt_double_buffer.h"
#include "record/wrap_defines_macro.h"

#define RING_SIZE 100

/// todo: enough ?
#define MAX_PMU_SIZE 10

/// todo: pass event ids and length from host
uint32_t *pmu_events;
uint32_t  pmu_num;

typedef struct {
    uint64_t timestamp;
    uint64_t pmu[MAX_PMU_SIZE];
} BufferNode;

bool enable_pmu = true;

BufferNode ring_buffer[24][RING_SIZE];
bool       buffer_node_valid[24][RING_SIZE];

typedef uint32_t bt_type;

int32_t bt_get() {
    return -1;
}

/**
 * @brief function to pick specific event from all 26 events
 *
 * @param pmus          [uint64_t]  counter reads
 * @param event_ids     [uint32_t]  target event ids
 * @param size          uint32_t    length of event_ids
 * @return uint64_t*    picked counter reads
 */
static uint64_t *mt_dsp_pmu_pick(uint64_t *pmus, uint32_t *event_ids, uint32_t size) {
    if (size > 26) JSI_ERROR("Too many DSP performance events!\nMAX: 26 | PICKING: %d\n", size);
    uint64_t *picked = (uint64_t *)malloc(size * sizeof(uint64_t));
    int       index  = 0;
    for (uint32_t i = 0; i < size; ++i) {
        picked[index++] = pmus[event_ids[i]];
    }
    return picked;
}

/**
 * @brief User defined callback for synchronous APIs
 *
 * @param domain API domain (accl_api_domain_t)
 * @param cid API operation (accl_api_op_t)
 * @param callback_data API call description (matrix_api_data_t)
 * @param arg Arguments for data
 */
void dev_api_callback_impl(uint32_t domain, uint32_t cid, const void *callback_data, void *arg) {
    static uint64_t pmus[MAX_PMU_NUM];  // todo: test this

    const matrix_api_data_t *data = (matrix_api_data_t *)(callback_data);
#ifdef DEBUG
    hthread_printf("Here in the callback of %d, correlation id is: %lu\n", cid, data->correlation_id);
#endif
    // hthread_printf("In the callback of %d, correlation ID is %lu\n", cid, data->correlation_id);

    uint64_t ts = get_clk();  // todo: not timestamp
    if (data->phase == ACCL_API_ENTER) {

        uint64_t ri = data->correlation_id % RING_SIZE;
        if (!buffer_node_valid[get_core_id()][ri]) {
            ring_buffer[get_core_id()][ri].timestamp = ts;
            if (enable_pmu) {
                MT_PMU_collector_get_all(pmus);
                uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
#ifdef DEBUG
                hthread_printf("DEV pmu_num is %d, chosen events are:\n", pmu_num);
                for (int i = 0; i < pmu_num; ++i) {
                    hthread_printf("%d ", pmu_events[i]);
                }
                hthread_printf("\nPrinting pmus in the callback ENTER phase:\n");
                // for (int i = 0; i < pmu_num; ++i) {
                //     char name[20];
                //     PMU_event_code_to_name(pmu_events[i], name);
                //     hthread_printf("%s: %lu\n", name, picked[i]);
                // }
#endif
                memcpy(ring_buffer[get_core_id()][ri].pmu, picked, sizeof(uint64_t) * pmu_num);
                free(picked);
            }
            buffer_node_valid[get_core_id()][ri] = 1;
        } else {
            hthread_printf("RING BUFFER COLLIDED IN ENTER CALLBACK! Happend on Correlation id %lu\n",
                           data->correlation_id);
            JSI_ERROR("TS BUFF RING COLLISION DETECTED. USE LARGER RING_SIZE!");
        }

    } else {
        size_t   ri       = data->correlation_id % RING_SIZE;
        uint64_t enter_ts = ring_buffer[get_core_id()][ri].timestamp;  // todo: not timestamp
        if (!buffer_node_valid[get_core_id()][ri]) {
            hthread_printf("RING BUFFER COLLIDED IN EXIT CALLBACK! Happened on Correlation id %lu\n",
                           data->correlation_id);

            JSI_ERROR("The timestamp in the ring buffer is invalid!");
        } else {
            buffer_node_valid[get_core_id()][ri] = 0;
        }
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
                hthread_printf("KERNEL RECORD DISCARDED!\n");
                fflush(stdout);
                return;
            }
            if (enable_pmu) {
                mt_record_kernel_t *tmp      = rec + 1;
                uint64_t           *counters = (uint64_t *)tmp;
                memcpy(counters, ring_buffer[get_core_id()][ri].pmu, pmu_num * sizeof(uint64_t));
                MT_PMU_collector_get_all(pmus);
                uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
                memcpy(counters + pmu_num, picked, sizeof(uint64_t) * pmu_num);
                free(picked);
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
            hthread_printf("Kernel:\n");
            hthread_printf("Message type: %u\n", rec[0].record.MsgType);
            hthread_printf("Correlation ID: %llu\n", rec[0].correlation_id);
            hthread_printf("Enter TS: %llu\n", rec[0].record.timestamps.enter);
            hthread_printf("Exit TS: %llu\n", rec[0].record.timestamps.exit);
            hthread_printf("DOMAIN-OP-KIND: %u-%u-%u\n", rec[0].domain, rec[0].op, rec[0].kind);
#endif
            buffer_node_valid[get_core_id()][ri] = 0;
            return;
        }
        switch (cid) {
            case ACCL_USER_FUNC: {
                record_activity_t *rec;
                if (enable_pmu)
                    rec = (record_activity_t *)cb_buffer_alloc(sizeof(record_activity_t)
                                                               + sizeof(uint64_t) * 2 * pmu_num);
                else
                    rec = (record_activity_t *)cb_buffer_alloc(sizeof(record_activity_t));
                if (rec == NULL) {
                    hthread_printf("USER FUNCTION RECORD DISCARDED!\n");
                    fflush(stdout);
                    return;
                }
                rec->record.MsgType          = event_ACCL_USER_FUNC;
                rec->record.timestamps.enter = ts;
                rec->record.timestamps.exit  = 0;
                rec[0].correlation_id        = data->correlation_id;
                if (enable_pmu) {
                    record_activity_t *tmp      = rec + 1;
                    uint64_t          *counters = (uint64_t *)tmp;
                    memcpy(counters, ring_buffer[get_core_id()][ri].pmu, pmu_num * sizeof(uint64_t));
                    // MT_PMU_collector_get_all(counters + MAX_PMU_SIZE);
                    MT_PMU_collector_get_all(pmus);
                    uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
#ifdef DEBUG
                    hthread_printf("Printing pmus in the callback EXIT phase:\n");
                    for (int i = 0; i < pmu_num; ++i) {
                        char name[20];
                        PMU_event_code_to_name(pmu_events[i], name);
                        hthread_printf("%s: %lu\n", name, picked[i]);
                    }
#endif
                    memcpy(counters + pmu_num, picked, sizeof(uint64_t) * pmu_num);
                    free(picked);
                }

                rec[0].correlation_id = data->correlation_id;
                rec[0].record.MsgType = event_ACCL_USER_FUNC;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef DEBUG
                hthread_printf("USER FUNCTION:\n");
                hthread_printf("Correlation ID: %llu\n", rec[0].correlation_id);
                hthread_printf("Enter TS: %llu\n", rec[0].record.timestamps.enter);
                hthread_printf("Exit TS: %llu\n", rec[0].record.timestamps.exit);
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
                    hthread_printf("MEMCPY RECORD DISCARDED!\n");
                    fflush(stdout);
                    return;
                }
                if (enable_pmu) {
                    mt_record_memcpy_t *tmp      = rec + 1;
                    uint64_t           *counters = (uint64_t *)tmp;
                    memcpy(counters, ring_buffer[get_core_id()][ri].pmu, pmu_num * sizeof(uint64_t));
                    MT_PMU_collector_get_all(pmus);
                    uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
#ifdef DEBUG
                    hthread_printf("Printing pmus in the callback EXIT phase:\n");
                    for (int i = 0; i < pmu_num; ++i) {
                        char name[20];
                        PMU_event_code_to_name(pmu_events[i], name);
                        hthread_printf("%s: %lu\n", name, picked[i]);
                    }
#endif
                    memcpy(counters + pmu_num, picked, sizeof(uint64_t) * pmu_num);
                    free(picked);
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
                hthread_printf("\nMemcpy:\n");
                hthread_printf("Correlation ID: %llu\n", rec[0].correlation_id);
                hthread_printf("SRC: %p\n", rec[0].src);
                hthread_printf("DST: %p\n", rec[0].dst);
                hthread_printf("Bytes: %d\n", rec[0].bytes);
                hthread_printf("Kind: %p\n", rec[0].kind);
                hthread_printf("Enter TS: %llu\n", rec[0].record.timestamps.enter);
                hthread_printf("Exit TS: %llu\n", rec[0].record.timestamps.exit);
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
                    hthread_printf("MEMCPY ASYNC RECORD DISCARDED!\n");
                    fflush(stdout);
                    return;
                }
                if (enable_pmu) {
                    mt_record_memcpy_async_t *tmp      = rec + 1;
                    uint64_t                 *counters = (uint64_t *)tmp;
                    memcpy(counters, ring_buffer[get_core_id()][ri].pmu, pmu_num * sizeof(uint64_t));
                    MT_PMU_collector_get_all(pmus);
                    uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
#ifdef DEBUG
                    hthread_printf("Printing pmus in the callback EXIT phase:\n");
                    for (int i = 0; i < pmu_num; ++i) {
                        char name[20];
                        PMU_event_code_to_name(pmu_events[i], name);
                        hthread_printf("%s: %lu\n", name, picked[i]);
                    }
#endif
                    memcpy(counters + pmu_num, picked, sizeof(uint64_t) * pmu_num);
                    free(picked);
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
                hthread_printf("\nMemcpy Async:\n");
                hthread_printf("Correlation ID: %llu\n", rec[0].correlation_id);
                hthread_printf("SRC: %p\n", rec[0].src);
                hthread_printf("DST: %p\n", rec[0].dst);
                hthread_printf("Bytes: %d\n", rec[0].bytes);
                hthread_printf("Kind: %p\n", rec[0].kind);
                hthread_printf("DMA Channel: %d\n", rec[0].dma_channel);
                hthread_printf("Enter TS: %llu\n", rec[0].record.timestamps.enter);
                hthread_printf("Exit TS: %llu\n", rec[0].record.timestamps.exit);
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
                    hthread_printf("DEV MALLOC API SYNCHRONOUS RECORD DISCARDED!\n");
                    fflush(stdout);
                    return;
                }
                if (enable_pmu) {
                    mt_record_malloc_t *tmp      = rec + 1;
                    uint64_t           *counters = (uint64_t *)tmp;
                    memcpy(counters, ring_buffer[get_core_id()][ri].pmu, pmu_num * sizeof(uint64_t));
                    MT_PMU_collector_get_all(pmus);
                    uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
#ifdef DEBUG
                    hthread_printf("Printing pmus in the callback EXIT phase:\n");
                    for (int i = 0; i < pmu_num; ++i) {
                        char name[20];
                        PMU_event_code_to_name(pmu_events[i], name);
                        hthread_printf("%s: %lu\n", name, picked[i]);
                    }
#endif
                    memcpy(counters + pmu_num, picked, sizeof(uint64_t) * pmu_num);
                    free(picked);
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
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
#ifdef DEBUG
                hthread_printf("\nMalloc:\n");
                hthread_printf("Correlation ID: %llu\n", rec[0].correlation_id);
                hthread_printf("Cluster ID: %d\n", rec[0].cluster_id);
                hthread_printf("Bytes: %d\n", rec[0].bytes);
                hthread_printf("Mode: %p\n", rec[0].mode);
                hthread_printf("Kind: %p\n", rec[0].kind);
                hthread_printf("Enter TS: %llu\n", rec[0].record.timestamps.enter);
                hthread_printf("Exit TS: %llu\n", rec[0].record.timestamps.exit);
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
                    hthread_printf("DEV MEM FREE RECORD DISCARDED!\n");
                    fflush(stdout);
                    return;
                }
                if (enable_pmu) {
                    mt_record_free_t *tmp      = rec + 1;
                    uint64_t         *counters = (uint64_t *)tmp;
                    memcpy(counters, ring_buffer[get_core_id()][ri].pmu, pmu_num * sizeof(uint64_t));
                    MT_PMU_collector_get_all(pmus);
                    uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
#ifdef DEBUG
                    hthread_printf("Printing pmus in the callback EXIT phase:\n");
                    for (int i = 0; i < pmu_num; ++i) {
                        char name[20];
                        PMU_event_code_to_name(pmu_events[i], name);
                        hthread_printf("%s: %lu\n", name, picked[i]);
                    }
#endif
                    memcpy(counters + pmu_num, picked, sizeof(uint64_t) * pmu_num);
                    free(picked);
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
#ifdef DEBUG
                hthread_printf("\nFREE:\n");
                hthread_printf("Correlation ID: %llu\n", rec[0].correlation_id);
                hthread_printf("Address: %p\n", rec[0].address);
                hthread_printf("Enter TS: %llu\n", rec[0].record.timestamps.enter);
                hthread_printf("Exit TS: %llu\n", rec[0].record.timestamps.exit);
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
                    hthread_printf("DEV DMA WAIT RECORD DISCARDED!\n");
                    fflush(stdout);
                    return;
                }
                if (enable_pmu) {
                    mt_record_dma_wait_t *tmp      = rec + 1;
                    uint64_t             *counters = (uint64_t *)tmp;
                    memcpy(counters, ring_buffer[get_core_id()][ri].pmu, pmu_num * sizeof(uint64_t));
                    MT_PMU_collector_get_all(pmus);
                    uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
#ifdef DEBUG
                    hthread_printf("Printing pmus in the callback EXIT phase:\n");
                    for (int i = 0; i < pmu_num; ++i) {
                        char name[20];
                        PMU_event_code_to_name(pmu_events[i], name);
                        hthread_printf("%s: %lu\n", name, picked[i]);
                    }
#endif
                    memcpy(counters + pmu_num, picked, sizeof(uint64_t) * pmu_num);
                    free(picked);
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
                    hthread_printf("DEV BARRIER RECORD DISCARDED!\n");
                    fflush(stdout);
                    return;
                }
                if (enable_pmu) {
                    mt_record_dev_barrier_t *tmp      = rec + 1;
                    uint64_t                *counters = (uint64_t *)tmp;
                    memcpy(counters, ring_buffer[get_core_id()][ri].pmu, pmu_num * sizeof(uint64_t));
                    MT_PMU_collector_get_all(pmus);
                    uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
#ifdef DEBUG
                    hthread_printf("Printing pmus in the callback EXIT phase:\n");
                    for (int i = 0; i < pmu_num; ++i) {
                        char name[20];
                        PMU_event_code_to_name(pmu_events[i], name);
                        hthread_printf("%s: %lu\n", name, picked[i]);
                    }
#endif
                    memcpy(counters + pmu_num, picked, sizeof(uint64_t) * pmu_num);
                    free(picked);
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
                    hthread_printf("DEV RWLOCK RECORD DISCARDED!\n");
                    fflush(stdout);
                    return;
                }
                if (enable_pmu) {
                    mt_record_dev_rwlock_t *tmp      = rec + 1;
                    uint64_t               *counters = (uint64_t *)tmp;
                    memcpy(counters, ring_buffer[get_core_id()][ri].pmu, pmu_num * sizeof(uint64_t));
                    MT_PMU_collector_get_all(pmus);
                    uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
#ifdef DEBUG
                    hthread_printf("Printing pmus in the callback EXIT phase:\n");
                    for (int i = 0; i < pmu_num; ++i) {
                        char name[20];
                        PMU_event_code_to_name(pmu_events[i], name);
                        hthread_printf("%s: %lu\n", name, picked[i]);
                    }
#endif
                    memcpy(counters + pmu_num, picked, sizeof(uint64_t) * pmu_num);
                    free(picked);
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
                    hthread_printf("DEV INTERRUPT RECORD DISCARDED!\n");
                    fflush(stdout);
                    return;
                }
                if (enable_pmu) {
                    mt_record_dev_intr_t *tmp      = rec + 1;
                    uint64_t             *counters = (uint64_t *)tmp;
                    memcpy(counters, ring_buffer[get_core_id()][ri].pmu, pmu_num * sizeof(uint64_t));
                    MT_PMU_collector_get_all(pmus);
                    uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
#ifdef DEBUG
                    hthread_printf("Printing pmus in the callback EXIT phase:\n");
                    for (int i = 0; i < pmu_num; ++i) {
                        char name[20];
                        PMU_event_code_to_name(pmu_events[i], name);
                        hthread_printf("%s: %lu\n", name, picked[i]);
                    }
#endif
                    memcpy(counters + pmu_num, picked, sizeof(uint64_t) * pmu_num);
                    free(picked);
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
                    hthread_printf("DEV COMMON RECORD DISCARDED!\n");
                    fflush(stdout);
                    return;
                }
                if (enable_pmu) {
                    record_activity_t *tmp      = rec + 1;
                    uint64_t          *counters = (uint64_t *)tmp;
                    memcpy(counters, ring_buffer[get_core_id()][ri].pmu, pmu_num * sizeof(uint64_t));
                    MT_PMU_collector_get_all(pmus);
                    uint64_t *picked = mt_dsp_pmu_pick(pmus, pmu_events, pmu_num);
#ifdef DEBUG
                    hthread_printf("Printing pmus in the callback EXIT phase:\n");
                    for (int i = 0; i < pmu_num; ++i) {
                        char name[20];
                        PMU_event_code_to_name(pmu_events[i], name);
                        hthread_printf("%s: %lu\n", name, picked[i]);
                    }
#endif
                    memcpy(counters + pmu_num, picked, sizeof(uint64_t) * pmu_num);
                    free(picked);
                }

                rec[0].record.MsgType = event_ACCL_API_UNKNOWN;

                rec[0].correlation_id          = data->correlation_id;
                rec[0].record.timestamps.enter = enter_ts;
                rec[0].record.timestamps.exit  = ts;
#ifdef ENABLE_BACKTRACE
                rec[0].record.ctxt = ctxt;
#endif
#ifdef DEBUG
                hthread_printf("\nDefault:\n");
                hthread_printf("Correlation ID: %llu\n", rec[0].correlation_id);
                hthread_printf("Enter TS: %llu\n", rec[0].record.timestamps.enter);
                hthread_printf("Exit TS: %llu\n", rec[0].record.timestamps.exit);
#endif
            }
            buffer_node_valid[get_core_id()][ri] = 0;
        }
    }
}

void (*dev_api_callback)(uint32_t domain, uint32_t cid, const void *callback_data, void *arg) = dev_api_callback_impl;
