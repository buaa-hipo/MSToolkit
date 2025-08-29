//
// Created by xiaox on 2024/12/15.
//
#include "record/accl_tracer.h"

// #include <sys/types.h>

#include <array>
#include <cstdint>
// #include <iostream>

// #include "record/record_utils.h"
#include "record/record_writer.h"

std::array<void *, 24>     callback_buffers   = {};
std::array<void *, 24>     activity_buffers   = {};
std::array<uint32_t *, 24> callback_used_host = {};
std::array<uint32_t *, 24> activity_used_host = {};

uint32_t opened_cluster;

static size_t total_ext_trace_size = 0;

static void write_to_file(void *src, size_t size, int core_id) {
    if (opened_cluster < 0 || opened_cluster > 3) {
        JSI_WARN("Opened cluster invalid!\n");
        return;
    }
    if (size == 0 || size > (size_t)(JSI_DEV_BUFFER_SIZE)) return;
    // JSI_LOG(LOG_LEVEL, "Writing to device ext trace file, size is %lu\n", size);
    total_ext_trace_size += size;
    RecordWriter::extStore(opened_cluster * 24 + core_id, src, size);
    // JSI_LOG(LOG_LEVEL, "Device ext trace file written\n");
}

AcclTracer::AcclTracer(int ring_size) : ring_size_(ring_size) {
    buffer_node_valid_.resize(ring_size_);
    ring_buffer_.resize(ring_size_);
}

void AcclTracer::flush() {
    for (int i = 0; i < 24; ++i) {
        write_to_file(callback_buffers[i], *callback_used_host[i], i);
    }
}

AcclTracer::~AcclTracer() {
    JSI_INFO("Total size of ext trace is now %lu\n", total_ext_trace_size);
}

void AcclTracer::RegisterCallback(void (*callback)(uint32_t, uint32_t, const void *, void *)) {
    api_callback_ = callback;
}

inline const char *accl_get_hthread_api_name(uint32_t op) {
    switch (op) {
        case ACCL_API_group_create:
            return "hthread_group_create2";
        case ACCL_API_group_create_launch:
            return "hthread_group_create6";
        case ACCL_API_group_create_masked_launch:
            return "hthread_group_masked_create";
        case ACCL_API_group_exec:
            return "hthread_group_exec";
        case ACCL_API_group_destroy:
            return "hthread_group_destroy";
        case ACCL_API_group_wait:
            return "hthread_group_wait";
        case ACCL_API_malloc:
            return "hthread_malloc";
        case ACCL_API_free:
            return "hthread_free";
        case ACCL_API_vector_malloc:
            return "vector_malloc";
        case ACCL_API_vector_free:
            return "vector_free";
        case ACCL_API_scalar_malloc:
            return "scalar_malloc";
        case ACCL_API_scalar_free:
            return "scalar_free";
        case ACCL_API_hbm_malloc:
            return "hbm_malloc";
        case ACCL_API_hbm_free:
            return "hbm_free";
        case ACCL_API_vector_load:
            return "vector_load";
        case ACCL_API_vector_store:
            return "vector_store";
        case ACCL_API_scalar_load:
            return "scalar_load";
        case ACCL_API_scalar_store:
            return "scalar_store";
        case ACCL_API_vector_load_async:
            return "vector_load_async";
        case ACCL_API_vector_store_async:
            return "vector_store_async";
        case ACCL_API_scalar_load_async:
            return "scalar_load_async";
        case ACCL_API_scalar_store_async:
            return "scalar_store_async";
        case ACCL_API_dma_p2p:
            return "dma_p2p";
        case ACCL_API_dma_broadcast:
            return "dma_broadcast";
        case ACCL_API_dma_segment:
            return "dma_segment";
        case ACCL_API_dma_sg:
            return "dma_sg";
        case ACCL_API_dma_wait:
            return "dma_wait";
        case ACCL_API_dat_load:
            return "hthread_dat_load";
        case ACCL_API_dat_unload:
            return "hthread_dat_unload";
        case ACCL_API_dev_open:
            return "hthread_dev_open";
        case ACCL_API_dev_close:
            return "hthread_dev_close";
        case ACCL_API_barrier_create:
            return "hthread_barrier_create";
        case ACCL_API_barrier_destroy:
            return "hthread_barrier_destroy";
        default:
            return NULL;
    }
}

const char *accl_get_hthread_op_name(uint32_t kind, uint32_t op) {
#ifdef DEBUG
    if (kind == ACCL_ACTIVITY_kernel) {
        std::cout << "\nSearching for kernel name" << std::endl;
        std::cout << "op is " << op << std::endl;
        std::cout << "size of kernel_index vector is " << kernel_index.size() << std::endl;
        std::cout << "Got kernel name: " << kernel_index[op] << std::endl;
        std::cout << "Name length: " << kernel_index[op].length() << std::endl;
    }
#endif
    switch (kind) {
        case ACCL_ACTIVITY_kernel:
            return kernel_index[op].c_str();
        default:
            return "";
    }
}

const char *accl_get_mt_op_string(uint32_t kind, uint32_t op) {
    return "";
}

/// Order of the global variables should not be changed.
/// Or move kernel_index vector into tracer.
std::vector<std::string> kernel_index;

AcclTracer tracer(100);
