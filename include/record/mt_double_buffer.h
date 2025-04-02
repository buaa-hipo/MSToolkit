
#ifndef MT_DOUBLE_BUFFER_H
#define MT_DOUBLE_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include "hthread_device.h"

// 1MB for each core
#define BUFFSIZE (1 * 1024 * 1024)

extern uint32_t *pmu_events;
extern uint32_t pmu_num;

// double buffer for records
extern volatile void * callback_buffer;      // 24 cores, 2 buffers for each core, 1MB for each buffer
extern volatile void * activity_buffer;      // 24 cores, 2 buffers for each core, 1MB for each buffer
extern volatile uint32_t *callback_used_dev;       // used mem space in the callback "writing pool" (2 * 24)
extern volatile uint32_t *activity_used_dev;       // used mem space in the activity "writing pool" (2 * 24)

// pointer to share with host
extern volatile uint32_t *cb_writing_pool_index;// each bit stands for the index of pool currently being writen, modified by DEV
extern volatile uint32_t *cb_ready_pool_index;  // each bit stands for the index of pool currently can be writen, modified by HOST
extern volatile uint32_t *ac_writing_pool_index;// each bit stands for the index of pool currently being writen, modified by DEV
extern volatile uint32_t *ac_ready_pool_index;  // each bit stands for the index of pool currently can be writen, modified by 

extern uint32_t local_cluster;

void * cb_buffer_alloc(size_t bytes);
void * ac_buffer_alloc(size_t bytes);
void flush_cb_buffer();
void flush_ac_buffer();
void flush_wait();

// false sharing ?
__global__ void double_buffer_init(uint32_t accl_dev_id, void *cb_buf, void *ac_buf, uint32_t *cb_used, uint32_t *ac_used, uint32_t *flags);
__global__ void double_buffer_fin();
__global__ void dev_pmu_init(uint32_t event_num, uint32_t *event_ids);

#endif