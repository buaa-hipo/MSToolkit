//
// Created by xiaox on 2025/1/12.
//

#ifndef MT_DEV_TYPES_H
#define MT_DEV_TYPES_H

#define __JSI_WARN__ "=== JSI Warning ==="
#define __JSI_ERROR__ "=== JSI Error ==="
#define __JSI_INFO__ "=== JSI Info ==="

typedef enum {
    JSILOG_ALWAYS=0,
    JSILOG_WARN,
    JSILOG_INFO,
    JSILOG_DEBUG,
    JSILOG_ALL
}JSILOG_LEVEL;

#define LOG_LEVEL JSILOG_WARN
#define JSI_LOG(level, format...) do { if(level<=LOG_LEVEL) hthread_printf("[JSILOG] " format); } while(0)

#define JSI_INFO(format...) JSI_LOG(JSILOG_INFO, format)
#define JSI_WARN(format...) JSI_LOG(JSILOG_WARN, format)
#define JSI_ERROR(format...) do { \
    hthread_printf(__JSI_ERROR__ format); \
    exit(-1); \
} while(0)

#include <stdint.h>

typedef union {
  struct {
    uint16_t domain;
    uint16_t op;
    uint32_t kind;
  } accl_key;
  uint64_t key;
} symbol_key_t;

typedef struct {
  uint64_t enter, exit;
} metric_t;

typedef struct {
  int16_t MsgType;
#ifdef ENABLE_BACKTRACE
  uint64_t ctxt;
#endif
  metric_t timestamps;
} record_header_t;

typedef struct {
  record_header_t record;
  uint64_t correlation_id;
} record_activity_t;

typedef struct {
  record_header_t record;
  uint64_t correlation_id;
  int32_t cluster_id;
  uint32_t thread_num;
  uint32_t thread_mask;
  uint32_t scalar_args_num;
  uint32_t ptr_args_num;
} mt_record_kernel_launch_t;

typedef struct {
  record_header_t record;
  uint64_t correlation_id;
  const void *src;
  const void *dst;
  uint64_t bytes;
  uint32_t kind;
} mt_record_memcpy_t;

typedef struct {
  record_header_t record;
  uint64_t correlation_id;
  const void *src;
  const void *dst;
  uint64_t bytes;
  uint16_t kind;
  uint16_t dma_channel;
} mt_record_memcpy_async_t;

typedef struct {
  record_header_t record;
  uint64_t correlation_id;
  int32_t cluster_id;
  uint32_t bytes;
  uint32_t mode;
  uint32_t kind;
} mt_record_malloc_t;

typedef struct {
  record_header_t record;
  uint64_t correlation_id;
  void *address;
} mt_record_free_t;

typedef struct {
  record_header_t record;
  uint64_t correlation_id;
  int32_t cluster_id;
  uint32_t dma_channel;
} mt_record_dma_wait_t;

typedef struct {
  record_header_t record;
  uint64_t correlation_id;
  int32_t barrier_id;
  uint32_t core_num;
  uint64_t timeout;
} mt_record_dev_barrier_t;

typedef struct {
  record_header_t record;
  uint64_t correlation_id;
  uint32_t lock_id;
  uint32_t op_kind;
} mt_record_dev_rwlock_t;

typedef struct {
  record_header_t record;
  uint64_t correlation_id;
  uint64_t intr_id;
  const void* func;
} mt_record_dev_intr_t;

#ifndef DMA_CALLBACK
typedef struct {
  record_header_t record;
  uint64_t correlation_id;
  uint16_t domain;
  uint16_t op;
  uint32_t kind;
} mt_record_kernel_t;
#endif

extern void (*dev_api_callback)(uint32_t, uint32_t, const void *, void *);

#endif // MT_DEV_TYPES_H
