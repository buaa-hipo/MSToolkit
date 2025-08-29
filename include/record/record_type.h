#ifndef __JSI_RECORD_TYPE_H__
#define __JSI_RECORD_TYPE_H__

// #include <mpi.h>
#include <unordered_map>
#include <string>
// #include "instrument/backtrace.h"
#include "record/record_defines.h"

#ifdef USING_OMPT
#include <ompt.h>
#endif

#define MPI_UNKNOWN ((int16_t)(-1))
// #define JSI_PROCESS_START ((int16_t)(-2))
// #define JSI_PROCESS_EXIT ((int16_t)(-3))
#define JSI_PROCESS_START ((int16_t)event_PROCESS_START)
#define JSI_PROCESS_EXIT ((int16_t)event_PROCESS_EXIT)
#define JSI_TOOL_IO_MSGTYPE ((int16_t)(-4))
#define JSI_ACCL_ACTIVITY_EVENT ((int16_t)(-5))
#define JSI_ACCL_API_MSGTYPE_START ((int16_t)(-0x100))
#define JSI_GET_ACCL_API_MSGTYPE(x) (-((int16_t)(x))+JSI_ACCL_API_MSGTYPE_START)
#define JSI_GET_ACCL_API_ID_FROM_MSGTYPE(x) ((int16_t)((-(int16_t)(x))+JSI_ACCL_API_MSGTYPE_START))
#define JSI_MSG_IS_ACCL_API(x) (((int16_t)(x))<=JSI_ACCL_API_MSGTYPE_START && ((int16_t)(x))>=JSI_DEFAULT_FUNC_MSGTYPE)
#define JSI_DEFAULT_FUNC_MSGTYPE ((int16_t)(-0x1000))
#define JSI_GET_FUNC_MSGTYPE(x) ((int16_t)(-((int16_t)(x))+JSI_DEFAULT_FUNC_MSGTYPE))
#define JSI_MSG_IS_FUNC(x) ((x)<=JSI_GET_FUNC_MSGTYPE(0))
#define JSI_GET_FUNC_ID_FROM_MSGTYPE(x) ((int16_t)(-((int16_t)(x))+JSI_DEFAULT_FUNC_MSGTYPE))

#ifndef JSI_NO_PACKED
#pragma pack(push)
#pragma pack(1)
#endif

enum JSI_TRACE_HEADER {
    JSI_TRACE_DEFAULT = 0,
    JSI_TRACE_ACCL_ROCM,
    JSI_TRACE_ACCL_CUDA,
    JSI_TRACE_OMP,
    JSI_TRACE_HEADER_NUM
};

union symbol_key_t {
    struct {
        uint16_t domain;
        uint16_t op;
        uint32_t kind;
    } accl_key;
    uint64_t key;
};

inline const char* get_trace_base(JSI_TRACE_HEADER header) {
    switch(header) {
        case JSI_TRACE_ACCL_ROCM:
        case JSI_TRACE_ACCL_CUDA:
            return "accl_ativities.";
        case JSI_TRACE_OMP:
            return "omptrace.";
        default:
            return "trace.";
    }
    return "trace.";
}

#ifndef JSI_NO_PACKED
#pragma pack(8)
#endif

struct metric_t {
    uint64_t enter, exit;
};


 struct record_t {
    int16_t MsgType;
#ifdef ENABLE_BACKTRACE
    uint64_t ctxt;
#endif
    metric_t timestamps;
};

struct record_activity_t {
    record_t record;
    uint64_t correlation_id;
};

#ifndef JSI_NO_PACKED
#pragma pack(1)
#endif

typedef enum hip_mem_kind {
    HIP_ARRAY = 1,
    HIP_DEVICE = 2,
    HIP_HOST = 3
} hip_mem_kind;

struct record_activity_mem_alloc_t {
    record_t record;
    uint64_t correlation_id;
    const void* ptr;
    size_t sizeBytes;
    uint32_t kind;
    int32_t cpu_id;
    const void *stream;
};

struct record_activity_free_t {
    record_t record;
    uint64_t correlation_id;
    const void* ptr;
    size_t sizeBytes;
    uint32_t kind;
    const void *stream;
};

struct record_activity_launch_t {
    record_t record;
    uint64_t correlation_id;
    struct {
        uint32_t x,y,z;
    } blockNum;
    struct {
        uint32_t x,y,z;
    } blockDim;
    uint32_t sharedMemBytes;
    const void* stream;
};


struct record_activity_memcpy_async_t {
    record_t record;
    uint64_t correlation_id;
    const void* dst;
    const void* src;
    size_t sizeBytes;
    uint32_t kind;
    const void* stream;
};


struct record_activity_memcpy_t {
    record_t record;
    uint64_t correlation_id;
    const void* dst;
    const void* src;
    size_t sizeBytes;
    uint32_t kind;
};

struct record_activity_wait_t {
    record_t record;
    uint64_t correlation_id;
    const void *event;
    const void *stream;
};

struct record_activity_event_t {
    record_t record;
    uint64_t correlation_id;
    const void *event;
    const void *stream;
};

#ifndef JSI_NO_PACKED
#pragma pack(8)
#endif

struct mt_record_kernel_launch_t {
    record_t record;
    uint64_t correlation_id;
    int32_t cluster_id;
    uint32_t thread_num;
    uint32_t group_id;
    uint32_t thread_mask;
    uint32_t scalar_args_num;
    uint32_t ptr_args_num;
};

struct mt_record_driver_t {
    record_t record;
    uint64_t correlation_id;
    int32_t cluster_id;
    uint32_t kind;
};

struct mt_record_barrier_t {
    record_t record;
    uint64_t correlation_id;
    int32_t cluster_id;
    int32_t barrier_id;
    uint32_t kind;
};

struct mt_record_rwlock_t {
    record_t record;
    uint64_t correlation_id;
    int32_t cluster_id;
    int32_t lock_id;
    uint32_t kind;
};

struct mt_record_intr_t {
    record_t record;
    uint64_t correlation_id;
    uint32_t group_id;
    uint32_t thread_id;
    uint64_t intr_id;
    const void* func;
};

struct mt_record_group_wait_t {
    record_t record;
    uint64_t correlation_id;
    int32_t group_id;
};

struct mt_record_memcpy_t {
    record_t record;
    uint64_t correlation_id;
    const void *src;
    const void *dst;
    uint64_t bytes;
    uint32_t kind;
};

struct mt_record_memcpy_async_t {
    record_t record;
    uint64_t correlation_id;
    const void *src;
    const void *dst;
    uint64_t bytes;
    uint16_t kind;
    uint16_t dma_channel;
};

struct mt_record_malloc_t {
    record_t record;
    uint64_t correlation_id;
    int32_t cluster_id;
    uint32_t bytes;
    uint32_t mode;
    uint32_t kind;
    void *address;
};

struct mt_record_free_t {
  record_t record;
  uint64_t correlation_id;
  void *address;
};

struct mt_record_dma_wait_t {
    record_t record;
    uint64_t correlation_id;
    int32_t cluster_id;
    uint32_t dma_channel;
};

struct mt_record_dev_barrier_t {
    record_t record;
    uint64_t correlation_id;
    int32_t barrier_id;
    uint32_t core_num;
    uint64_t timeout;
};

struct mt_record_dev_rwlock_t {
    record_t record;
    uint64_t correlation_id;
    uint32_t lock_id;
    uint32_t op_kind;
};

struct mt_record_dev_intr_t {
    record_t record;
    uint64_t correlation_id;
    uint64_t intr_id;
    const void* func;
};

#ifndef DMA_CALLBACK
struct mt_record_kernel_t {
    record_t record;
    uint64_t correlation_id;
    uint16_t domain;
    uint16_t op;
    uint32_t kind;
};
#endif

#ifndef JSI_NO_PACKED
#pragma pack(1)
#endif

 struct record_comm_t {
    record_t record;
    uint64_t datatype; // compress
    int typesize;
	int dest;
	int tag;
    int count;      // compress
    uint64_t comm; //compress
};


 struct record_comm_rank_t {
    record_t record;
    uint64_t comm;
    int rank;
};


 struct record_comm_dup_t {
    record_t record;
    uint64_t comm;
    uint64_t new_comm;
};


 struct record_comm_split_t {
    record_t record;
    uint64_t comm;
    uint64_t new_comm;
    int color;
};


 struct record_all2all_t {
    record_t record;
    uint64_t datatype;
    int typesize;
	int sendcnt;
	int recvcnt;
    uint64_t comm;
};


 struct record_allreduce_t {
    record_t record;
    uint64_t datatype;
    int typesize;
	int count;
	uint64_t op;
    uint64_t comm;
};

 struct record_reduce_t {
    record_t record;
    uint64_t datatype;
    int typesize;
	int count;
    int root;
	uint64_t op;
    uint64_t comm;
};


 struct record_bcast_t {
    record_t record;
    uint64_t datatype;
    int typesize;
	int count;
    int root;
    uint64_t comm;
};


 struct record_barrier_t {
    record_t record;
    uint64_t comm;
};


 struct record_comm_async_t {
    record_t record;
    uint64_t datatype;
    int typesize;
	int dest;
	int tag;
    int count;
    uint64_t comm;
    uint64_t request;
};


 struct record_comm_wait_t {
    record_t record;
    uint64_t request;
};

// 通用压缩结构体

 struct record_compress_t {
    uint64_t datatype;
    int count;
    uint64_t comm;
    int32_t idx;
};



 struct record_comm_compress_t {
    record_t record;
    int dest;
    int tag;
    int typesize;
    size_t idx;
};


 struct record_all2all_compress_t {
    record_t record;
    int recvcnt;
    int typesize;
    size_t idx;
};


 struct record_allreduce_compress_t {
    record_t record;
    uint64_t op;
    int typesize;
    size_t idx;
};


 struct record_bcast_compress_t {
    record_t record;
    int root;
    int typesize;
    size_t idx;
};


 struct record_reduce_compress_t {
    record_t record;
    int root;
    int typesize;
    uint64_t op;
    size_t idx;
};



 struct record_comm_async_compress_t {
    record_t record;
    int dest;
    int tag;
    int typesize;
    uint64_t request;
    size_t idx;
};

/* ACCL TRACER RECORD TYPE */

 struct ext_record_accl {
    symbol_key_t sym_key;
    int correlation_id;
    uint64_t begin_ns;
    uint64_t end_ns;
};

struct record_memory_malloc {
    record_t record;
    const void* ptr;
    size_t size_bytes;
};

struct record_memory_calloc {
    record_t record;
    const void* ptr;
    size_t size_bytes;
};

struct record_memory_realloc {
    record_t record;
    const void* ptr;
    const void* newptr;
    size_t size_bytes;
};

struct record_memory_free {
    record_t record;
    const void* ptr;
};

struct record_memory_memalign{
    record_t record;
    const void* ptr;
    size_t alignment;
    size_t size_bytes;
};

struct record_memory_aligned_alloc{
    record_t record;
    const void* ptr;
    size_t alignment;
    size_t size_bytes;
};

struct record_memory_posix_memalign{
    record_t record;
    const void* ptr;
    size_t alignment;
    size_t size_bytes;
    int error_code;
};

#ifdef USING_OMPT
/* OPENMP TRACER RECORD TYPE */
struct ext_record_ompt_thread_begin_t {
    record_t record;
    uint64_t thread_id;
    ompt_thread_t thread_type;
};

struct ext_record_ompt_thread_end_t {
    record_t record;
    uint64_t thread_id;
};

struct ext_record_ompt_parallel_begin_t {
    record_t record;
    uint64_t thread_id;
    ompt_id_t encountering_task_id; 
    ompt_id_t parallel_id; 
    unsigned int requested_parallelism; 
    int flags; 
    const void *codeptr_ra;
};

struct ext_record_ompt_parallel_end_t {
    record_t record;
    uint64_t thread_id;
    ompt_id_t parallel_id; 
    ompt_id_t encountering_task_id; 
    int flags; 
    const void *codeptr_ra; 
};

struct ext_record_ompt_work_t {
    record_t record;
    uint64_t thread_id;
    ompt_work_t wstype; 
    ompt_scope_endpoint_t endpoint; 
    ompt_id_t parallel_id; 
    ompt_id_t task_id; 
    uint64_t count; 
    const void *codeptr_ra; 
};

struct ext_record_ompt_dispatch_t {
    record_t record;
    uint64_t thread_id;
    uint64_t parallel_id; 
    uint64_t task_id; 
    ompt_dispatch_t kind; 
    uint64_t instance;  
};

struct ext_record_ompt_task_create_t {
    record_t record;
    uint64_t thread_id;
    ompt_id_t encountering_task_id; 
    ompt_id_t new_task_id; 
    int flags; 
    int has_dependences; 
    const void *codeptr_ra;
};

struct ext_record_ompt_dependences_t {
    record_t record;
    uint64_t thread_id;
    ompt_id_t task_id; 
    uint64_t dep_variable; 
    ompt_dependence_type_t  dep_type;
    int ndeps;
};

struct ext_record_ompt_task_dependence_t {
    record_t record;
    uint64_t thread_id;
    ompt_id_t src_task_id; 
    ompt_id_t sink_task_id;
};

struct ext_record_ompt_task_schedule_t {
    record_t record;
    uint64_t thread_id;
    ompt_id_t prior_task_id; 
    ompt_task_status_t prior_task_status; 
    ompt_id_t next_task_id;
};

struct ext_record_ompt_implicit_task_t {
    record_t record;
    uint64_t thread_id;
    ompt_scope_endpoint_t endpoint; 
    ompt_id_t parallel_id; 
    ompt_id_t task_id; 
    unsigned int actual_parallelism; 
    unsigned int index; 
    int flags; 
};

struct ext_record_ompt_master_t {
    record_t record;
    uint64_t thread_id;
    ompt_scope_endpoint_t endpoint; 
    ompt_id_t parallel_id; 
    ompt_id_t task_id; 
    const void *codeptr_ra;
};

struct ext_record_ompt_sync_region_t {
    record_t record;
    uint64_t thread_id;
    ompt_sync_region_t kind; 
    ompt_scope_endpoint_t endpoint; 
    ompt_id_t parallel_id; 
    ompt_id_t task_id; 
    const void *codeptr_ra;
};

struct ext_record_ompt_mutex_acquire_t {
    record_t record;
    uint64_t thread_id;
    ompt_mutex_t kind; 
    unsigned int hint; 
    unsigned int impl; 
    ompt_wait_id_t wait_id; 
    const void *codeptr_ra; 
};

struct ext_record_ompt_mutex_t {
    record_t record;
    uint64_t thread_id;
    ompt_mutex_t kind; 
    ompt_wait_id_t wait_id; 
    const void *codeptr_ra;
};

struct ext_record_ompt_nest_lock_t {
    record_t record;
    uint64_t thread_id;
    ompt_scope_endpoint_t endpoint; 
    ompt_wait_id_t wait_id; 
    const void *codeptr_ra;
};

struct ext_record_ompt_flush_t {
    record_t record;
    uint64_t thread_id;
    const void *codeptr_ra;
};

struct ext_record_ompt_cancel_t {
    record_t record;
    uint64_t thread_id;
    ompt_id_t task_id; 
    int flags; 
    const void *codeptr_ra;
};

#else 

struct ext_record_ompt_thread_begin_t {
    record_t record;
};

struct ext_record_ompt_thread_end_t {
    record_t record;
};

struct ext_record_ompt_parallel_begin_t {
    record_t record;
};

struct ext_record_ompt_parallel_end_t {
    record_t record;
};

struct ext_record_ompt_work_t {
    record_t record;
};

struct ext_record_ompt_dispatch_t {
    record_t record;
};

struct ext_record_ompt_task_create_t {
    record_t record;
};

struct ext_record_ompt_dependences_t {
    record_t record;
};

struct ext_record_ompt_task_dependence_t {
    record_t record;
};

struct ext_record_ompt_task_schedule_t {
    record_t record;
};

struct ext_record_ompt_implicit_task_t {
    record_t record;
};

struct ext_record_ompt_master_t {
    record_t record;
};

struct ext_record_ompt_sync_region_t {
    record_t record;
};

struct ext_record_ompt_mutex_acquire_t {
    record_t record;
};

struct ext_record_ompt_mutex_t {
    record_t record;
};

struct ext_record_ompt_nest_lock_t {
    record_t record;
};

struct ext_record_ompt_flush_t {
    record_t record;
};

struct ext_record_ompt_cancel_t {
    record_t record;
};

#endif

struct record_pthread_create_t {
    record_t record;
    pthread_t thread;
    uint64_t thread_id;
    // pthread_attr_t *attr;
    void *(*start_routine)(void *); 
    void *arg;
};

struct record_pthread_join_t {
    record_t record;
    pthread_t thread;
    uint64_t thread_id;
};

struct record_pthread_detach_t {
    record_t record;
    pthread_t thread;
    uint64_t thread_id;
};

struct record_pthread_exit_t {
    record_t record;
    uint64_t thread_id;
};

struct record_pthread_mutex_init_t {
    record_t record;
    uint64_t thread_id;
};

struct record_pthread_mutex_destroy_t {
    record_t record;
    uint64_t thread_id;
};

struct record_pthread_mutex_lock_t {
    record_t record;
    uint64_t thread_id;
};

struct record_pthread_mutex_trylock_t {
    record_t record;
    uint64_t thread_id;
};

struct record_pthread_mutex_unlock_t {
    record_t record;
    uint64_t thread_id;
};

struct record_pthread_cond_init_t {
    record_t record;
    uint64_t thread_id;
};

struct record_pthread_cond_destroy_t {
    record_t record;
    uint64_t thread_id;
};

struct record_pthread_cond_wait_t {
    record_t record;
    uint64_t thread_id;
};

struct record_pthread_cond_timedwait_t {
    record_t record;
    uint64_t thread_id;
};

struct record_pthread_cond_signal_t {
    record_t record;
    uint64_t thread_id;
};

struct record_pthread_cond_broadcast_t {
    record_t record;
    uint64_t thread_id;
};

struct record_child_thread_start_t {
    record_t record;
    uint64_t thread_id;
    void *(*start_routine)(void *); 
    void *arg;
};

struct record_child_thread_finalize_t {
    record_t record;
    uint64_t thread_id;
};

#ifndef JSI_NO_PACKED
#pragma pack(pop)
#endif

#endif
