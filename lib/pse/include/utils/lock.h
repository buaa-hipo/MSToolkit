#pragma once
#include <atomic>

namespace pse
{
namespace utils
{
template <typename T>
class SpinLock
{
    std::atomic_ref<T> flag;
    SpinLock(const SpinLock &) = delete;
    SpinLock &operator=(const SpinLock &) = delete;

public:
    SpinLock(T &v)
    : flag(v)
    {
    }

    void lock()
    {
        while (flag.exchange(1, std::memory_order_acquire))
            ;
    }

    void unlock() { flag.store(0, std::memory_order_release); }
};

template <typename T>
class LockGuard
{
    bool _valid;
    T *_lock;
    LockGuard(const LockGuard &) = delete;
    LockGuard &operator=(const LockGuard &) = delete;

public:
    LockGuard(T *lock, bool valid)
    : _valid(valid)
    , _lock(lock)
    {
        if (_valid)
        {
            lock->lock();
        }
    }

    LockGuard(T &lock, bool valid)
    : _valid(valid)
    , _lock(&lock)
    {
        if (_valid)
        {
            _lock->lock();
        }
    }

    ~LockGuard()
    {
        if (_valid)
        {
            _lock->unlock();
        }
    }
};

} // namespace utils

} // namespace pse
