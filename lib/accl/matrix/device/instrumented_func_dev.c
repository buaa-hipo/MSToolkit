#include "instrument/instrumented_func_dev.h"

#include <stdint.h>
#include <string.h>

#include "hthread_device.h"
#include "instrument/MT_PMU_collector.h"
#include "record/mt_buffer.h"
#include "record/mt_callback_defs.h"
#include "record/mt_dev_types.h"

#ifdef DMA_CALLBACK
// ONE cluster has 20 DMA channels (tested)
unsigned long dma_begin_ts[20];

// todo: or query each dma channel in every trampolines

// if no dma_wait_p2p pr dma_wait_sg then no need to record dma type
// char dma_type[20];
unsigned long dma_corr_id[20];
#endif  // DMA_CALLBACK

uint32_t local_cluster = 0;

unsigned long correlation_id[24] = {0};

#define PRINT(...)

// instrumented dev sync api
void *instrumented_vector_malloc(unsigned int bytes) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_alloc.bytes      = bytes;
    data.mem_alloc.cluster_id = local_cluster;
    data.mem_alloc.mode       = 4;
    data.mem_alloc.kind       = ACCL_MALLOC_AM;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_malloc, &data, NULL);
    void *mem              = vector_malloc(bytes);
    data.mem_alloc.address = mem;
    data.phase             = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_malloc, &data, NULL);

    ++correlation_id[core_id];
    return mem;
}

int instrumented_vector_free(void *ptr) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_alloc.bytes      = 0;
    data.mem_alloc.cluster_id = -1;
    data.mem_alloc.mode       = 0;
    data.mem_alloc.kind       = ACCL_FREE;
    data.mem_alloc.address    = ptr;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_free, &data, NULL);
    int res    = vector_free(ptr);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_free, &data, NULL);

    ++correlation_id[core_id];
    return res;
}

int instrumented_vector_load(void *mem, void *buf, unsigned int bytes) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase               = ACCL_API_ENTER;
    data.correlation_id      = correlation_id[core_id];
    data.mem_sync.dst        = buf;
    data.mem_sync.src        = mem;
    data.mem_sync.size_bytes = bytes;
    data.mem_sync.kind       = ACCL_MEMCPY_OFF_AM;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_load, &data, NULL);
    int res    = vector_load(mem, buf, bytes);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_load, &data, NULL);

    ++correlation_id[core_id];
    return res;
}

int instrumented_vector_store(void *buf, void *mem, unsigned int bytes) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase               = ACCL_API_ENTER;
    data.correlation_id      = correlation_id[core_id];
    data.mem_sync.dst        = mem;
    data.mem_sync.src        = buf;
    data.mem_sync.size_bytes = bytes;
    data.mem_sync.kind       = ACCL_MEMCPY_AM_OFF;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_store, &data, NULL);
    int res    = vector_store(mem, buf, bytes);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_store, &data, NULL);

    ++correlation_id[core_id];
    return res;
}

void *instrumented_scalar_malloc(unsigned int bytes) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_alloc.bytes      = bytes;
    data.mem_alloc.cluster_id = local_cluster;
    data.mem_alloc.mode       = 4;
    data.mem_alloc.kind       = ACCL_MALLOC_SM;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_malloc, &data, NULL);
    void *mem              = scalar_malloc(bytes);
    data.mem_alloc.address = mem;
    data.phase             = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_malloc, &data, NULL);

    ++correlation_id[core_id];
    return mem;
}

int instrumented_scalar_free(void *ptr) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_alloc.bytes      = 0;
    data.mem_alloc.cluster_id = -1;
    data.mem_alloc.mode       = 0;
    data.mem_alloc.kind       = ACCL_FREE;
    data.mem_alloc.address    = ptr;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_free, &data, NULL);
    int res    = scalar_free(ptr);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_free, &data, NULL);

    ++correlation_id[core_id];
    return res;
}

int instrumented_scalar_load(void *mem, void *buf, unsigned int bytes) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase               = ACCL_API_ENTER;
    data.correlation_id      = correlation_id[core_id];
    data.mem_sync.dst        = buf;
    data.mem_sync.src        = mem;
    data.mem_sync.size_bytes = bytes;
    data.mem_sync.kind       = ACCL_MEMCPY_OFF_SM;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_load, &data, NULL);

    int res = scalar_load(mem, buf, bytes);

    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_load, &data, NULL);

    ++correlation_id[core_id];
    return res;
}

int instrumented_scalar_store(void *buf, void *mem, unsigned int bytes) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase               = ACCL_API_ENTER;
    data.correlation_id      = correlation_id[core_id];
    data.mem_sync.dst        = mem;
    data.mem_sync.src        = buf;
    data.mem_sync.size_bytes = bytes;
    data.mem_sync.kind       = ACCL_MEMCPY_SM_OFF;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_store, &data, NULL);

    int res = scalar_store(mem, buf, bytes);

    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_store, &data, NULL);

    ++correlation_id[core_id];
    return res;
}

void *instrumented_hbm_malloc(unsigned long bytes) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_alloc.bytes      = bytes;
    data.mem_alloc.cluster_id = local_cluster;
    data.mem_alloc.mode       = 3;
    data.mem_alloc.kind       = ACCL_MALLOC_HBM;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_hbm_malloc, &data, NULL);

    void *mem = hbm_malloc(bytes);

    data.mem_alloc.address = mem;
    data.phase             = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_hbm_malloc, &data, NULL);

    ++correlation_id[core_id];

    return mem;
}

void instrumented_hbm_free(void *ptr) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_alloc.bytes      = 0;
    data.mem_alloc.cluster_id = -1;
    data.mem_alloc.mode       = 0;
    data.mem_alloc.kind       = ACCL_FREE;
    data.mem_alloc.address    = ptr;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_hbm_free, &data, NULL);

    hbm_free(ptr);

    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_hbm_free, &data, NULL);

    ++correlation_id[core_id];
}

// instrumented dev async api

uint64_t begin_ts[24] = {0};

matrix_api_data_t kernel_data[24];
// matrix_api_data_t user_func_data[4][24];
// int               func_data_stack_tail[24];

void instrumented_kernel(uint32_t host_pid, uint64_t kernel_cid, uint32_t kid,
                         accl_api_phase_t phase) {
/// ! Modified so that activity record for kernel can be in the same trace file as callback records.
#ifdef DMA_CALLBACK
    if (phase == ACCL_API_ENTER) {
        correlation_id[get_core_id()] = kernel_cid + 1;
        MT_PMU_collector_init();
        uint64_t ts             = get_clk();  // todo: use timestamp instead of dsp clock cycles
        begin_ts[get_core_id()] = ts;
        return;
    }
    uint64_t                ts     = get_clk();  // todo: use timestamp instead of dsp clock cycles
    int                     tid    = get_core_id();
    accl_activity_record_t *record = (accl_activity_record_t *)ac_buffer_alloc(sizeof(accl_activity_record_t));
    record->domain                 = ACCL_DOMAIN_HTHREAD_OPS;
    record->kind                   = ACCL_ACTIVITY_kernel;
    record->op                     = kid;
    record->begin_ns               = begin_ts[tid];
    record->end_ns                 = ts;
    record->correlation_id         = kernel_cid;
    record->process_id             = host_pid;
    record->thread_id              = tid;
    record->kernel_index           = kid;  /// todo: kernel name map
#else
    int core_id = get_core_id();
    if (phase == ACCL_API_ENTER) {
        kernel_data[core_id].kernel.domain     = ACCL_DOMAIN_HTHREAD_OPS;
        kernel_data[core_id].kernel.kind       = ACCL_ACTIVITY_kernel;
        kernel_data[core_id].kernel.op         = kid;
        kernel_data[core_id].correlation_id    = kernel_cid;
        kernel_data[core_id].kernel.process_id = host_pid;
        kernel_data[core_id].kernel.thread_id  = core_id;
        kernel_data[core_id].phase             = ACCL_API_ENTER;

        correlation_id[core_id] = kernel_cid + 1;

        if (enable_pmu) simple_pmu_start();

        // func_data_stack_tail[core_id] = -1;
        dev_api_callback(ACCL_DOMAIN_HTHREAD_OPS, kid, &kernel_data[core_id], NULL);
        return;
    } else {
        kernel_data[core_id].phase = ACCL_API_EXIT;
        dev_api_callback(ACCL_DOMAIN_HTHREAD_OPS, kid, &kernel_data[core_id], NULL);

        if (enable_pmu) simple_pmu_end();
    }
#endif

#ifdef DMA_CALLBACK
    PRINT("Kernel:\n");
    PRINT("{\n");
    PRINT("\tCorrelation ID: %llu\n", record->correlation_id);
    PRINT("\tProcess ID: %d\n", record->process_id);
    PRINT("\tThread ID: %d\n", record->thread_id);
    PRINT("\tBegin TS: %llu\n", record->begin_ns);
    PRINT("\tEnd TS: %llu\n", record->end_ns);
    PRINT("\tOP-kid: %u, %u\n", record->op, kid);
    PRINT("}\n");
#endif
}

void instrumented_func(unsigned long cid, accl_api_phase_t phase) {
    // JSI_WARN("[core-%d] Entering instrumented func\n", get_core_id());
    // fflush(stdout);
    int               core_id = get_core_id();
    matrix_api_data_t data;

    if (phase == ACCL_API_ENTER) {
        data.phase = ACCL_API_ENTER;
        ++correlation_id[core_id];
    } else {
        data.phase = ACCL_API_EXIT;
    }
    data.correlation_id = cid;
    dev_api_callback(ACCL_DOMAIN_COMMON, ACCL_USER_FUNC, &data, NULL);

    // JSI_WARN("[core-%d] Exiting instrumented func\n", get_core_id());
    // fflush(stdout);
}

int instrumented_vector_load_async(void *mem, void *buf, unsigned int bytes) {  // mem - dev_mem | buf - vm
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_async.dst        = buf;
    data.mem_async.src        = mem;
    data.mem_async.size_bytes = bytes;
    data.mem_async.kind       = ACCL_MEMCPY_OFF_SM;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_load_async, &data, NULL);

    int channel = vector_load_async(mem, buf, bytes);

    data.mem_async.dma_channel = channel;
#ifdef DMA_CALLBACK
    dma_begin_ts[channel] = get_clk();
    dma_corr_id[channel]  = correlation_id;
#endif
    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_load_async, &data, NULL);

    ++correlation_id[core_id];

    return channel;
}

int instrumented_vector_store_async(void *buf, void *mem, unsigned int bytes) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_async.dst        = mem;
    data.mem_async.src        = buf;
    data.mem_async.size_bytes = bytes;
    data.mem_async.kind       = ACCL_MEMCPY_SM_OFF;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_store_async, &data, NULL);

    int channel                = vector_store_async(mem, buf, bytes);
    data.mem_async.dma_channel = channel;
#ifdef DMA_CALLBACK
    dma_begin_ts[channel] = get_clk();
    dma_corr_id[channel]  = correlation_id;
#endif
    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_vector_store_async, &data, NULL);

    ++correlation_id[core_id];

    return channel;
}

int instrumented_scalar_load_async(void *mem, void *buf, unsigned int bytes) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_async.dst        = buf;
    data.mem_async.src        = mem;
    data.mem_async.size_bytes = bytes;
    data.mem_async.kind       = ACCL_MEMCPY_OFF_SM;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_load_async, &data, NULL);

    int channel                = scalar_load_async(mem, buf, bytes);
    data.mem_async.dma_channel = channel;
#ifdef DMA_CALLBACK
    dma_begin_ts[channel] = get_clk();
    dma_corr_id[channel]  = correlation_id;
#endif
    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_load_async, &data, NULL);

    ++correlation_id[core_id];

    return channel;
}

int instrumented_scalar_store_async(void *buf, void *mem, unsigned int bytes) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_async.dst        = mem;
    data.mem_async.src        = buf;
    data.mem_async.size_bytes = bytes;
    data.mem_async.kind       = ACCL_MEMCPY_OFF_SM;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_store_async, &data, NULL);

    int channel                = scalar_store_async(mem, buf, bytes);
    data.mem_async.dma_channel = channel;
#ifdef DMA_CALLBACK
    dma_begin_ts[channel] = get_clk();
    dma_corr_id[channel]  = correlation_id;
#endif
    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_scalar_store_async, &data, NULL);

    ++correlation_id[core_id];

    return channel;
}

static accl_memcpy_kind_t ACCL_get_memcpy_direction(void *src, void *dst) {
    if (src >= DDR_BASE) {  // from off
        if (dst >= AM_BASE && dst < SM_BASE)
            return ACCL_MEMCPY_OFF_AM;
        else if (dst >= SM_BASE && dst < DDR_BASE)
            return ACCL_MEMCPY_OFF_SM;
        else if (dst < AM_BASE)
            return ACCL_MEMCPY_OFF_GSM;
        else
            return ACCL_MEMCPY_OFF_OFF;
    } else if (src >= SM_BASE && (dst >= DDR_BASE || dst < AM_BASE)) {  // from sm
        return ACCL_MEMCPY_SM_OFF;                                      // to ddr/hbm/gsm
    } else if (src >= AM_BASE && src < SM_BASE && (dst >= DDR_BASE || dst < AM_BASE)) {
        return ACCL_MEMCPY_SM_OFF;
    } else if (src < AM_BASE) {
        if (dst >= AM_BASE && dst < SM_BASE)
            return ACCL_MEMCPY_GSM_AM;
        else if (dst >= SM_BASE && dst < DDR_BASE)
            return ACCL_MEMCPY_GSM_SM;
        else if (dst < AM_BASE)
            return ACCL_MEMCPY_GSM_GSM;
        else
            return ACCL_MEMCPY_GSM_OFF;
    }
    return ACCL_MEMCPY_ERROR;
}

unsigned int instrumented_dma_p2p(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
                                  void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
                                  bool row_syn, unsigned int synmask) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_async.dst        = dst;
    data.mem_async.src        = src;
    data.mem_async.size_bytes = src_row_num * (src_row_size - src_row_step);

    data.mem_async.kind = ACCL_get_memcpy_direction(src, dst);

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_p2p, &data, NULL);

    unsigned int channel       = dma_p2p(src, src_row_num, src_row_size, src_row_step, dst, dst_row_num, dst_row_size,
                                         dst_row_step, row_syn, synmask);
    data.mem_async.dma_channel = channel;
#ifdef DMA_CALLBACK
    dma_begin_ts[channel] = get_clk();
    dma_corr_id[channel]  = correlation_id;
#endif
    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_p2p, &data, NULL);

    ++correlation_id[core_id];

    return channel;
}

unsigned int instrumented_dma_broadcast(void *src, unsigned long src_row_num, unsigned int src_row_size,
                                        int src_row_step, void *dst, unsigned long dst_row_num,
                                        unsigned int dst_row_size, int dst_row_step, unsigned core_id,
                                        unsigned int barrier_id) {
    matrix_api_data_t data;

    data.phase                    = ACCL_API_ENTER;
    data.correlation_id           = correlation_id[get_core_id()];
    data.mem_broadcast.dst        = dst;
    data.mem_broadcast.src        = src;
    data.mem_broadcast.size_bytes = src_row_num * (src_row_size - src_row_step);
    data.mem_broadcast.core_id    = core_id;

    data.mem_broadcast.kind = ACCL_get_memcpy_direction(src, dst);

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_broadcast, &data, NULL);

    unsigned int channel = dma_broadcast(src, src_row_num, src_row_size, src_row_step, dst, dst_row_num, dst_row_size,
                                         dst_row_step, core_id, barrier_id);
    data.mem_broadcast.dma_channel = channel;
#ifdef DMA_CALLBACK
    dma_begin_ts[channel] = get_clk();
    dma_corr_id[channel]  = correlation_id;
#endif
    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_broadcast, &data, NULL);

    ++correlation_id[get_core_id()];

    return channel;
}

unsigned int instrumented_dma_segment(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
                                      void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
                                      unsigned int c_start, unsigned int c_num, unsigned int c_step,
                                      unsigned int barrier_id) {
    matrix_api_data_t data;

    data.phase                  = ACCL_API_ENTER;
    data.correlation_id         = correlation_id[get_core_id()];
    data.mem_segment.dst        = dst;
    data.mem_segment.src        = src;
    data.mem_segment.size_bytes = src_row_num * (src_row_size - src_row_step);
    data.mem_segment.core_num   = c_num;
    data.mem_segment.core_start = c_start;
    data.mem_segment.step       = c_step;

    data.mem_segment.kind = ACCL_get_memcpy_direction(src, dst);

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_segment, &data, NULL);

    unsigned int channel = dma_segment(src, src_row_num, src_row_size, src_row_step, dst, dst_row_num, dst_row_size,
                                       dst_row_step, c_start, c_num, c_step, barrier_id);
    data.mem_segment.dma_channel = channel;
#ifdef DMA_CALLBACK
    dma_begin_ts[channel] = get_clk();
    dma_corr_id[channel]  = correlation_id;
#endif
    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_segment, &data, NULL);

    ++correlation_id[get_core_id()];

    return channel;
}

unsigned int instrumented_dma_sg(void *src_base, void *src_index, unsigned long src_row_num, unsigned int src_row_size,
                                 int src_row_step, void *dst, unsigned long dst_row_num, unsigned int dst_row_size,
                                 int dst_row_step) {
    matrix_api_data_t data;

    data.phase             = ACCL_API_ENTER;
    data.correlation_id    = correlation_id[get_core_id()];
    data.mem_sg.dst        = dst;
    data.mem_sg.src        = src_base;
    data.mem_sg.size_bytes = src_row_num * (src_row_size - src_row_step);
    data.mem_sg.index      = src_index;

    data.mem_sg.kind = ACCL_get_memcpy_direction(src_base, dst);

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_sg, &data, NULL);

    unsigned int channel    = dma_sg(src_base, src_index, src_row_num, src_row_size, src_row_step, dst, dst_row_num,
                                     dst_row_size, dst_row_step);
    data.mem_sg.dma_channel = channel;
#ifdef DMA_CALLBACK
    dma_begin_ts[channel] = get_clk();
    dma_corr_id[channel]  = correlation_id;
#endif
    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_sg, &data, NULL);

    ++correlation_id[get_core_id()];

    return channel;
}

void instrumented_dma_wait(unsigned int ch) {
    // todo: activity api not implemented
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_wait.cluster_id  = local_cluster;
    data.mem_wait.dma_channel = ch;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_wait, &data, NULL);
    dma_wait(ch);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_wait, &data, NULL);
    ++correlation_id[core_id];
}

void instrumented_dma_wait_p2p(unsigned int ch) {
    // todo: activity api not implemented
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_wait.cluster_id  = local_cluster;
    data.mem_wait.dma_channel = ch;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_wait_p2p, &data, NULL);
    dma_wait_p2p(ch);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_wait_p2p, &data, NULL);
    ++correlation_id[core_id];
}

void instrumented_dma_wait_sg(unsigned int ch) {
    // todo: activity api not implemented
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                = ACCL_API_ENTER;
    data.correlation_id       = correlation_id[core_id];
    data.mem_wait.cluster_id  = local_cluster;
    data.mem_wait.dma_channel = ch;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_wait_sg, &data, NULL);
    dma_wait_sg(ch);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dma_wait_sg, &data, NULL);
    ++correlation_id[core_id];
}

void instrumented_group_barrier(unsigned int b_id) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                  = ACCL_API_ENTER;
    data.correlation_id         = correlation_id[core_id];
    data.dev_barrier.barrier_id = b_id;
    data.dev_barrier.core_num   = get_group_size();
    data.dev_barrier.timeout    = UINT64_MAX;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_barrier, &data, NULL);

    group_barrier(b_id);

    data.phase = ACCL_API_EXIT;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_barrier, &data, NULL);

    ++correlation_id[core_id];
}

void instrumented_core_barrier(unsigned int b_id, unsigned int num) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                  = ACCL_API_ENTER;
    data.correlation_id         = correlation_id[core_id];
    data.dev_barrier.barrier_id = b_id;
    data.dev_barrier.core_num   = num;
    data.dev_barrier.timeout    = UINT64_MAX;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_core_barrier, &data, NULL);

    core_barrier(b_id, num);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_core_barrier, &data, NULL);

    ++correlation_id[core_id];
}

void instrumented_core_barrier_wait(unsigned int b_id, unsigned int num, unsigned long wait_clk) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase                  = ACCL_API_ENTER;
    data.correlation_id         = correlation_id[core_id];
    data.dev_barrier.barrier_id = b_id;
    data.dev_barrier.core_num   = num;
    data.dev_barrier.timeout    = wait_clk;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_core_barrier_wait, &data, NULL);

    core_barrier_wait(b_id, num, wait_clk);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_core_barrier_wait, &data, NULL);

    ++correlation_id[core_id];
}

int instrumented_rwlock_try_rdlock(unsigned int lock_id) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase               = ACCL_API_ENTER;
    data.correlation_id      = correlation_id[core_id];
    data.dev_rw_lock.lock_id = lock_id;
    data.dev_rw_lock.op_kind = 0;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_try_rdlock, &data, NULL);

    int result = rwlock_try_rdlock(lock_id);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_try_rdlock, &data, NULL);

    ++correlation_id[core_id];

    return result;
}

int instrumented_rwlock_try_wrlock(unsigned int lock_id) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase               = ACCL_API_ENTER;
    data.correlation_id      = correlation_id[core_id];
    data.dev_rw_lock.lock_id = lock_id;
    data.dev_rw_lock.op_kind = 1;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_try_wrlock, &data, NULL);

    int result = rwlock_try_wrlock(lock_id);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_try_wrlock, &data, NULL);

    ++correlation_id[core_id];

    return result;
}

void instrumented_rwlock_rdlock(unsigned int lock_id) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase               = ACCL_API_ENTER;
    data.correlation_id      = correlation_id[core_id];
    data.dev_rw_lock.lock_id = lock_id;
    data.dev_rw_lock.op_kind = 2;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_rdlock, &data, NULL);

    rwlock_rdlock(lock_id);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_rdlock, &data, NULL);

    ++correlation_id[core_id];
}

void instrumented_rwlock_wrlock(unsigned int lock_id) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase               = ACCL_API_ENTER;
    data.correlation_id      = correlation_id[core_id];
    data.dev_rw_lock.lock_id = lock_id;
    data.dev_rw_lock.op_kind = 3;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_wrlock, &data, NULL);

    rwlock_wrlock(lock_id);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_wrlock, &data, NULL);

    ++correlation_id[core_id];
}

void instrumented_rwlock_unlock(unsigned int lock_id) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase               = ACCL_API_ENTER;
    data.correlation_id      = correlation_id[core_id];
    data.dev_rw_lock.lock_id = lock_id;
    data.dev_rw_lock.op_kind = 4;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_unlock, &data, NULL);

    rwlock_unlock(lock_id);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_unlock, &data, NULL);

    ++correlation_id[core_id];
}

unsigned long instrumented_intr_handler_register(void (*func)(int no)) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase          = ACCL_API_ENTER;
    data.correlation_id = correlation_id[core_id];
    data.intr.func      = (void *)func;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_intr_handler_register, &data, NULL);

    unsigned long result = intr_handler_register(func);
    data.phase           = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_intr_handler_register, &data, NULL);

    ++correlation_id[core_id];

    return result;
}

void instrumented_cpu_interrupt(unsigned long val) {
    int               core_id = get_core_id();
    matrix_api_data_t data;

    data.phase          = ACCL_API_ENTER;
    data.correlation_id = correlation_id[core_id];
    data.intr.intr_id   = val;

    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_cpu_interrupt, &data, NULL);

    cpu_interrupt(val);
    data.phase = ACCL_API_EXIT;
    dev_api_callback(ACCL_DOMAIN_HTHREAD_API, ACCL_API_cpu_interrupt, &data, NULL);

    ++correlation_id[core_id];
}
