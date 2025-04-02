
#ifndef MEMORY_POOL_H_
#define MEMORY_POOL_H_

//#include "roctracer.h"

#include <cassert>
#include <condition_variable>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <future>
#include <mutex>
// #include "mt_record_types.h"
#include "mt_callback_defs.h"

typedef void (*accl_pool_allocator_t)(char **, size_t, void *);

typedef void (*accl_pool_callback_t)(const char *, const char *, void *);

struct buffer_pool_property_t {
    uint32_t mode;                                      // ROC Tracer mode.
    uint64_t buffer_size = 1024;                                 // Size of buffer in bytes.
    /**
     * The allocator function to use to allocate and deallocate the buffer. If
     * NULL then \p malloc, \p realloc, and \p free are used.
     */
    accl_pool_allocator_t alloc_fun;
    void *alloc_arg;                                    // The argument to pass when invoking the \p alloc_fun allocator.
    accl_pool_callback_t buffer_callback_fun;    // The function to call when a buffer becomes full or is flushed.
    void *buffer_callback_arg;                          // The argument to pass when invoking the \p buffer_callback_fun callback.
};

namespace accl_activity_pool {
    using Functor = std::function<void(accl_activity_record_t&, const void *)>;
    /// @brief Double buffer pool for activity api.
    ///
    /// When buffer full or Flush() called, switch buffer.
    class MemoryPool {
    public:

        /// Construct pool with user defined property
        explicit MemoryPool(const buffer_pool_property_t&);

        /// Flush buffer and join consumer thread
        ~MemoryPool();

        MemoryPool(const MemoryPool &) = delete;

        MemoryPool &operator=(const MemoryPool &) = delete;

        /// @param record record struct, in \p accl_activity_record_t format.
        /// @param data data to be embedded, e.g. \p *accl_activity_record_t::kernel_name .
        /// @param data_size size of \p data.
        /// @param embed_data \p Functor, user defined operation.
        void Write(accl_activity_record_t&& record, const void *data, size_t data_size, Functor&& embed_data);

        void Write(accl_activity_record_t&& record);

        /// Flush the records and block until all consumed.
        void Flush();

    private:
        /// Switch to another block
        void SwitchBuffers();

        void ConsumerThreadLoop(std::promise<void> ready);

        void NotifyConsumerThread(const std::byte *data_begin, const std::byte *data_end);

        /// Allocate memory with user defined allocator (malloc/realloc/free if not defined)
        void AllocateMemory(std::byte **ptr, size_t size) const;

        /// Properties used to create the memory pool.
        const buffer_pool_property_t properties_;

        std::byte *pool_begin_;         ///< Begin address of all allocated space.
        std::byte *pool_end_;           ///< End address of all allocated space.
        std::byte *buffer_begin_;       ///< Begin address of buffer currently used.
        std::byte *buffer_end_;         ///< End address of buffer currently used.
        std::byte *record_ptr_;         ///< Address to be written to.
        std::byte *data_ptr_;           ///< Begin address of data section after record section.
        std::mutex producer_mutex_;

        std::thread consumer_thread_;
        struct {
            const std::byte *begin;
            const std::byte *end;
            bool valid = false;         ///< true for consumer in use, false for idle.
        } consumer_arg_;

        std::mutex consumer_mutex_;
        std::condition_variable consumer_cond_;
    };
}

#endif // MEMORY_POOL_H_
