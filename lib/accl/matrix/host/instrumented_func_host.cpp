
#include "instrument/instrumented_func_host.h"

#include <unistd.h>

#include <unordered_map>

#include "hthread_host.h"
#include "ral/section.h"
#include "record/accl_tracer.h"
#include "record/mt_callback_defs.h"
#include "record/record_defines.h"
#include "record/record_writer.h"
#include "utils/jsi_log.h"

static unsigned long host_correlation_id = 1;  // todo: how to set cid ? is this safe ?

// in a process, each hthread api call would have a unique correlation id
// thus api enter record can be associated with exit record

/**
 *  load/unload a dat to toggle flag of corresponding cluster
 *  no need to toggle when closing a cluster
 */
static bool dat_loaded[4] = {0};

unsigned long buf_args[5];
unsigned long pmu_init_args[3];

extern std::unique_ptr<pse::ral::DirSectionInterface> &get_thread_trace_dir();

static std::unordered_map<int, int> group2cluster;
static std::unordered_map<int, int> group2threads;

static std::unordered_map<std::string, size_t> kernel2offset;

void *instrumented_hthread_malloc(int cluster_id, int bytes, int mode) {
    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

    data->phase                = ACCL_API_ENTER;
    data->correlation_id       = host_correlation_id;
    data->mem_alloc.bytes      = bytes;
    data->mem_alloc.cluster_id = cluster_id;
    data->mem_alloc.mode       = mode;
    data->mem_alloc.kind       = ACCL_MALLOC_DDR;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_malloc, data, NULL);

    void *mem = hthread_malloc(cluster_id, bytes, mode);

    data->phase             = ACCL_API_EXIT;
    data->mem_alloc.address = mem;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_malloc, data, NULL);
    DEALLOCATE(data, sizeof(matrix_api_data_t));
    ++host_correlation_id;

    return mem;
}

void instrumented_hthread_free(void *ptr) {
    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

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
    DEALLOCATE(data, sizeof(matrix_api_data_t));
    ++host_correlation_id;
}

int instrumented_hthread_group_create6(int cluster_id, int num, const char *func_name, int scalar_args, int ptr_args,
                                       uint64_t *arg_array) {

    size_t offset;
    if (kernel2offset.contains(func_name)) {
        offset = kernel2offset.at(func_name);
    } else {
        auto &dir                = get_thread_trace_dir();
        auto  string_sec         = dir->openStringSection(StaticSectionDesc::COMMON_USE_STRING_SEC, true);
        offset                   = string_sec->write(func_name);
        kernel2offset[func_name] = offset;
    }

    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

    data->correlation_id              = host_correlation_id;
    data->kernel_info.cluster_id      = cluster_id;
    data->kernel_info.thread_num      = num;
    data->kernel_info.thread_mask     = 0;
    data->kernel_info.scalar_args_num = scalar_args;
    data->kernel_info.ptr_args_num    = ptr_args;
    data->phase                       = ACCL_API_ENTER;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create_launch, data, NULL);

    int para_num = scalar_args + ptr_args;

    unsigned long tmp_args[para_num + 3];
    tmp_args[0] = (unsigned long)getpid();
    tmp_args[1] = (unsigned long)host_correlation_id;
    tmp_args[2] = offset;
    for (int i = 0; i < para_num; ++i) {
        tmp_args[i + 3] = arg_array[i];
    }

    int group_id = hthread_group_create(cluster_id, num, func_name, scalar_args + 3, ptr_args, tmp_args);

    group2cluster[group_id] = cluster_id;
    group2threads[group_id] = num;

    data->phase                       = ACCL_API_EXIT;
    data->kernel_info.thread_group_id = group_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create_launch, data, NULL);

    ++host_correlation_id;
    DEALLOCATE(data, sizeof(matrix_api_data_t));
    return group_id;
}

int instrumented_hthread_group_create2(int cluster_id, int num) {
    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

    data->correlation_id              = host_correlation_id;
    data->kernel_info.cluster_id      = cluster_id;
    data->kernel_info.thread_num      = num;
    data->kernel_info.thread_mask     = 0;
    data->kernel_info.scalar_args_num = 0;
    data->kernel_info.ptr_args_num    = 0;
    data->phase                       = ACCL_API_ENTER;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create, data, NULL);

    int group_id            = hthread_group_create(cluster_id, num);
    group2cluster[group_id] = cluster_id;
    group2threads[group_id] = num;

    data->phase                       = ACCL_API_EXIT;
    data->kernel_info.thread_group_id = group_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create, data, NULL);

    ++host_correlation_id;
    DEALLOCATE(data, sizeof(matrix_api_data_t));
    return group_id;
}

int instrumented_hthread_group_masked_create(int cluster_id, uint32_t pmask, const char *func_name, int scalar_args,
                                             int ptr_args, uint64_t *arg_array) {

    size_t offset;
    if (kernel2offset.contains(func_name)) {
        offset = kernel2offset.at(func_name);
    } else {
        auto &dir                = get_thread_trace_dir();
        auto  string_sec         = dir->openStringSection(StaticSectionDesc::COMMON_USE_STRING_SEC, true);
        offset                   = string_sec->write(func_name);
        kernel2offset[func_name] = offset;
    }

    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

    uint32_t num = __builtin_popcount(pmask);

    data->correlation_id              = host_correlation_id;
    data->kernel_info.cluster_id      = cluster_id;
    data->kernel_info.thread_num      = num;
    data->kernel_info.thread_mask     = pmask;
    data->kernel_info.scalar_args_num = scalar_args;
    data->kernel_info.ptr_args_num    = ptr_args;
    data->phase                       = ACCL_API_ENTER;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create_masked_launch, data, NULL);

    int           para_num = scalar_args + ptr_args;
    unsigned long tmp_args[para_num + 3];
    tmp_args[0] = (unsigned long)getpid();
    tmp_args[1] = (unsigned long)host_correlation_id;
    tmp_args[2] = offset;
    for (int i = 0; i < para_num; ++i) {
        tmp_args[i + 3] = arg_array[i];
    }

    int group_id = hthread_group_masked_create(cluster_id, pmask, func_name, scalar_args + 3, ptr_args, tmp_args);
    group2cluster[group_id] = cluster_id;
    group2threads[group_id] = num;

    data->phase                       = ACCL_API_EXIT;
    data->kernel_info.thread_group_id = group_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_create_masked_launch, data, NULL);
    DEALLOCATE(data, sizeof(matrix_api_data_t));
    ++host_correlation_id;

    return group_id;
}

int instrumented_hthread_group_exec(int thread_id, const char *func_name, int scalar_args, int ptr_args,
                                    uint64_t *arg_array) {

    size_t offset;
    if (kernel2offset.contains(func_name)) {
        offset = kernel2offset.at(func_name);
    } else {
        auto &dir                = get_thread_trace_dir();
        auto  string_sec         = dir->openStringSection(StaticSectionDesc::COMMON_USE_STRING_SEC, true);
        offset                   = string_sec->write(func_name);
        kernel2offset[func_name] = offset;
    }

    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

    data->correlation_id              = host_correlation_id;
    data->kernel_info.cluster_id      = group2cluster[thread_id];
    data->kernel_info.thread_num      = group2threads[thread_id];
    data->kernel_info.thread_mask     = 0;
    data->kernel_info.scalar_args_num = scalar_args;
    data->kernel_info.ptr_args_num    = ptr_args;
    data->phase                       = ACCL_API_ENTER;
    data->kernel_info.thread_group_id = thread_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_exec, data, NULL);

    int           para_num = scalar_args + ptr_args;
    unsigned long tmp_args[para_num + 3];
    tmp_args[0] = (unsigned long)getpid();
    tmp_args[1] = (unsigned long)host_correlation_id;
    tmp_args[2] = offset;
    for (int i = 0; i < para_num; ++i) {
        tmp_args[i + 3] = arg_array[i];
    }

    int err = hthread_group_exec(thread_id, func_name, scalar_args + 3, ptr_args, tmp_args);

    data->phase = ACCL_API_EXIT;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_exec, data, NULL);
    DEALLOCATE(data, sizeof(matrix_api_data_t));
    ++host_correlation_id;

    return err;
}

int instrumented_hthread_group_wait(int thread_id) {
    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

    data->phase                       = ACCL_API_ENTER;
    data->correlation_id              = host_correlation_id;
    data->kernel_info.thread_group_id = thread_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_wait, data, NULL);

    int res = hthread_group_wait(thread_id);

    if (res != HT_SUCCESS) {
        JSI_ERROR("kernel of cluster-%d failed with error code %d\n", opened_cluster, res);
    }

    data->phase = ACCL_API_EXIT;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_wait, data, NULL);

    ++host_correlation_id;

    tracer.flush();
    for (int i = 0; i < 24; ++i) {
        ((uint32_t *)(buf_args[3]))[i] = 0;
        ((uint32_t *)(buf_args[4]))[i] = 0;
    }
    DEALLOCATE(data, sizeof(matrix_api_data_t));
    return res;
}

int instrumented_hthread_group_destroy(int thread_id) {
    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

    data->correlation_id              = host_correlation_id;
    data->kernel_info.cluster_id      = group2cluster[thread_id];
    data->kernel_info.thread_num      = group2threads[thread_id];
    data->kernel_info.thread_mask     = 0;
    data->kernel_info.scalar_args_num = 0;
    data->kernel_info.ptr_args_num    = 0;
    data->phase                       = ACCL_API_ENTER;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_destroy, data, NULL);

    int err = hthread_group_destroy(thread_id);
    group2threads.erase(thread_id);
    group2cluster.erase(thread_id);

    data->phase = ACCL_API_EXIT;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_group_destroy, data, NULL);
    DEALLOCATE(data, sizeof(matrix_api_data_t));
    ++host_correlation_id;

    return err;
}

static void free_buffers() {
    for (int i = 1; i < 5; ++i) {
        if ((void *)buf_args[i] != NULL) {
            hthread_free((void *)buf_args[i]);
            buf_args[i] = (unsigned long)NULL;
        }
    }
    if ((void *)pmu_init_args[2] != NULL) {
        hthread_free((void *)pmu_init_args[2]);
        pmu_init_args[2] = (unsigned long)NULL;
    }
}

int instrumented_hthread_dat_load(int init_dev_id, const char *path) {
    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

    opened_cluster = init_dev_id;

    data->dev.kind       = 0;
    data->dev.cluster_id = init_dev_id;
    data->phase          = ACCL_API_ENTER;
    data->correlation_id = host_correlation_id;

    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dat_load, data, NULL);

    if (dat_loaded[init_dev_id]) {
        tracer.flush();
        // int available_thread_num = hthread_get_avail_threads(init_dev_id);
        // // mt_buffer.c
        // int id = hthread_group_create6(init_dev_id, available_thread_num, "dev_pmu_end", 0, 0, NULL);
        // hthread_group_wait(id);
        // hthread_group_destroy(id);
        free_buffers();
    }

    dat_loaded[init_dev_id] = true;

    int result = hthread_dat_load(init_dev_id, path);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dat_load, data, NULL);

    // Initialize device buffers and flags.
    buf_args[0] = (unsigned long)init_dev_id;
    // callback_buffers[init_dev_id]
    buf_args[1] = (unsigned long)hthread_malloc(init_dev_id, 24 * JSI_DEV_BUFFER_SIZE, HT_MEM_RW);
    // activity_buffers[init_dev_id]
    buf_args[2] = (unsigned long)hthread_malloc(init_dev_id, 24 * JSI_DEV_BUFFER_SIZE, HT_MEM_RW);
    // callback_used[init_dev_id]
    buf_args[3] = (unsigned long)hthread_malloc(init_dev_id, 24 * sizeof(uint32_t), HT_MEM_RW);
    // activity_used[init_dev_id]
    buf_args[4] = (unsigned long)hthread_malloc(init_dev_id, 24 * sizeof(uint32_t), HT_MEM_RW);

    for (int i = 0; i < 24; ++i) {
        callback_buffers[i]    = (void *)((char *)buf_args[1] + i * JSI_DEV_BUFFER_SIZE);
        activity_buffers[i]    = (void *)((char *)buf_args[2] + i * JSI_DEV_BUFFER_SIZE);
        callback_used_host[i]  = (uint32_t *)buf_args[3] + i;
        activity_used_host[i]  = (uint32_t *)buf_args[4] + i;
        *callback_used_host[i] = 0;
        *activity_used_host[i] = 0;
    }

    // mt_buffer.c
    int init_id = hthread_group_create(init_dev_id, 1, "buffer_init", 1, 4, buf_args);
    int err     = hthread_group_wait(init_id);
    if (err != HT_SUCCESS) {
        JSI_ERROR("buffer_init kernel of cluster-%d failed with error code %d\n", init_dev_id, err);
    }

    // Initialize device PMUs.
    int       event_num = tracer.device_pmu_events.size();
    uint32_t *event_ids = (uint32_t *)hthread_malloc(init_dev_id, sizeof(uint32_t) * event_num, HT_MEM_RW);

    JSI_INFO("Device pmu events:\n");
    for (auto event : tracer.device_pmu_events) {
        JSI_INFO("%d ", event);
    }
    JSI_INFO("\n");

    for (int i = 0; i < event_num; ++i) {
        event_ids[i] = tracer.device_pmu_events[i];
    }
    if (getenv("JSI_COLLECT_DEV_PMU_EVENT") != NULL)
        pmu_init_args[0] = 1;
    else
        pmu_init_args[0] = 0;
    pmu_init_args[1] = (unsigned long)event_num;
    pmu_init_args[2] = (unsigned long)event_ids;
    // mt_buffer.c
    hthread_group_exec(init_id, "dev_pmu_init", 2, 1, pmu_init_args);
    err = hthread_group_wait(init_id);
    if (err != HT_SUCCESS) {
        JSI_ERROR("dev_pmu_init kernel of cluster-%d failed with error code %d\n", init_dev_id, err);
    }
    hthread_group_destroy(init_id);

    // // Start device PMUs.
    // int available_thread_num = hthread_get_avail_threads(init_dev_id);
    // // mt_buffer.c
    // int id = hthread_group_create6(init_dev_id, available_thread_num, "dev_pmu_start", 0, 0, NULL);
    // hthread_group_wait(id);
    // hthread_group_destroy(id);

    DEALLOCATE(data, sizeof(matrix_api_data_t));
    ++host_correlation_id;
    return result;
}

int instrumented_hthread_dat_unload(int dev_id) {
    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

    data->dev.kind       = 1;
    data->dev.cluster_id = dev_id;
    data->phase          = ACCL_API_ENTER;
    data->correlation_id = host_correlation_id;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dat_unload, data, NULL);

    tracer.flush();
    // int available_thread_num = hthread_get_avail_threads(dev_id);
    // // mt_buffer.c
    // int id = hthread_group_create6(dev_id, available_thread_num, "dev_pmu_end", 0, 0, NULL);
    // hthread_group_wait(id);
    // hthread_group_destroy(id);
    free_buffers();

    opened_cluster = -1;

    dat_loaded[dev_id] = false;

    int result = hthread_dat_unload(dev_id);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dat_unload, data, NULL);
    DEALLOCATE(data, sizeof(matrix_api_data_t));

    ++host_correlation_id;
    return result;
}

int instrumented_hthread_dev_close(int dev_id) {
    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

    data->dev.kind       = 1;
    data->dev.cluster_id = dev_id;
    data->phase          = ACCL_API_ENTER;
    data->correlation_id = host_correlation_id;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dev_close, data, NULL);

    if (dat_loaded[dev_id]) {
        tracer.flush();
        // int available_thread_num = hthread_get_avail_threads(dev_id);
        // // mt_buffer.c
        // int id = hthread_group_create6(dev_id, available_thread_num, "dev_pmu_end", 0, 0, NULL);
        // hthread_group_wait(id);
        // hthread_group_destroy(id);
        free_buffers();
        opened_cluster     = -1;
        dat_loaded[dev_id] = false;
    }

    int result = hthread_dev_close(dev_id);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dev_close, data, NULL);
    ++host_correlation_id;
    DEALLOCATE(data, sizeof(matrix_api_data_t));
    return result;
}

int instrumented_hthread_dev_open(int dev_id) {
    matrix_api_data_t *data = (matrix_api_data_t *)ALLOCATE(sizeof(matrix_api_data_t));

    data->dev.kind       = 0;
    data->dev.cluster_id = dev_id;
    data->phase          = ACCL_API_ENTER;
    data->correlation_id = host_correlation_id;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dev_open, data, NULL);

    int result = hthread_dev_open(dev_id);

    data->phase = ACCL_API_EXIT;
    tracer.api_callback_(ACCL_DOMAIN_HTHREAD_API, ACCL_API_dev_open, data, NULL);
    ++host_correlation_id;
    DEALLOCATE(data, sizeof(matrix_api_data_t));
    return result;
}