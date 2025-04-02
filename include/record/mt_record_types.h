
#ifndef MT_DEV_RECORD_TYPE_H
#define MT_DEV_RECORD_TYPE_H



#include <stdint.h>

// #define __RECORD_ATTR__ __attribute__((__packed__))

#ifdef __cplusplus
extern "C" {
#endif
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
} accl_record_common_t;

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
    int32_t cluster_id;
    uint32_t dma_channel;
} mt_record_dma_wait_t;

// record tags
typedef enum {
    ACCL_API_ENTER = 0,
    ACCL_API_EXIT = 1
} accl_api_phase_t;

// 48 bytes if no mem_async
// 64 bytes if included
typedef struct {
    accl_api_phase_t phase;
    uint64_t correlation_id;
    union {                             // todo: whether to use union
        struct {
            int32_t cluster_id;
            uint32_t bytes;
            uint32_t mode;              // 1 for RO, 2 for WO, 3 for RW, 4 for CACHE, 0 for mem free
            uint32_t kind;
        } mem_alloc;
        struct {
            int32_t cluster_id;
            uint32_t thread_num;
            uint32_t kernel_index;  // todo: is this necessary? index the user defined kernels with int32 ?
            uint32_t thread_mask;
            uint32_t scalar_args_num;
            uint32_t ptr_args_num;
            int32_t thread_group_id;
        } kernel_info;
        struct {                        // for vector/scalar load/store
            const void *dst;
            const void *src;
            uint64_t size_bytes;
            uint32_t kind;              // todo: how to find out if dst or src is HBM/GSM/DDR
        } mem_sync;
        struct {
            const void *dst;
            const void *src;
            uint64_t size_bytes;
            uint32_t kind;
            uint32_t dma_channel;
        } mem_async;
        struct {
            int32_t cluster_id;
            uint32_t dma_channel;
        } mem_wait;
        struct {
            const void *dst;
            const void *src;
            uint64_t size_bytes;
            uint32_t core_id;            // broadcast initiator
            uint32_t kind;
            uint32_t dma_channel;
        } mem_broadcast;
        struct {
            const void *dst;
            const void *src;
            uint64_t size_bytes;
            uint32_t core_start;        // core_id to receive the first segment
            uint32_t core_num;          // total core number in the dma_segment op
            uint32_t step;              // step length between member cores
            uint32_t kind;
            uint32_t dma_channel;
        } mem_segment;
        struct {
            const void *dst;
            const void *src;
            const void *index;
            uint64_t size_bytes;
            uint32_t kind;
            uint32_t dma_channel;
        } mem_sg;                       // scatter & gather dma op
    };
} matrix_api_data_t;

#ifdef FLEXIBLE_ACTIVITY_RECORD
// unified accl record type
typedef struct {
    uint16_t domain;      /* activity domain id */
    uint32_t kind; /* activity kind */
    uint16_t op;     /* activity op */
    struct {
        uint64_t correlation_id; /* activity ID */
        uint64_t begin_ns;           /* host begin timestamp */
        uint64_t end_ns;             /* host end timestamp */
    };
    union {
        struct {
            int device_id;     /* device id */
            uint64_t queue_id; /* queue id */
        };
        struct {
            uint32_t process_id; /* device id */
            uint32_t thread_id;  /* thread id */
        };
        struct {
            uint64_t external_id; /* external correlation id */
        };
    };
    union {
        uint64_t bytes;            /* data size bytes */
        const char* kernel_name; /* kernel name */
        const char* mark_message;
    };
} accl_activity_record_t;

#else
typedef struct {
    uint16_t domain;      /* activity domain id */
    uint32_t kind; /* activity kind */
    uint16_t op;     /* activity op */
    struct {
        uint64_t correlation_id; /* activity ID */
        uint64_t begin_ns;           /* host begin timestamp */
        uint64_t end_ns;             /* host end timestamp */
    };
    uint32_t process_id; /* device id */
    uint32_t thread_id;  /* thread id */
    union {
        uint64_t bytes;
        uint64_t kernel_index;      /* index of kernel in the list */
        const char* kernel_name;
        const char* mark_message;
    };
} accl_activity_record_t;

#endif

typedef union {
    struct {
        uint16_t domain;
        uint16_t op;
        uint32_t kind;
    } accl_key;
    uint64_t key;
} symbol_key_t;

typedef struct {
    symbol_key_t sym_key;
    int correlation_id;
    uint64_t begin_ns;
    uint64_t end_ns;
} ext_record_accl_t;

/**
 * @brief Category of dev apis.
 * 
 */
typedef enum {
    /// Host thread group apis
    ACCL_API_group_create,
    ACCL_API_group_create_masked_launch,
    ACCL_API_group_create_launch,
    ACCL_API_group_exec,
    ACCL_API_group_wait,
    ACCL_API_group_destroy,
    /// Dev memory apis on host side
    ACCL_API_malloc,
    ACCL_API_free,
    /// Dev memory/cache apis on dev side
    ACCL_API_vector_malloc,
    ACCL_API_vector_free,
    ACCL_API_scalar_malloc,
    ACCL_API_scalar_free,
    ACCL_API_hbm_malloc,
    ACCL_API_hbm_free,
    /// Dev memcpy apis
    ACCL_API_vector_load,
    ACCL_API_vector_store,
    ACCL_API_scalar_load,
    ACCL_API_scalar_store,
    ACCL_API_vector_load_async,
    ACCL_API_vector_store_async,
    ACCL_API_scalar_load_async,
    ACCL_API_scalar_store_async,
    ACCL_API_dma_p2p,
    ACCL_API_dma_broadcast,
    ACCL_API_dma_segment,
    ACCL_API_dma_sg,
    ACCL_API_dma_wait,
    /// Dev user function
    ACCL_USER_FUNC,
    /// ! not implemented
    ACCL_API_SHMalloc,
} accl_api_op_t;

/**
 * @brief Category of dev asynchronous operations.
 * 
 */
typedef enum {
    ACCL_ACTIVITY_kernel,
    ACCL_ACTIVITY_vector_load_async,
    ACCL_ACTIVITY_vector_store_async,
    ACCL_ACTIVITY_scalar_load_async,
    ACCL_ACTIVITY_scalar_store_async,
    ACCL_ACIVITY_dma_p2p,
    ACCL_ACIVITY_dma_broadcast,
    ACCL_ACIVITY_dma_segment,
    ACCL_ACIVITY_dma_sg,
    /// ! Not implemented
    ACCL_HOST_MEM,
    ACCL_HOST_THREAD,
    ACCL_HOST_DRIVER,
    ACCL_DEV_MEM,
    ACCL_DEV_THREAD,
    ACCL_DEV_DRIVER,
    ACCL_DMA,
    ACCL_DEV_USER_FUNC
} accl_api_kind_t;

/**
 * @brief Category of records.
 * 
 */
typedef enum {
    ACCL_DOMAIN_HTHREAD_API,    /// Records for hthread callback api
    ACCL_DOMAIN_HTHREAD_OPS,    /// Records for hthread activity api
    ACCL_DOMAIN_LIBMT,          /// ! (Not implemented) Records for libmt apis
    ACCL_DOMAIN_COMMON          /// Records for common api
} accl_api_domain_t;

typedef enum {
    ACCL_MALLOC_DDR,
    ACCL_MALLOC_HBM,
    ACCL_MALLOC_GSM,
    ACCL_MALLOC_AM,
    ACCL_MALLOC_SM,
    ACCL_FREE
} accl_malloc_kind_t;

typedef enum {                  // todo: maybe differentiate HBM/SHM/DDR 
    ACCL_MEMCPY_OFF_AM,
    ACCL_MEMCPY_OFF_SM,
    ACCL_MEMCPY_AM_OFF,
    ACCL_MEMCPY_SM_OFF,
    ACCL_MEMCPY_OFF_OFF
} accl_memcpy_kind_t;

extern void (*dev_api_callback)(uint32_t, uint32_t, const void *, void *);

#ifdef __cplusplus
}
#endif

#endif