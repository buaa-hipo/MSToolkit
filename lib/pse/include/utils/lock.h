#pragma once
#include <atomic>
#include <stdio.h>

namespace pse
{
namespace utils
{

class _ConfigHelper {
public:
    static const std::string& get_hostname() {
        static std::string hostname = _ConfigHelper::_get_hostname();
        return hostname;
    }
    static uint32_t get_pid() {
        static auto pid = getpid();
        return pid;
    }
    static uint32_t get_tid() {
        thread_local static auto tid = syscall(SYS_gettid);
        return tid;
    }
private:
    static std::string _get_hostname() {
        char _hostname[1024];
        gethostname(_hostname, 1024);
        _hostname[1023] = '\0';
        return std::string(_hostname);
    }
};

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
        // printf("[SpinLock PID=%d, TID=%d] Trying to get LOCK for flag: %x, value=%x\n", _ConfigHelper::get_pid(), _ConfigHelper::get_tid(), &flag, flag.load());
        while (flag.exchange(1, std::memory_order_acquire))
            ;
        // printf("[SpinLock PID=%d, TID=%d] LOCK %x GET!\n", _ConfigHelper::get_pid(), _ConfigHelper::get_tid(), &flag);
    }

    void unlock()
    { 
        flag.store(0, std::memory_order_release); 
        // printf("[SpinLock PID=%d, TID=%d] UNLOCKED flag: %x, value=%x\n", _ConfigHelper::get_pid(), _ConfigHelper::get_tid(), &flag, flag.load()); 
    }
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
