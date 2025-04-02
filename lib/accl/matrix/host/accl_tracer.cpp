//
// Created by xiaox on 2024/12/15.
//
#include "record/accl_tracer.h"

#include "record/record_writer.h"
// #include "record/mt_record_types.h"
#include <sys/types.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#ifdef DEBUG
#include <iostream>
#endif

std::array<std::array<void *, 24>, 4> callback_buffers;          ///< Callback buffer pools of each cluster.
std::array<std::array<void *, 24>, 4> activity_buffers;          ///< First activity buffer pools of each cluster.
std::array<uint32_t *, 4>             callback_writing_indices;  ///< Indices of callback buffer currently being
                                                                 ///< written to (bitmap).
std::array<uint32_t *, 4> callback_ready_indices;                ///< Indices of callback buffer currently ready
                                                                 ///< (bitmap).
std::array<uint32_t *, 4> activity_writing_indices;              ///< Indices of activity buffer currently being
                                                                 ///< written to (bitmap).
std::array<uint32_t *, 4> activity_ready_indices;                ///< Indices of activity buffer currently ready
                                                                 ///< (bitmap).
std::array<uint32_t *, 4> callback_used_host;                    ///< Used callback buffer size (in bytes), length=48.
std::array<uint32_t *, 4> activity_used_host;                    ///< Used activity buffer size (in bytes), length=48.

void write_to_file(char *src, size_t size, int cluster_id,
                   int core_id) {  ///< TODO: write callback records to file.
    JSI_LOG(LOG_LEVEL, "Writing to device ext trace file, size is %lu\n", size);
#ifdef USE_SIMPLE_FILE
    int         pid       = getpid();
    std::string file_name = "./dev_trace_" + std::to_string(pid) + "_" + std::to_string(cluster_id) + "_"
                            + std::string(((core_id >= 10) ? 0 : 1), '0') + std::to_string(core_id) + ".tr";
    printf("Opening trace file: %s\n", file_name.c_str());
    FILE *file = fopen(file_name.c_str(), "ab");
    fwrite(src, size, 1, file);
    fclose(file);
    printf("Storing ext trace for process-%d, cluster-%d, core-%d\n", pid, cluster_id, core_id);
#endif
    RecordWriter::extStore(cluster_id * 24 + core_id, src, size);
    JSI_LOG(LOG_LEVEL, "Device ext trace file written\n");
}

static void collect_from_callback_buffer(const int cluster_id, const int core_id) {
    if (callback_ready_indices[cluster_id] == nullptr) {
        // printf("Callback buffer of Cluster-%d Core-%d is not initialized\n",
        // cluster_id, core_id);
        return;
    }

#ifdef DEBUG
    printf("Cluster-%d Core-%d callback_writing_indices: %d\n", cluster_id, core_id,
           *callback_writing_indices[cluster_id]);
#endif

    /// Index of buffer currently being written
    const uint32_t writing_index = ((1 << core_id) & (*callback_writing_indices[cluster_id])) >> core_id;
    /// Index of buffer that is empty (or waiting and wasting)
    const uint32_t ready_index = ((1 << core_id) & (*callback_ready_indices[cluster_id])) >> core_id;

    if (writing_index != ready_index) return;

    /// If during kernel, either writing index != ready index or bytes used in
    /// original buffer will not be 0. If during flush, unused buffer can be
    /// skipped.
    if (writing_index == ready_index && callback_used_host[cluster_id][core_id * 2 + 1 - writing_index] == 0) {
        *callback_ready_indices[cluster_id] ^= (1 << core_id);
        return;
    }
#ifdef DEBUG
    printf("Callback buffer used on Cluster-%d Core-%d: %d\n", cluster_id, core_id,
           callback_used_host[cluster_id][core_id * 2 + 1 - writing_index]);
#endif
    if (writing_index == ready_index) {
        auto src = static_cast<char *>(callback_buffers[cluster_id][core_id]) + (1 - ready_index) * JSI_DEV_BUFFER_SIZE;
#ifdef DEBUG
        std::cout << "Got something from callback buffer of core " << cluster_id << " - " << core_id << std::endl;

        int size = callback_used_host[cluster_id][core_id * 2 + 1 - writing_index];
        printf("SIZE is %d\n", size);

        for (size_t i = 0; i < size; i += 4) {
            printf("HOST - %p: [%x]\n", src + i, *(src + i));
            printf("HOST - %p: [%x]\n", src + i + 1, *(src + i + 1));
            printf("HOST - %p: [%x]\n", src + i + 2, *(src + i + 2));
            printf("HOST - %p: [%x]\n", src + i + 3, *(src + i + 3));
        }
#endif
        /// TODO: is this operation safe ?
        write_to_file(src, callback_used_host[cluster_id][core_id * 2 + 1 - writing_index], cluster_id, core_id);
        /// clear buffer and set bytes used to 0
        callback_used_host[cluster_id][core_id * 2 + 1 - writing_index] = 0;
        *callback_ready_indices[cluster_id] ^= (1 << core_id);
    }
}

static void collect_from_activity_buffer(const int cluster_id, const int core_id) {
    if (activity_ready_indices[cluster_id] == nullptr) {
        // printf("Activity buffer of Cluster-%d Core-%d is not initialized\n",
        // cluster_id, core_id);
        return;
    }

    /// Index of buffer currently being written
    const uint32_t writing_index = ((1 << core_id) & *activity_writing_indices[cluster_id]) >> core_id;
    /// Index of buffer that is empty (or waiting and wasting)
    const uint32_t ready_index = ((1 << core_id) & *activity_ready_indices[cluster_id]) >> core_id;

    if (writing_index != ready_index) return;

    /// If during kernel, either writing index != ready index or bytes used in
    /// original buffer will not be 0. If during flush, unused buffer can be
    /// skipped.
    if (writing_index == ready_index && activity_used_host[cluster_id][core_id * 2 + 1 - writing_index] == 0) {
        *activity_ready_indices[cluster_id] ^= (1 << core_id);
        return;
    }

#ifdef DEBUG
    printf("Activity buffer used on Cluster-%d Core-%d: %d\n", cluster_id, core_id,
           activity_used_host[cluster_id][core_id * 2 + 1 - writing_index]);
#endif

    if (writing_index == ready_index) {
#ifdef DEBUG
        std::cout << "Got something from activity buffer of core " << cluster_id << " - " << core_id << std::endl;
#endif
        auto src = static_cast<char *>(activity_buffers[cluster_id][core_id]) + (1 - ready_index) * JSI_DEV_BUFFER_SIZE;

        /// TODO: is this operation safe ?
        auto    *cur  = reinterpret_cast<accl_activity_record_t *>(src);
        uint32_t used = activity_used_host[cluster_id][core_id * 2 + 1 - writing_index];
        while (reinterpret_cast<char *>(cur) < src + used) {
            /// TODO: how to embed data ?
            tracer.pool_->Write(std::move(*cur));
            ++cur;
        }
        /// clear buffer and set bytes used to 0
        activity_used_host[cluster_id][core_id * 2 + 1 - writing_index] = 0;
        *activity_ready_indices[cluster_id] ^= (1 << core_id);
    }
}

static void ConsumerThreadLoop() {
    while (tracer.stop_flag_.load()) {
        for (int cluster_id = 0; cluster_id < 4; cluster_id++) {
            for (int core_id = 0; core_id < 24; ++core_id) {
                collect_from_callback_buffer(cluster_id, core_id);
                collect_from_activity_buffer(cluster_id, core_id);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

AcclTracer::AcclTracer(int ring_size)
    : ring_size_(ring_size), pool_(nullptr) {
    buffer_node_valid_.resize(ring_size_);
    ring_buffer_.resize(ring_size_);
    for (auto &buffer : callback_buffers)
        buffer.fill(nullptr);
    for (auto &buffer : activity_buffers)
        buffer.fill(nullptr);
    callback_writing_indices.fill(nullptr);
    callback_ready_indices.fill(nullptr);
    activity_writing_indices.fill(nullptr);
    activity_ready_indices.fill(nullptr);
    callback_used_host.fill(nullptr);
    activity_used_host.fill(nullptr);
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
    consumer_thread_  = std::thread(ConsumerThreadLoop);

    /// TODO: substitute this with storage apis.
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
