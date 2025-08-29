
#ifndef MT_DOUBLE_BUFFER_H
#define MT_DOUBLE_BUFFER_H

#include <stddef.h>
#include <stdint.h>

#include "hthread_device.h"

// 1MB for each core
// #define BUFFSIZE (1 * 1024 * 1024)
#define BUFFSIZE (1 << 22)

extern uint32_t *pmu_events;
extern uint32_t  pmu_num;
extern bool      enable_pmu;

// double buffer for records
extern void     *callback_buffer[24];    // 24 cores, 2 buffers for each core, 1MB for each buffer
extern void     *activity_buffer[24];    // 24 cores, 2 buffers for each core, 1MB for each buffer
extern uint32_t *callback_used_dev[24];  // free mem space in the callback "writing
// pool" (2 ints * 24)
extern uint32_t *activity_used_dev[24];  // free mem space in the activity "writing
                                         // pool" (2 ints * 24)

extern uint32_t local_cluster;

void *cb_buffer_alloc(size_t bytes);
void *ac_buffer_alloc(size_t bytes);


// false sharing ?
__global__ void buffer_init(uint32_t accl_dev_id, void *cb_buf, void *ac_buf, uint32_t *cb_used, uint32_t *ac_used);
__global__ void buffer_fin();
__global__ void dev_pmu_init(uint32_t enable_dev_pmu, uint32_t event_num, uint32_t *event_ids);

#endif