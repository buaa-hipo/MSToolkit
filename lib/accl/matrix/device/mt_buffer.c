
#include "record/mt_buffer.h"

#include <stdint.h>

#include "hthread_device.h"
#include "instrument/MT_PMU_collector.h"

void     *callback_buffer[24];
void     *activity_buffer[24];
uint32_t *callback_used_dev[24];
uint32_t *activity_used_dev[24];

bool enable_pmu = false;

void *cb_buffer_alloc(size_t bytes) {
    int core_id = get_core_id();
    if (*callback_used_dev[core_id] + bytes > BUFFSIZE) {
        return NULL;
    } else {
        void *block = (void *)((char *)callback_buffer[core_id] + *callback_used_dev[core_id]);
        *callback_used_dev[core_id] += bytes;
        return block;
    }
}

#ifdef DMA_CALLBACK
void *ac_buffer_alloc(size_t bytes) {
    int core_id = get_core_id();
    if (*activity_used_dev[core_id] + bytes > BUFFSIZE) {
        return NULL;
    } else {
        void *block = (void *)((char *)activity_buffer[core_id] + *activity_used_dev[core_id]);
        *activity_used_dev[core_id] += bytes;
        return block;
    }
}
#endif

__global__ void buffer_init(uint32_t accl_dev_id, void *cb_buf, void *ac_buf, uint32_t *cb_used, uint32_t *ac_used) {
    for (int i = 0; i < 24; ++i) {
        callback_buffer[i]   = (void *)((char *)cb_buf + i * BUFFSIZE);
        activity_buffer[i]   = (void *)((char *)ac_buf + i * BUFFSIZE);
        callback_used_dev[i] = cb_used + i;
        activity_used_dev[i] = ac_used + i;
    }

    local_cluster = accl_dev_id;
}

__global__ void dev_pmu_init(uint32_t enable_dev_pmu, uint32_t event_num, uint32_t *event_ids) {
    pmu_num    = event_num;
    pmu_events = event_ids;
    if (enable_dev_pmu != 0) {
        enable_pmu = true;
        hthread_printf("DEV: PMU enabled!\n");
    } else {
        enable_pmu = false;
        hthread_printf("DEV: PMU disabled!\n");
    }
}

// prof_read() will be 0 if not after prof_start().
__global__ void dev_pmu_start() {
    if (enable_pmu) {
        simple_pmu_start();
        hthread_printf("DEV: PMU started!\n");
    }
}

__global__ void dev_pmu_end() {
    if (enable_pmu) {
        simple_pmu_end();
        hthread_printf("DEV: PMU ended!\n");
    }
}