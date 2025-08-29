
#include "record/mt_double_buffer.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hthread_device.h"
#include "record/mt_dev_types.h"

#define BUFFSIZE (1024)

// double buffer for records
void     *callback_buffer[24][2];    // 24 cores, 2 buffers for each core, 1MB for each buffer
void     *activity_buffer[24][2];    // 24 cores, 2 buffers for each core, 1MB for each buffer
uint32_t *callback_used_dev[24][2];  // free mem space in the callback "writing
                                     // pool" (2 ints * 24)
uint32_t *activity_used_dev[24][2];  // free mem space in the activity "writing
                                     // pool" (2 ints * 24)

// pointer to share with host
uint32_t *cb_writing_pool_index[24];  // each bit stands for the index of pool currently
                                      // being writen, modified by DEV
uint32_t *cb_ready_pool_index[24];    // each bit stands for the index of pool currently can
                                      // be writen, modified by HOST
uint32_t *ac_writing_pool_index[24];  // each bit stands for the index of pool currently
                                      // being writen, modified by DEV
uint32_t *ac_ready_pool_index[24];    // each bit stands for the index of pool currently can
                                      // be writen, modified by HOST

uint32_t *dev_flush_flag;

bool enable_pmu;

void *cb_buffer_alloc(size_t bytes) {
#ifdef DEBUG
    JSI_WARN("Allocating callback buffer - %lu\n", bytes);
#endif

    int core_id       = get_core_id();
    int writing_index = *cb_writing_pool_index[core_id];

    if (*callback_used_dev[core_id][*cb_writing_pool_index[core_id]] + bytes > BUFFSIZE) {
        if (*cb_ready_pool_index[core_id] == *cb_writing_pool_index[core_id]) {
            return NULL;
        }
#ifdef DEBUG
        JSI_WARN("Current callback buffer is not enough\n");
        fflush(stdout);
#endif

        *cb_writing_pool_index[core_id] = 1 - writing_index;

#ifdef DEBUG
        JSI_WARN("[core-%d] Changing callback buffer to %d\n", core_id, *cb_writing_pool_index[core_id]);
        fflush(stdout);
#endif

        *callback_used_dev[core_id][writing_index] = bytes;
        return callback_buffer[core_id][writing_index];
    } else {
        void *block =
            (void *)((char *)callback_buffer[core_id][writing_index] + *callback_used_dev[core_id][writing_index]);
        *callback_used_dev[core_id][writing_index] += bytes;
#ifdef DEBUG
        JSI_WARN("[core-%d] Callback buffer used: %d bytes\n", core_id, *callback_used_dev[core_id][writing_index]);
        fflush(stdout);
#endif
        return block;
    }
}

void *ac_buffer_alloc(size_t bytes) {
#ifdef DEBUG
    JSI_WARN("Allocating activity buffer - %lu\n", bytes);
#endif
    int core_id = get_core_id();

    if (*activity_used_dev[core_id][*ac_writing_pool_index[core_id]] + bytes > BUFFSIZE) {
        if (*ac_ready_pool_index[core_id] == *ac_writing_pool_index[core_id]) return NULL;
#ifdef DEBUG
        JSI_WARN("Current activity buffer is not enough\n");
#endif

        *ac_writing_pool_index[core_id] = 1 - *ac_writing_pool_index[core_id];

        *activity_used_dev[core_id][*ac_writing_pool_index[core_id]] = bytes;
        return activity_buffer[core_id][*ac_writing_pool_index[core_id]];
    } else {
        void *block = (void *)((char *)activity_buffer[core_id][*ac_writing_pool_index[core_id]]
                               + *activity_used_dev[core_id][*ac_writing_pool_index[core_id]]);
        *activity_used_dev[core_id][*ac_writing_pool_index[core_id]] += bytes;
        return block;
    }
}

__attribute__((optimize("0"))) void flush_buffer() {
    *dev_flush_flag = 114514;
}

__attribute__((optimize("0"))) void new_flush_wait() {
    while (*dev_flush_flag == 114514) {
        dsp_sleep(10000);
    }
}

__global__ __attribute__((optimize("0"))) void double_buffer_init(uint32_t accl_dev_id, void *cb_buf, void *ac_buf,
                                                                  uint32_t *cb_used, uint32_t *ac_used, uint32_t *flags,
                                                                  uint32_t *host_flush_flag) {
#ifdef DEBUG
    hthread_printf("Double buffer initializing\n");
    fflush(stdout);
#endif
    for (int i = 0; i < 24; ++i) {
        callback_buffer[i][0] = (void *)((char *)cb_buf + 2 * i * BUFFSIZE);
        callback_buffer[i][1] = (void *)((char *)cb_buf + 2 * i * BUFFSIZE + BUFFSIZE);
        // hthread_printf("Cluster-%u, callback_buffer[%d][0] is %p\n", accl_dev_id, i, callback_buffer[i][0]);
        // hthread_printf("Cluster-%u, callback_buffer[%d][1] is %p\n", accl_dev_id, i, callback_buffer[i][1]);
    }
    for (int i = 0; i < 24; ++i) {
        activity_buffer[i][0] = (void *)((char *)ac_buf + 2 * i * BUFFSIZE);
        activity_buffer[i][1] = (void *)((char *)ac_buf + 2 * i * BUFFSIZE + BUFFSIZE);
    }
    memset(cb_buf, 0, 24 * 2 * BUFFSIZE);
    memset(ac_buf, 0, 24 * 2 * BUFFSIZE);
    for (int i = 0; i < 24; ++i) {
        callback_used_dev[i][0] = cb_used + 2 * i;
        callback_used_dev[i][1] = cb_used + 2 * i + 1;
        // hthread_printf("Cluster-%u, core-%d callback_used_dev[%d][0] is %p\n", accl_dev_id, get_core_id(), i,
        // callback_used_dev[i][0]); hthread_printf("Cluster-%u, core-%d callback_used_dev[%d][1] is %p\n", accl_dev_id,
        // get_core_id(), i, callback_used_dev[i][1]);
        activity_used_dev[i][0] = ac_used + 2 * i;
        activity_used_dev[i][1] = ac_used + 2 * i + 1;
    }
#ifdef DEBUG
    hthread_printf("DEV - Callback buffer starts at %p\n", callback_buffer);
    hthread_printf("DEV - Activity buffer starts at %p\n", activity_buffer);
    fflush(stdout);
#endif
    for (int i = 0; i < 24; ++i) {
        *callback_used_dev[i][0] = 0;
        *callback_used_dev[i][1] = 0;
        *activity_used_dev[i][0] = 0;
        *activity_used_dev[i][1] = 0;
    }
    for (int i = 0; i < 24; ++i) {
        cb_writing_pool_index[i]  = (uint32_t *)flags + i * 4;
        *cb_writing_pool_index[i] = 0;
        cb_ready_pool_index[i]    = (uint32_t *)flags + i * 4 + 1;
        *cb_ready_pool_index[i]   = 1;
        ac_writing_pool_index[i]  = (uint32_t *)flags + i * 4 + 2;
        *ac_writing_pool_index[i] = 0;
        ac_ready_pool_index[i]    = (uint32_t *)flags + i * 4 + 3;
        *ac_ready_pool_index[i]   = 1;
    }

    dev_flush_flag  = host_flush_flag;
    *dev_flush_flag = 0;
    local_cluster   = accl_dev_id;
#ifdef DEBUG
    hthread_printf("Double buffer initialized\n");
    fflush(stdout);
#endif
}

__global__ __attribute__((optimize("0"))) void double_buffer_fin() {
#ifdef DEBUG
    hthread_printf("Double buffer finalizing...\n");
#endif
    flush_buffer();
    // flush_cb_buffer();
    // ! ac buffer not maintained
    // flush_ac_buffer();
    // flush_wait();
    new_flush_wait();
#ifdef DEBUG
    hthread_printf("Double buffer finalized\n");
#endif
}

__global__ __attribute__((optimize("0"))) void dev_pmu_init(uint32_t enable_dev_pmu, uint32_t event_num,
                                                            uint32_t *event_ids) {
#ifdef DEBUG
    hthread_printf("In dev pmu init kernel\n");
    fflush(stdout);
#endif
    pmu_num    = event_num;
    pmu_events = event_ids;
    if (enable_dev_pmu != 0) {
        enable_pmu = true;
    } else {
        enable_pmu = false;
    }
#ifdef DEBUG
    hthread_printf("Dev pmu initialized\n");
    fflush(stdout);
#endif
}