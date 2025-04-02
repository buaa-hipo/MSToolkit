
#include "record/mt_double_buffer.h"
#include "hthread_device.h"
#include "record/mt_dev_types.h"
#include <stdint.h>
#include <stdio.h>

// double buffer for records
volatile void
    *callback_buffer; // 24 cores, 2 buffers for each core, 1MB for each buffer
volatile void
    *activity_buffer; // 24 cores, 2 buffers for each core, 1MB for each buffer
volatile uint32_t *callback_used_dev; // free mem space in the callback "writing
                                      // pool" (2 ints * 24)
volatile uint32_t *activity_used_dev; // free mem space in the activity "writing
                                      // pool" (2 ints * 24)

// pointer to share with host
volatile uint32_t
    *cb_writing_pool_index; // each bit stands for the index of pool currently
                            // being writen, modified by DEV
volatile uint32_t
    *cb_ready_pool_index; // each bit stands for the index of pool currently can
                          // be writen, modified by HOST
volatile uint32_t
    *ac_writing_pool_index; // each bit stands for the index of pool currently
                            // being writen, modified by DEV
volatile uint32_t
    *ac_ready_pool_index; // each bit stands for the index of pool currently can
                          // be writen, modified by HOST

void *__attribute__((optimize("0"))) cb_buffer_alloc(size_t bytes) {

  JSI_WARN("Allocating callback buffer - %lu\n", bytes);

  int core_id = get_core_id();
  uint32_t mask = 1 << core_id;
  if (callback_used_dev[core_id * 2 +
                        ((*cb_writing_pool_index & mask) >> core_id)] +
          bytes >
      BUFFSIZE) {
    if ((*cb_ready_pool_index & mask) == (*cb_writing_pool_index & mask))
      return NULL;
    *cb_writing_pool_index ^= mask;
    callback_used_dev[core_id * 2 +
                      ((*cb_writing_pool_index & mask) >> core_id)] = bytes;
    return (void *)((char *)callback_buffer + core_id * 2 * BUFFSIZE +
                    BUFFSIZE * ((*cb_writing_pool_index & mask) >> core_id));
  } else {

#ifdef DEBUG
    JSI_WARN("Current callback buffer is enough\n");
    JSI_WARN(
        "cb_ready_pool_index is %p[%d], cb_writing_pool_index is %p[%d]\n",
        cb_ready_pool_index, *cb_ready_pool_index, cb_writing_pool_index,
        *cb_writing_pool_index);
#endif
    void *block = (void *)((char *)callback_buffer + core_id * 2 * BUFFSIZE +
                           callback_used_dev[core_id * 2 +
                                             ((*cb_writing_pool_index & mask) >>
                                              core_id)]);
    callback_used_dev[core_id * 2 +
                      ((*cb_writing_pool_index & mask) >> core_id)] += bytes;
    return block;
  }
}

void *ac_buffer_alloc(size_t bytes) {
  int core_id = get_core_id();
  uint32_t mask = 1 << core_id;
  if (activity_used_dev[core_id * 2 +
                        ((*ac_writing_pool_index & mask) >> core_id)] +
          bytes >
      BUFFSIZE) {
    if ((*ac_ready_pool_index & mask) == (*ac_writing_pool_index & mask))
      return NULL;
    *ac_writing_pool_index ^= mask;
    activity_used_dev[core_id * 2 +
                      ((*ac_writing_pool_index & mask) >> core_id)] = bytes;
    return (void *)((char *)activity_buffer + core_id * 2 * BUFFSIZE +
                    BUFFSIZE * ((*ac_writing_pool_index & mask) >> core_id));
  } else {

#ifdef DEBUG
    JSI_WARN("Current activity buffer is enough\n");
    JSI_WARN(
        "ac_ready_pool_index is %p[%d], ac_writing_pool_index is %p[%d]\n",
        ac_ready_pool_index, *ac_ready_pool_index, ac_writing_pool_index,
        *ac_writing_pool_index);
#endif
    void *block = (void *)((char *)activity_buffer + core_id * 2 * BUFFSIZE +
                           activity_used_dev[core_id * 2 +
                                             ((*ac_writing_pool_index & mask) >>
                                              core_id)]);
    activity_used_dev[core_id * 2 +
                      ((*ac_writing_pool_index & mask) >> core_id)] += bytes;
    return block;
  }
}

/// used when cluster no longer needed
/// tag buffers so that host can read them
void flush_cb_buffer() { /// todo: problem in *cb_writing_pool_index ^= mask; ?
  int core_id = get_core_id();
  int size =
      callback_used_dev[core_id * 2 +
                        ((*cb_writing_pool_index & (1 << core_id)) >> core_id)];
#ifdef DEBUG
  hthread_printf("DEV - FLUSH from %p\n", callback_buffer);
  hthread_printf("\n\nDEV FLUSH\n");
  hthread_printf("size = %d\n", size);

  for (int i = 0; i < size; i += 4) {
    hthread_printf("DEV - %p: [%x]\n", ((char *)callback_buffer + i),
                   *((char *)callback_buffer + i));
    hthread_printf("DEV - %p: [%x]\n", ((char *)callback_buffer + i + 1),
                   *((char *)callback_buffer + i + 1));
    hthread_printf("DEV - %p: [%x]\n", ((char *)callback_buffer + i + 2),
                   *((char *)callback_buffer + i + 2));
    hthread_printf("DEV - %p: [%x]\n", ((char *)callback_buffer + i + 3),
                   *((char *)callback_buffer + i + 3));
  }
  hthread_printf("\nEND OF DEV FLUSH\n");
  fflush(stdout);
#endif
  if (get_thread_id() == 0) {
    if ((*cb_ready_pool_index) == (*cb_writing_pool_index))
      return;
    *cb_writing_pool_index = *cb_ready_pool_index;
  }
}

void flush_ac_buffer() {
  int core_id = get_core_id();
#ifdef DEBUG
  hthread_printf("FLUSH - ac_ready_pool_index is %p[%d], ac_writing_pool_index "
                 "is %p[%d]\n",
                 ac_ready_pool_index, *ac_ready_pool_index,
                 ac_writing_pool_index, *ac_writing_pool_index);
  fflush(stdout);
#endif
  if (get_thread_id() == 0) {
    if ((*ac_ready_pool_index) == (*ac_writing_pool_index))
      return;
    *ac_writing_pool_index = *ac_ready_pool_index;
  }
}

/// Busy wait until each bit of ready and writing is different (last 24 bits)
void flush_wait() {
  bool flag = true;
  while (flag) {
    flag = false;
    for (int i = 0; i < 24; ++i) {
      /// If two bits in ready and writing are the same
      /// Which means host has not collect buffer data
      if (((*cb_ready_pool_index) & (1 << i)) ==
          ((*cb_writing_pool_index) & (1 << i))) {
        flag = true;
      }
    }
#ifdef DEBUG
    hthread_printf("WAIT - cb_ready_pool_index is %p[%d], "
                   "cb_writing_pool_index is %p[%d] - %llu\n",
                   cb_ready_pool_index, *cb_ready_pool_index,
                   cb_writing_pool_index, *cb_writing_pool_index, get_clk());
#endif
  }
  flag = true;
  while (flag) {
    flag = false;
    for (int i = 0; i < 24; ++i) {
      /// If two bits in ready and writing are the same
      /// Which means host has not collect buffer data
      if (((*ac_ready_pool_index) & (1 << i)) ==
          ((*ac_writing_pool_index) & (1 << i))) {
        flag = true;
      }
    }
#ifdef DEBUG
    hthread_printf("WAIT - ac_ready_pool_index is %p[%d],    "
                   "ac_writing_pool_index is %p[%d] - %llu\n",
                   ac_ready_pool_index, *ac_ready_pool_index,
                   ac_writing_pool_index, *ac_writing_pool_index, get_clk());
#endif
  }
#ifdef DEBUG
  while ((*cb_ready_pool_index) == (*cb_writing_pool_index)) {
    hthread_printf("cb waiting %llu\n", get_clk());
  }
  fflush(stdout);
  while ((*ac_ready_pool_index) == (*ac_writing_pool_index)) {
    hthread_printf("ac waiting %llu\n", get_clk());
  }
  fflush(stdout);
#endif
}

__global__ __attribute__((optimize("0"))) void
double_buffer_init(uint32_t accl_dev_id, void *cb_buf, void *ac_buf,
                   uint32_t *cb_used, uint32_t *ac_used, uint32_t *flags) {
  JSI_LOG(JSILOG_INFO, "Double buffer initializing\n");
  callback_buffer = cb_buf;
  activity_buffer = ac_buf;
  callback_used_dev = cb_used;
  activity_used_dev = ac_used;
#ifdef DEBUG
  hthread_printf("DEV - Callback buffer starts at %p\n", callback_buffer);
  hthread_printf("DEV - Activity buffer starts at %p\n", activity_buffer);
  fflush(stdout);
#endif
  for (int i = 0; i < 24; ++i) {
    callback_used_dev[i * 2] = 0;
    callback_used_dev[i * 2 + 1] = 0;
    activity_used_dev[i * 2] = 0;
    activity_used_dev[i * 2 + 1] = 0;
  }
  cb_writing_pool_index = flags;
  cb_ready_pool_index = flags + 1;
  ac_writing_pool_index = flags + 2;
  ac_ready_pool_index = flags + 3;
  *cb_writing_pool_index = 0x0;
  *cb_ready_pool_index = 0x00ffffff;
  *ac_writing_pool_index = 0x0;
  *ac_ready_pool_index = 0x00ffffff;
  local_cluster = accl_dev_id;
  hthread_printf("Double buffer initialized\n");
}

__global__ __attribute__((optimize("0"))) void double_buffer_fin() {
  hthread_printf("Double buffer finalizing...\n");
  flush_cb_buffer();
  flush_ac_buffer();
  flush_wait();
  hthread_printf("Double buffer finalized\n");
}

__global__ __attribute__((optimize("0"))) void
dev_pmu_init(uint32_t event_num, uint32_t *event_ids) {
  hthread_printf("In dev pmu init kernel\n");
  fflush(stdout);
  pmu_num = event_num;
  pmu_events = event_ids;
  hthread_printf("Dev pmu initialized\n");
  fflush(stdout);
}