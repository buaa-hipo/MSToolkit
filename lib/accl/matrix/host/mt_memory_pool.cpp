#include <condition_variable>
#include <thread>
#include <mutex>
#include "record/mt_memory_pool.h"
using namespace std;

accl_activity_pool::MemoryPool::MemoryPool(const buffer_pool_property_t &properties) : properties_(properties) {
    const size_t allocation_size = 2 * std::max(2 * sizeof(accl_activity_record_t), properties_.buffer_size);
    pool_begin_ = nullptr;
    AllocateMemory(&pool_begin_, allocation_size);
    assert(pool_begin_ != nullptr && "pool allocator failed");

    pool_end_ = pool_begin_ + allocation_size;
    buffer_begin_ = pool_begin_;
    buffer_end_ = buffer_begin_ + properties_.buffer_size;
    record_ptr_ = buffer_begin_;
    data_ptr_ = buffer_end_;

    // Create a consumer thread and wait for it to be ready to accept work.
    std::promise<void> ready;
    const std::future<void> future = ready.get_future();
    consumer_thread_ = std::thread(&MemoryPool::ConsumerThreadLoop, this, std::move(ready));
    future.wait();
}

accl_activity_pool::MemoryPool::~MemoryPool() {
    Flush();

    // Wait for the previous flush to complete, then send the exit signal.
    NotifyConsumerThread(nullptr, nullptr);
    consumer_thread_.join();

    // Free the pool's buffer memory.
    AllocateMemory(&pool_begin_, 0);
}

void accl_activity_pool::MemoryPool::Write(accl_activity_record_t &&record, const void *data, size_t data_size,
                                           Functor&& embed_data = {}) {
    // If data is null, then data_size must be 0
    assert(data != nullptr || data_size == 0);

    std::lock_guard producer_lock(producer_mutex_);

    // The amount of memory reserved in the buffer to store data.
    // If data is too big then it won't be copied into the buffer.
    const size_t reserve_data_size =
            data_size <= (properties_.buffer_size - sizeof(accl_activity_record_t)) ? data_size : 0;

    std::byte *next_record = record_ptr_ + sizeof(accl_activity_record_t);
    if (next_record > data_ptr_ - reserve_data_size) {
        // Consume records in buffer.
        NotifyConsumerThread(buffer_begin_, record_ptr_);
        SwitchBuffers();
        // Pointed to new buffer.
        next_record = record_ptr_ + sizeof(accl_activity_record_t);
        // assert(next_record <= buffer_end_ && "buffer size is less than the record size");
    }

    // Embed data in the record.
    if (reserve_data_size != 0) {
        // If size fits, copy data first.
        data_ptr_ -= data_size;
        ::memcpy(data_ptr_, data, data_size);
        embed_data(record, data_ptr_);
    } else if (data != nullptr) {
        // Otherwise just embed.
        embed_data(record, data);
    }

    // Store record into buffer.
    ::memcpy(record_ptr_, &record, sizeof(accl_activity_record_t));
    record_ptr_ = next_record;

    // If the data does not fit in the buffer, flush the buffer synchronously to avoid dangling pointer.
    if (data != nullptr && reserve_data_size == 0) {
        NotifyConsumerThread(buffer_begin_, record_ptr_);
        SwitchBuffers();
        {
            std::unique_lock consumer_lock(consumer_mutex_);
            consumer_cond_.wait(consumer_lock, [this]() { return !consumer_arg_.valid; });
        }
    }
}

void accl_activity_pool::MemoryPool::Write(accl_activity_record_t &&record) {
    Write(std::forward<accl_activity_record_t>(record), nullptr, 0, {});
}

void accl_activity_pool::MemoryPool::Flush() {
    {
        std::lock_guard producer_lock(producer_mutex_);
        if (record_ptr_ == buffer_begin_)
            return;

        NotifyConsumerThread(buffer_begin_, record_ptr_);
        SwitchBuffers();
    }
    {
        // Wait for the current operation to complete.
        std::unique_lock consumer_lock(consumer_mutex_);
        consumer_cond_.wait(consumer_lock, [this]() { return !consumer_arg_.valid; });
    }
}

void accl_activity_pool::MemoryPool::SwitchBuffers() {
    buffer_begin_ = (buffer_end_ == pool_end_) ? pool_begin_ : buffer_end_;
    buffer_end_ = buffer_begin_ + properties_.buffer_size;
    record_ptr_ = buffer_begin_;
    data_ptr_ = buffer_end_;
}

void accl_activity_pool::MemoryPool::ConsumerThreadLoop(std::promise<void> ready) {
    std::unique_lock consumer_lock(consumer_mutex_);

    // Consumer thread is ready to accept work.
    ready.set_value();

    while (true) {
        consumer_cond_.wait(consumer_lock, [this]() { return consumer_arg_.valid; });

        // begin == end == nullptr means the thread needs to exit.
        if (consumer_arg_.begin == nullptr && consumer_arg_.end == nullptr)
            break;

        // Call user defined callback function to consume records.
        properties_.buffer_callback_fun(
                reinterpret_cast<const char *>(consumer_arg_.begin),
                reinterpret_cast<const char *>(consumer_arg_.end),
                properties_.buffer_callback_arg
        );

        // Mark this operation as complete (valid=false) and notify all producers that may be
        // waiting for this operation to finish, or to start a new operation. See comment below in
        // NotifyConsumerThread().
        consumer_arg_.valid = false;
        consumer_cond_.notify_all();
    }
}

void accl_activity_pool::MemoryPool::NotifyConsumerThread(const std::byte *data_begin, const std::byte *data_end) {
    std::unique_lock consumer_lock(consumer_mutex_);

    // If consumer_arg_ is still in use (valid=true), then wait for the consumer thread to finish
    // processing the current operation. Multiple producers may wait here, one will be allowed to
    // continue once the consumer thread is idle and valid=false. This prevents a race condition
    // where operations would be lost if multiple producers could enter this critical section
    // (sequentially) before the consumer thread could re-acquire the consumer_mutex_ lock.
    consumer_cond_.wait(consumer_lock, [this]() { return !consumer_arg_.valid; });

    consumer_arg_.begin = data_begin;
    consumer_arg_.end = data_end;

    consumer_arg_.valid = true;
    consumer_cond_.notify_all();
}

void accl_activity_pool::MemoryPool::AllocateMemory(std::byte **ptr, size_t size) const {
    if (properties_.alloc_fun != nullptr) {
        // Use the custom allocator provided in the properties.
        properties_.alloc_fun(reinterpret_cast<char **>(ptr), size, properties_.alloc_arg);
        return;
    }
    // No custom allocator was provided, use default allocator.
    if (*ptr == nullptr) {
        *ptr = static_cast<std::byte *>(malloc(size));
    } else if (size != 0) {
        *ptr = static_cast<std::byte *>(realloc(*ptr, size));
    } else {
        free(*ptr);
        *ptr = nullptr;
    }
}
