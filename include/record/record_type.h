#ifndef __JSI_RECORD_TYPE_H__
#define __JSI_RECORD_TYPE_H__

// #include <mpi.h>
#include <unordered_map>
#include <string>
#include "instrument/backtrace.h"
#include "record/record_defines.h"

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
        default:
            return "trace.";
    }
    return "trace.";
}

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


#ifndef JSI_NO_PACKED
#pragma pack(pop)
#endif

#endif
