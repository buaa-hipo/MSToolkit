//
// Created by xiaox on 2024/5/4.
//

#ifndef JSI_TOOLKIT_ACCL_TRACER_H
#define JSI_TOOLKIT_ACCL_TRACER_H

#include <cstdint>
#include <set>
#include <thread>
#include <vector>

#define ACCL_TRACER_INITIALIZE_PRIORITY (RECORD_INIT_PRIORITY + 50)
#define ACCL_TRACER_FINALIZE_PRIORITY   (RECORD_INIT_PRIORITY + 50)

#define MAX_PMU_SIZE 10

#define JSI_DEV_BUFFER_SIZE (1024)

// #ifdef MATRIX

// #include "mt_record_types.h"
#include <array>

#include "mt_callback_defs.h"
#include "mt_memory_pool.h"

extern std::array<std::array<std::array<void *, 2>, 24>, 4>     callback_buffers;
extern std::array<std::array<std::array<void *, 2>, 24>, 4>     activity_buffers;
extern std::array<std::array<uint32_t *, 24>, 4>                callback_writing_indices;
extern std::array<std::array<uint32_t *, 24>, 4>                callback_ready_indices;
extern std::array<std::array<uint32_t *, 24>, 4>                activity_writing_indices;
extern std::array<std::array<uint32_t *, 24>, 4>                activity_ready_indices;
extern std::array<std::array<std::array<uint32_t *, 2>, 24>, 4> callback_used_host;
extern std::array<std::array<std::array<uint32_t *, 2>, 24>, 4> activity_used_host;
extern std::array<uint32_t *, 4>                                host_flush_flag;

extern uint32_t opened_cluster;

// #endif

enum accl_status_t {
    ACCL_STATUS_SUCCESS                                  = 0,
    ACCL_STATUS_ERROR                                    = -1,
    ACCL_STATUS_ERROR_INVALID_DOMAIN_ID                  = -2,
    ACCL_STATUS_ERROR_INVALID_ARGUMENT                   = -3,
    ACCL_STATUS_ERROR_DEFAULT_POOL_UNDEFINED             = -4,
    ACCL_STATUS_ERROR_DEFAULT_POOL_ALREADY_DEFINED       = -5,
    ACCL_STATUS_ERROR_MEMORY_ALLOCATION                  = -6,
    ACCL_STATUS_ERROR_MISMATCHED_EXTERNAL_CORRELATION_ID = -7,
    ACCL_STATUS_ERROR_NOT_IMPLEMENTED                    = -8,
};

// think about dynamic pmu array length
struct BufferNode {
    uint64_t timestamp;
    uint64_t pmu[MAX_PMU_SIZE];
};

class AcclTracer {

    // bool enable_backtrace_;
    // bool enable_pmu_;

public:
    const int ring_size_;
    // bool buffer_node_valid_[ring_size_];
    std::vector<uint8_t>    buffer_node_valid_;
    std::vector<BufferNode> ring_buffer_;
    std::vector<uint32_t>   device_pmu_events;

    accl_activity_pool::MemoryPool *pool_;
    std::thread                     consumer_thread_;
    std::atomic<bool>               stop_flag_;
    // std::function<void(uint32_t, uint32_t, const void*, void*)> api_callback_;
    void (*api_callback_)(uint32_t, uint32_t, const void *, void *);

    // bool EnabledPMU() const { return enable_pmu_; }
    // bool EnabledBacktrace() const { return enable_backtrace_; }

    explicit AcclTracer(int ring_size = 100);
    ~AcclTracer();
    void StartTracing();
    void StopTracing();
    void MemoryPoolInit(buffer_pool_property_t *property);
    // void RegisterCallback(const std::function<void(uint32_t, uint32_t, const void*, void*)>&);
    void RegisterCallback(void (*)(uint32_t, uint32_t, const void *, void *));
    void FlushPool() const;
};

inline accl_status_t accl_next_record(const accl_activity_record_t *cur, const accl_activity_record_t **next) {
    if (cur == nullptr) return ACCL_STATUS_ERROR;
    // try {

    // }
    // catch (std::exception& e) {
    //     JSI_ERROR("error detected when finding next records");    // todo: need modifying
    // }
    *next = cur + 1;  // todo: maybe not safe ?
    return ACCL_STATUS_SUCCESS;
}

// #ifdef MATRIX

extern std::vector<std::string> kernel_index;

/// Get api name string from api kind and operation.
/// For kernel launch, return kernel name.
/// For asynchronous memory copy, return api name.
const char *accl_get_hthread_api_name(uint32_t);
const char *accl_get_hthread_op_name(uint32_t, uint32_t);

// ! Not implemented
const char *accl_get_mt_op_string(uint32_t, uint32_t);

// #endif

inline const char *accl_op_string(uint32_t domain, uint32_t op, uint32_t kind) {
    switch (domain) {
        case ACCL_DOMAIN_HTHREAD_API:
            return accl_get_hthread_api_name(op);
        case ACCL_DOMAIN_HTHREAD_OPS:
            return accl_get_hthread_op_name(kind, op);
        case ACCL_DOMAIN_LIBMT:
            return accl_get_mt_op_string(kind, op);
        default:
            return "";
    }
}

extern AcclTracer tracer;

#endif  // JSI_TOOLKIT_ACCL_TRACER_H
