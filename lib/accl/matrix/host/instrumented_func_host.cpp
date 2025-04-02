
#include "instrument/instrumented_func_host.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>

#include "hthread_host.h"
#include "record/accl_tracer.h"
#include "record/mt_callback_defs.h"
#include "record/mt_double_buffer.h"
#include "record/record_writer.h"

static unsigned int ddr_mem_cost = 0;
static unsigned int hbm_mem_cost = 0;
// static unsigned int ddr_mem_cost = 0;

static unsigned long host_correlation_id = 1;  // todo: how to set cid ? is this safe ?

// in a process, each hthread api call would have a unique correlation id
// thus api enter record can be associated with exit record

/**
 *  load/unload a dat to toggle flag of corresponding cluster
 *  no need to toggle when closing a cluster
 */
static bool dat_loaded[4] = {0};

// std::vector<std::string> kernel_index;

void *instrumented_hthread_malloc(int cluster_id, int bytes, int mode) {
    matrix_api_data_t *data    = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->phase                = ACCL_API_ENTER;
    data->correlation_id       = host_correlation_id;
    data->mem_alloc.bytes      = bytes;
    data->mem_alloc.cluster_id = cluster_id;
    data->mem_alloc.mode       = mode;
    data->mem_alloc.kind       = ACCL_MALLOC_DDR;

    JSI_WARN("Before entering callback of hthread_malloc\n");
    fflush(stderr);
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_malloc, data, NULL);
    JSI_WARN("After entering callback of hthread_malloc\n");
    fflush(stderr);

    void *mem = hthread_malloc(cluster_id, bytes, mode);

    if (mem) ddr_mem_cost += bytes;

    data->phase             = ACCL_API_EXIT;
    data->mem_alloc.address = mem;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_malloc, data, NULL);
    free(data);
    ++host_correlation_id;

    return mem;
}

void instrumented_hthread_free(void *ptr) {
    matrix_api_data_t *data    = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->phase                = ACCL_API_ENTER;
    data->correlation_id       = host_correlation_id;
    data->mem_alloc.bytes      = 0;
    data->mem_alloc.cluster_id = -1;
    data->mem_alloc.mode       = 0;  // todo: 0 for free ?
    data->mem_alloc.kind       = ACCL_FREE;
    data->mem_alloc.address    = ptr;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_free, data, NULL);

    hthread_free(ptr);

    data->phase = ACCL_API_EXIT;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_free, data, NULL);
    free(data);
    ++host_correlation_id;
}

static matrix_api_data_t *ACCL_get_hthread_group_record(int cluster_id, int num, const char *func_name, int scalar_args,
                                                        int ptr_args, uint64_t *arg_array, uint32_t mask) {
    matrix_api_data_t *data      = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->correlation_id         = host_correlation_id;
    data->kernel_info.cluster_id = cluster_id;
    data->kernel_info.thread_num = num;
    if (func_name == nullptr)
        data->kernel_info.kernel_index = 0;
    else
        data->kernel_info.kernel_index = kernel_index.size() - 1;
    data->kernel_info.thread_mask     = mask;
    data->kernel_info.scalar_args_num = scalar_args;
    data->kernel_info.ptr_args_num    = ptr_args;
    data->kernel_info.thread_group_id = -1;
    return data;
}

#ifdef USE_NATIVE_META_API
// ! RecordWriterExt here
static RecordWriterExt *writer;

static union {
    struct {
        uint16_t domain;
        uint32_t op;
        uint16_t kind;
    };

    uint64_t kernel_key;
} accl_key;

static void write_kernel_meta(const char *func_name) {
    // todo: performance test
    accl_key.domain = ACCL_DOMAIN_HTHREAD_OPS;
    accl_key.op     = kernel_index.size() - 1;
    accl_key.kind   = ACCL_ACTIVITY_kernel;
    writer->writeStringBuffer(accl_key.kernel_key, func_name);
}
#endif

extern std::unique_ptr<pse::ral::DirSectionInterface> &get_thread_trace_dir();

int instrumented_hthread_group_create6(int cluster_id, int num, const char *func_name, int scalar_args, int ptr_args,
                                       uint64_t *arg_array) {

    auto &dir        = get_thread_trace_dir();
    auto  string_sec = dir->openStringSection(StaticSectionDesc::COMMON_USE_STRING_SEC, true);
    auto  res        = string_sec->write(func_name);

    matrix_api_data_t *data =
        ACCL_get_hthread_group_record(cluster_id, num, func_name, scalar_args, ptr_args, arg_array, 0);
    data->phase = ACCL_API_ENTER;
#ifndef DMA_CALLBACK

#endif
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create_launch, data, NULL);

    int           para_num = scalar_args + ptr_args;
    unsigned long tmp_args[para_num + 3];
    tmp_args[0] = (unsigned long)getpid();
    tmp_args[1] = (unsigned long)host_correlation_id;
    tmp_args[2] = res;
    for (int i = 0; i < para_num; ++i) {
        tmp_args[i + 3] = arg_array[i];
    }

    int group_id = hthread_group_create(cluster_id, num, func_name, scalar_args + 3, ptr_args, tmp_args);

    data->phase                       = ACCL_API_EXIT;
    data->kernel_info.thread_group_id = group_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create_launch, data, NULL);

    ++host_correlation_id;
    free(data);
    return group_id;
}

int instrumented_hthread_group_create2(int cluster_id, int num) {
    matrix_api_data_t *data = ACCL_get_hthread_group_record(cluster_id, num, nullptr, 0, 0, NULL, 0);
    data->phase             = ACCL_API_ENTER;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create, data, NULL);

    int group_id = hthread_group_create(cluster_id, num);

    data->phase                       = ACCL_API_EXIT;
    data->kernel_info.thread_group_id = group_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create, data, NULL);

    ++host_correlation_id;
    free(data);
    return group_id;
}

int instrumented_hthread_group_masked_create(int cluster_id, uint32_t pmask, const char *func_name, int scalar_args,
                                             int ptr_args, uint64_t *arg_array) {

    auto &dir        = get_thread_trace_dir();
    auto  string_sec = dir->openStringSection(StaticSectionDesc::COMMON_USE_STRING_SEC, true);
    auto  res        = string_sec->write(func_name);

    uint32_t           num = __builtin_popcount(pmask);
    matrix_api_data_t *data =
        ACCL_get_hthread_group_record(cluster_id, num, func_name, scalar_args, ptr_args, arg_array, pmask);
    data->phase = ACCL_API_ENTER;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create_masked_launch, data, NULL);

    int           para_num = scalar_args + ptr_args;
    unsigned long tmp_args[para_num + 3];
    tmp_args[0] = (unsigned long)getpid();
    tmp_args[1] = (unsigned long)host_correlation_id;
    tmp_args[2] = res;
    for (int i = 0; i < para_num; ++i) {
        tmp_args[i + 3] = arg_array[i];
    }

    int group_id = hthread_group_masked_create(cluster_id, pmask, func_name, scalar_args + 3, ptr_args, tmp_args);

    data->phase                       = ACCL_API_EXIT;
    data->kernel_info.thread_group_id = group_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create_masked_launch, data, NULL);
    free(data);
    ++host_correlation_id;

    return group_id;
}

int instrumented_hthread_group_exec(int thread_id, const char *func_name, int scalar_args, int ptr_args,
                                    uint64_t *arg_array) {

    auto &dir        = get_thread_trace_dir();
    auto  string_sec = dir->openStringSection(StaticSectionDesc::COMMON_USE_STRING_SEC, true);
    auto  res        = string_sec->write(func_name);

    matrix_api_data_t *data = ACCL_get_hthread_group_record(-1, 0, func_name, scalar_args, ptr_args, arg_array, 0);
    data->phase             = ACCL_API_ENTER;
    data->kernel_info.thread_group_id = thread_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_exec, data, NULL);

    int           para_num = scalar_args + ptr_args;
    unsigned long tmp_args[para_num + 3];
    tmp_args[0] = (unsigned long)getpid();
    tmp_args[1] = (unsigned long)host_correlation_id;
    tmp_args[2] = res;
    for (int i = 0; i < para_num; ++i) {
        tmp_args[i + 3] = arg_array[i];
    }

    int err = hthread_group_exec(thread_id, func_name, scalar_args + 3, ptr_args, tmp_args);

    data->phase = ACCL_API_EXIT;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_exec, data, NULL);
    free(data);
    ++host_correlation_id;

    return err;
}

int instrumented_hthread_group_wait(int thread_id) {
    matrix_api_data_t *data           = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->phase                       = ACCL_API_ENTER;
    data->correlation_id              = host_correlation_id;
    data->kernel_info.thread_group_id = thread_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_wait, data, NULL);

    int res = hthread_group_wait(thread_id);

    data->phase = ACCL_API_EXIT;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_wait, data, NULL);

    ++host_correlation_id;

    return res;
}

int instrumented_hthread_group_destroy(int thread_id) {
    matrix_api_data_t *data = ACCL_get_hthread_group_record(-1, 0, "", 0, 0, NULL, 0);
    data->phase             = ACCL_API_ENTER;
    data->correlation_id    = host_correlation_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_destroy, data, NULL);

    int err = hthread_group_destroy(thread_id);

    data->phase = ACCL_API_EXIT;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_destroy, data, NULL);
    free(data);
    ++host_correlation_id;

    return err;
}

int instrumented_hthread_dat_load(int init_dev_id, const char *path) {
    matrix_api_data_t *data = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->dev.kind          = 0;
    data->dev.cluster_id    = init_dev_id;
    data->phase             = ACCL_API_ENTER;
    data->correlation_id    = host_correlation_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dat_load, data, NULL);

    if (dat_loaded[init_dev_id]) {  // finalize double buffer and retrieve data
        int g = hthread_group_create(init_dev_id, 1, "double_buffer_fin", 0, 0, NULL);
        hthread_group_wait(g);
        // ! join consumer thread before cluster reset.
        tracer.StopTracing();
        printf("Consumer thread joined\n");
    } else {
        dat_loaded[init_dev_id] = true;
    }
    /// TODO: initialize shared memory pointers.
    int           result = hthread_dat_load(init_dev_id, path);
    unsigned long buf_args[6];
    buf_args[0] = (unsigned long)init_dev_id;
    buf_args[1] = (unsigned long)hthread_malloc(init_dev_id, 24 * 2 * BUFFSIZE, HT_MEM_RW);
    // callback_buffers[init_dev_id]
    buf_args[2] = (unsigned long)hthread_malloc(init_dev_id, 24 * 2 * BUFFSIZE, HT_MEM_RW);
    buf_args[3] = (unsigned long)hthread_malloc(init_dev_id, 24 * 2 * sizeof(uint32_t), HT_MEM_RW);
    buf_args[4] = (unsigned long)hthread_malloc(init_dev_id, 24 * 2 * sizeof(uint32_t), HT_MEM_RW);
    buf_args[5] = (unsigned long)hthread_malloc(init_dev_id, 4 * sizeof(uint32_t), HT_MEM_RW);

    callback_used_host[init_dev_id] = (uint32_t *)buf_args[3];
    activity_used_host[init_dev_id] = (uint32_t *)buf_args[4];

    callback_writing_indices[init_dev_id] = (uint32_t *)buf_args[5];
    callback_ready_indices[init_dev_id]   = (uint32_t *)buf_args[5] + 1;
    activity_writing_indices[init_dev_id] = (uint32_t *)buf_args[5] + 2;
    activity_ready_indices[init_dev_id]   = (uint32_t *)buf_args[5] + 3;

    for (int i = 0; i < 24; ++i) {
        callback_buffers[init_dev_id][i] = (void *)((char *)buf_args[1] + i * 2 * BUFFSIZE);
#ifdef DEBUG
        printf("host - Callback buffer for Cluster-%d Core-%d: %p\n", init_dev_id, i, callback_buffers[init_dev_id][i]);
#endif
    }
    for (int i = 0; i < 24; ++i) {
        activity_buffers[init_dev_id][i] = (void *)((char *)buf_args[2] + i * 2 * BUFFSIZE);
#ifdef DEBUG
        printf("host - Activity buffer for Cluster-%d Core-%d: %p\n", init_dev_id, i, activity_buffers[init_dev_id][i]);
#endif
    }

#ifdef DEBUG
    printf("host - callback_writing_indices: %p\n", callback_writing_indices[init_dev_id]);
    printf("host - activity_writing_indices: %p\n", activity_writing_indices[init_dev_id]);
#endif
    int init_id = hthread_group_create(init_dev_id, 1, "double_buffer_init", 1, 5, buf_args);
    hthread_group_wait(init_id);
    int       event_num = tracer.device_pmu_events.size();
    uint32_t *event_ids = (uint32_t *)hthread_malloc(init_dev_id, sizeof(uint32_t) * event_num, HT_MEM_RO);
    for (int i = 0; i < event_num; ++i) {
        event_ids[i] = tracer.device_pmu_events[i];
    }
    unsigned long tmp_args[2] = {(unsigned long)event_num, (unsigned long)event_ids};
    JSI_WARN("Before dev_pmu_init\n");
    fflush(stderr);
    hthread_group_exec(init_id, "dev_pmu_init", 1, 1, tmp_args);
    hthread_group_wait(init_id);
    hthread_group_destroy(init_id);
    tracer.StartTracing();

    JSI_WARN("After dev_pmu_init\n");
    fflush(stderr);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dat_load, data, NULL);
    free(data);
    ++host_correlation_id;
    return result;
}

int instrumented_hthread_dat_unload(int dev_id) {
    matrix_api_data_t *data = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->dev.kind          = 1;
    data->dev.cluster_id    = dev_id;
    data->phase             = ACCL_API_ENTER;
    data->correlation_id    = host_correlation_id;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dat_unload, data, NULL);

    int g = hthread_group_create(dev_id, 1, "double_buffer_fin", 0, 0, NULL);
    hthread_group_wait(g);
    // ! join consumer thread before cluster reset.
    tracer.StopTracing();
    printf("Consumer thread joined\n");

    dat_loaded[dev_id] = false;
    int result         = hthread_dat_unload(dev_id);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dat_unload, data, NULL);
    free(data);
    ++host_correlation_id;
    return result;
}

int instrumented_hthread_dev_close(int dev_id) {
    matrix_api_data_t *data = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->dev.kind          = 1;
    data->dev.cluster_id    = dev_id;
    data->phase             = ACCL_API_ENTER;
    data->correlation_id    = host_correlation_id;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dev_close, data, NULL);

    if (dat_loaded[dev_id]) {  // finalize double buffer and retrieve data
        int g = hthread_group_create(dev_id, 1, "double_buffer_fin", 0, 0, NULL);
        hthread_group_wait(g);
        // ! join consumer thread before cluster reset.
        tracer.StopTracing();
        printf("Consumer thread joined\n");

        dat_loaded[dev_id] = false;
    }

    int result = hthread_dev_close(dev_id);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dev_close, data, NULL);
    ++host_correlation_id;
    free(data);
    return result;
}

int instrumented_hthread_dev_open(int dev_id) {
    matrix_api_data_t *data = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->dev.kind          = 0;
    data->dev.cluster_id    = dev_id;
    data->phase             = ACCL_API_ENTER;
    data->correlation_id    = host_correlation_id;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dev_open, data, NULL);

    int result = hthread_dev_open(dev_id);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dev_open, data, NULL);
    ++host_correlation_id;
    free(data);
    return result;
}

int instrumented_hthread_dev_owner(int dev_id) {
    matrix_api_data_t *data = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->phase             = ACCL_API_ENTER;
    data->correlation_id    = host_correlation_id;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dev_owner, data, NULL);

    int result = hthread_dev_owner(dev_id);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dev_owner, data, NULL);
    ++host_correlation_id;
    free(data);
    return result;
}

int instrumented_hthread_group_get_status(int group_id) {
    matrix_api_data_t *data = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->phase             = ACCL_API_ENTER;
    data->correlation_id    = host_correlation_id;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_get_status, data, NULL);

    int result = hthread_group_get_status(group_id);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_get_status, data, NULL);
    ++host_correlation_id;
    free(data);
    return result;
}

int instrumented_hthread_barrier_create(int dev_id) {
    matrix_api_data_t *data  = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->phase              = ACCL_API_ENTER;
    data->correlation_id     = host_correlation_id;
    data->barrier.cluster_id = dev_id;
    data->barrier.kind       = 0;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_barrier_create, data, NULL);

    int result = hthread_barrier_create(dev_id);

    data->phase              = ACCL_API_EXIT;
    data->barrier.barrier_id = result;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_barrier_create, data, NULL);
    ++host_correlation_id;
    free(data);
    return result;
}

void instrumented_hthread_barrier_destroy(int barrier_id) {
    matrix_api_data_t *data  = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->phase              = ACCL_API_ENTER;
    data->correlation_id     = host_correlation_id;
    data->barrier.cluster_id = 4;
    data->barrier.kind       = 1;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_barrier_destroy, data, NULL);

    hthread_barrier_destroy(barrier_id);

    data->phase              = ACCL_API_EXIT;
    data->barrier.barrier_id = barrier_id;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_barrier_destroy, data, NULL);
    ++host_correlation_id;
    free(data);
}

int instrumented_hthread_rwlock_create(int dev_id) {
    matrix_api_data_t *data  = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->phase              = ACCL_API_ENTER;
    data->correlation_id     = host_correlation_id;
    data->rw_lock.cluster_id = dev_id;
    data->rw_lock.kind       = 0;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_create, data, NULL);

    int result = hthread_rwlock_create(dev_id);

    data->phase           = ACCL_API_EXIT;
    data->rw_lock.lock_id = result;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_create, data, NULL);
    ++host_correlation_id;
    free(data);
    return result;
}

void instrumented_hthread_rwlock_destroy(int lock_id) {
    matrix_api_data_t *data  = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->phase              = ACCL_API_ENTER;
    data->correlation_id     = host_correlation_id;
    data->rw_lock.cluster_id = 4;
    data->rw_lock.kind       = 0;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_destroy, data, NULL);

    hthread_rwlock_destroy(lock_id);

    data->phase           = ACCL_API_EXIT;
    data->rw_lock.lock_id = lock_id;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_rwlock_destroy, data, NULL);
    ++host_correlation_id;
    free(data);
}

void instrumented_hthread_intr_send(int g_id, int t_id, unsigned long intr_id) {
    matrix_api_data_t *data   = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->phase               = ACCL_API_ENTER;
    data->correlation_id      = host_correlation_id;
    data->intr.group_id  = g_id;
    data->intr.thread_id = t_id;
    data->intr.intr_id   = intr_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_intr_send, data, NULL);

    hthread_intr_send(g_id, t_id, intr_id);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_intr_send, data, NULL);
    ++host_correlation_id;
    free(data);
}

unsigned long instrumented_hthread_handler_register(int thread_id, void (*func)(int id, unsigned long val)) {
    matrix_api_data_t *data   = (matrix_api_data_t *)malloc(sizeof(matrix_api_data_t));
    data->phase               = ACCL_API_ENTER;
    data->correlation_id      = host_correlation_id;
    data->intr.group_id  = thread_id;
    data->intr.thread_id = INT_MAX;
    data->intr.func      = (void *)func;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_intr_reg, data, NULL);

    int result = hthread_handler_register(thread_id, func);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_intr_reg, data, NULL);
    ++host_correlation_id;
    free(data);
    return result;
}