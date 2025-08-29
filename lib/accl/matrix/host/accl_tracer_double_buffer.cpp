//
// Created by xiaox on 2024/12/15.
//
#include "record/accl_tracer.h"

#include "record/record_writer.h"
// #include "record/mt_record_types.h"
#include <sys/types.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <set>
#include <thread>

#include "utils/safe.hpp"

std::array<std::array<std::array<void *, 2>, 24>, 4>     callback_buffers         = {};
std::array<std::array<std::array<void *, 2>, 24>, 4>     activity_buffers         = {};
std::array<std::array<uint32_t *, 24>, 4>                callback_writing_indices = {};
std::array<std::array<uint32_t *, 24>, 4>                callback_ready_indices   = {};
std::array<std::array<uint32_t *, 24>, 4>                activity_writing_indices = {};
std::array<std::array<uint32_t *, 24>, 4>                activity_ready_indices   = {};
std::array<std::array<std::array<uint32_t *, 2>, 24>, 4> callback_used_host       = {};
std::array<std::array<std::array<uint32_t *, 2>, 24>, 4> activity_used_host       = {};
std::array<uint32_t *, 4>                                host_flush_flag          = {};

uint32_t opened_cluster;

void write_to_file(char *src, size_t size, int cluster_id,
                   int core_id) {  ///< TODO: write callback records to file.
    JSI_LOG(LOG_LEVEL, "Writing to device ext trace file, size is %lu\n", size);
    if (size == 0 || size > JSI_DEV_BUFFER_SIZE) return;
    RecordWriter::extStore(cluster_id * 24 + core_id, src, size);
    JSI_LOG(LOG_LEVEL, "Device ext trace file written\n");
}

static void collect_from_callback_buffer(const int cluster_id, const int core_id) {
    if (callback_ready_indices[cluster_id][core_id] == nullptr) {
        return;
    }

    const auto writing_index = *callback_writing_indices[cluster_id][core_id];
    const auto ready_index   = *callback_ready_indices[cluster_id][core_id];

    // printf("Callback buffer used on Cluster-%d Core-%d: %p[%u], %p[%u]\n", cluster_id, core_id,
    //        callback_used_host[cluster_id][core_id][1 - writing_index],
    //        *callback_used_host[cluster_id][core_id][1 - writing_index],
    //        callback_used_host[cluster_id][core_id][writing_index],
    //        *callback_used_host[cluster_id][core_id][writing_index]);
    // printf("Content of callback_buffers[%d][%d][0]: \n", cluster_id, core_id);
    // for (int i = 0; i < 64; ++i) {
    //     auto content = reinterpret_cast<uint32_t *>(callback_buffers[cluster_id][core_id][0]);
    //     printf("%x %x %x %X\n", content[0], content[1], content[2], content[3]);
    // }

    if (writing_index != ready_index) return;

    auto src = static_cast<char *>(callback_buffers[cluster_id][core_id][1 - writing_index]);
    // std::cout << "Got something from callback buffer of core " << cluster_id << " - " << core_id << std::endl;

    auto size = *callback_used_host[cluster_id][core_id][1 - writing_index];
    // printf("SIZE is %u\n", size);
    /// TODO: is this operation safe ?
    write_to_file(src, size, cluster_id, core_id);
    /// clear buffer and set bytes used to 0
    *callback_used_host[cluster_id][core_id][1 - writing_index] = 0;

    *callback_ready_indices[cluster_id][core_id] = 1 - *callback_ready_indices[cluster_id][core_id];
    // std::cout << "callback_ready_indices[" << cluster_id << "] toggled to "
    //           << *callback_ready_indices[cluster_id][core_id] << std::endl;
}

// ! Not maintained
static void collect_from_activity_buffer(const int cluster_id, const int core_id) {
    if (activity_ready_indices[cluster_id][core_id] == nullptr) {
        return;
    }

#ifdef DEBUG
    printf("Cluster-%d Core-%d activity_writing_indices: %d\n", cluster_id, core_id,
           *activity_writing_indices[cluster_id]);
#endif

    const uint32_t writing_index = *activity_writing_indices[cluster_id][core_id];
    const uint32_t ready_index   = *activity_ready_indices[cluster_id][core_id];

    if (writing_index != ready_index) return;

#ifdef DEBUG
    printf("activity buffer used on Cluster-%d Core-%d: %d\n", cluster_id, core_id,
           activity_used_host[cluster_id][core_id * 2 + 1 - writing_index]);
#endif

    auto src = static_cast<char *>(activity_buffers[cluster_id][core_id][1 - writing_index]);
    std::cout << "Got something from activity buffer of core " << cluster_id << " - " << core_id << std::endl;

    auto size = *activity_used_host[cluster_id][core_id][1 - writing_index];
    printf("SIZE is %u\n", size);
    /// TODO: is this operation safe ?
    write_to_file(src, size, cluster_id, core_id);
    /// clear buffer and set bytes used to 0
    *activity_used_host[cluster_id][core_id][1 - writing_index] = 0;

    *activity_ready_indices[cluster_id][core_id] = 1 - *activity_ready_indices[cluster_id][core_id];
    std::cout << "activity_ready_indices[" << cluster_id << "] toggled to "
              << *activity_ready_indices[cluster_id][core_id] << std::endl;
}

static void flush_from_dev(int cluster_id) {
    std::cout << "Begin flush from dev" << std::endl;
    for (int core_id = 0; core_id < 24; ++core_id) {
        if (callback_ready_indices[cluster_id][core_id] == nullptr) {
            continue;
        }
        const uint32_t writing_index = *callback_writing_indices[cluster_id][core_id];
        const uint32_t ready_index   = *callback_ready_indices[cluster_id][core_id];

        if (writing_index == ready_index) {
            auto src  = static_cast<char *>(callback_buffers[cluster_id][core_id][1 - writing_index]);
            auto size = *callback_used_host[cluster_id][core_id][1 - writing_index];
            write_to_file(src, size, cluster_id, core_id);
        }
        auto src  = static_cast<char *>(callback_buffers[cluster_id][core_id][writing_index]);
        auto size = *callback_used_host[cluster_id][core_id][writing_index];
        write_to_file(src, size, cluster_id, core_id);
    }
}

static void ConsumerThreadLoop() {
    jsi_safe_enter_instr();
    std::cout << "Begin of consumer thread" << std::endl;

    while (tracer.stop_flag_.load()) {
        for (int core_id = 0; core_id < 24; ++core_id) {
            collect_from_callback_buffer(opened_cluster, core_id);
            // ! activity buffer not maintained
            // collect_from_activity_buffer(cluster_id, core_id);
        }
        if (opened_cluster >= 0 && opened_cluster < 4 && host_flush_flag[opened_cluster] != nullptr) {
            // JSI_WARN("host_flush_flag of cluster %d is %d\n", opened_cluster, *host_flush_flag[opened_cluster]);
            // fflush(stderr);
            if (*host_flush_flag[opened_cluster] != 0) {
                flush_from_dev(opened_cluster);
                *host_flush_flag[opened_cluster] = 0;
                host_flush_flag[opened_cluster]  = nullptr;
                JSI_WARN("Cluster %d flushed!\n", opened_cluster);
                fflush(stderr);
            }
        }
    }
    // exit
}

AcclTracer::AcclTracer(int ring_size) : ring_size_(ring_size), pool_(nullptr) {
    buffer_node_valid_.resize(ring_size_);
    ring_buffer_.resize(ring_size_);
    // for (auto &buffer : callback_buffers) buffer.fill(nullptr);
    // for (auto &buffer : activity_buffers) buffer.fill(nullptr);
    // callback_writing_indices.fill(nullptr);
    // callback_ready_indices.fill(nullptr);
    // activity_writing_indices.fill(nullptr);
    // activity_ready_indices.fill(nullptr);
    // callback_used_host.fill(nullptr);
    // activity_used_host.fill(nullptr);
    stop_flag_.store(true);
}

void AcclTracer::MemoryPoolInit(buffer_pool_property_t *property) {
    pool_ = new accl_activity_pool::MemoryPool(*property);
}

AcclTracer::~AcclTracer() {
    delete pool_;
}

void AcclTracer::RegisterCallback(void (*callback)(uint32_t, uint32_t, const void *, void *)) {
    api_callback_ = callback;
}

void AcclTracer::StartTracing() {
    consumer_thread_ = std::thread(ConsumerThreadLoop);
}

void AcclTracer::StopTracing() {
    stop_flag_.store(false);
    consumer_thread_.join();
}

void AcclTracer::FlushPool() const {
    pool_->Flush();
}

const char *accl_get_hthread_api_name(uint32_t op) {
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
            return nullptr;
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
