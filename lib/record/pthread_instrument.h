#ifndef __INSTRUMENT_PTHREAD__
#define __INSTRUMENT_PTHREAD__

#include "record/record_type.h"
#include "record/record_writer.h"
#include "instrument/pmu_collector.h"
#include "utils/configuration.h"
#include "utils/safe.hpp"

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <malloc.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <atomic>

#include <pthread.h>
#include <mpi.h>

#define PTHREAD_WRAPPER_INIT_PRIORITY (RECORD_INIT_PRIORITY+200)
#define PTHREAD_WRAPPER_FINI_PRIORITY (RECORD_FINI_PRIORITY+200)

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

bool enable_pthread_collect = false;

std::atomic<int> jsi_pthread_wrapper_init_completed(0);
std::atomic<int> jsi_pthread_wrapper_fini_completed(0);
std::atomic<int> jsi_pthread_function_init_completed(0);

static pthread_mutex_t init_func_lock = PTHREAD_MUTEX_INITIALIZER;

static thread_local int mpi_initialized = 0;
static thread_local int mpi_finalized = 0;  

// 线程创建与结束相关函数
typedef int (*pthread_create_ptr)(pthread_t *thread, const pthread_attr_t *attr,
    void *(*start_routine)(void *), void *arg);
typedef int (*pthread_join_ptr)(pthread_t thread, void **retval);
typedef int (*pthread_detach_ptr)(pthread_t thread);
typedef void (*pthread_exit_ptr)(void *retval);

// 互斥锁相关函数
typedef int (*pthread_mutex_init_ptr)(pthread_mutex_t *mutex,
        const pthread_mutexattr_t *attr);
typedef int (*pthread_mutex_destroy_ptr)(pthread_mutex_t *mutex);
typedef int (*pthread_mutex_lock_ptr)(pthread_mutex_t *mutex);
typedef int (*pthread_mutex_trylock_ptr)(pthread_mutex_t *mutex);
typedef int (*pthread_mutex_unlock_ptr)(pthread_mutex_t *mutex);

// 条件变量相关函数
typedef int (*pthread_cond_init_ptr)(pthread_cond_t *cond, const pthread_condattr_t *attr);
typedef int (*pthread_cond_destroy_ptr)(pthread_cond_t *cond);
typedef int (*pthread_cond_wait_ptr)(pthread_cond_t *cond, pthread_mutex_t *mutex);
typedef int (*pthread_cond_timedwait_ptr)(pthread_cond_t *cond, pthread_mutex_t *mutex,
            const struct timespec *abstime);
typedef int (*pthread_cond_signal_ptr)(pthread_cond_t *cond);
typedef int (*pthread_cond_broadcast_ptr)(pthread_cond_t *cond);

static pthread_create_ptr real_pthread_create = nullptr;
static pthread_join_ptr   real_pthread_join   = nullptr;
static pthread_detach_ptr real_pthread_detach = nullptr;
static pthread_exit_ptr   real_pthread_exit   = nullptr;

static pthread_mutex_init_ptr   real_pthread_mutex_init   = nullptr;
static pthread_mutex_destroy_ptr real_pthread_mutex_destroy = nullptr;
static pthread_mutex_lock_ptr    real_pthread_mutex_lock    = nullptr;
static pthread_mutex_trylock_ptr real_pthread_mutex_trylock = nullptr;
static pthread_mutex_unlock_ptr  real_pthread_mutex_unlock  = nullptr;

static pthread_cond_init_ptr      real_pthread_cond_init      = nullptr;
static pthread_cond_destroy_ptr   real_pthread_cond_destroy   = nullptr;
static pthread_cond_wait_ptr      real_pthread_cond_wait      = nullptr;
static pthread_cond_timedwait_ptr real_pthread_cond_timedwait = nullptr;
static pthread_cond_signal_ptr    real_pthread_cond_signal    = nullptr;
static pthread_cond_broadcast_ptr real_pthread_cond_broadcast = nullptr;

void jsi_pthread_function_init()
{
    real_pthread_create = (pthread_create_ptr)dlsym(RTLD_NEXT, "pthread_create");
    real_pthread_join = (pthread_join_ptr)dlsym(RTLD_NEXT, "pthread_join");
    real_pthread_detach = (pthread_detach_ptr)dlsym(RTLD_NEXT, "pthread_detach");
    real_pthread_exit = (pthread_exit_ptr)dlsym(RTLD_NEXT, "pthread_exit");

    real_pthread_mutex_init = (pthread_mutex_init_ptr)dlsym(RTLD_NEXT, "pthread_mutex_init");
    real_pthread_mutex_destroy = (pthread_mutex_destroy_ptr)dlsym(RTLD_NEXT, "pthread_mutex_destroy");
    real_pthread_mutex_lock = (pthread_mutex_lock_ptr)dlsym(RTLD_NEXT, "pthread_mutex_lock");
    real_pthread_mutex_trylock = (pthread_mutex_trylock_ptr)dlsym(RTLD_NEXT, "pthread_mutex_trylock");
    real_pthread_mutex_unlock = (pthread_mutex_unlock_ptr)dlsym(RTLD_NEXT, "pthread_mutex_unlock");

    real_pthread_cond_init = (pthread_cond_init_ptr)dlsym(RTLD_NEXT, "pthread_cond_init");
    real_pthread_cond_destroy = (pthread_cond_destroy_ptr)dlsym(RTLD_NEXT, "pthread_cond_destroy");
    real_pthread_cond_wait = (pthread_cond_wait_ptr)dlsym(RTLD_NEXT, "pthread_cond_wait");
    real_pthread_cond_timedwait = (pthread_cond_timedwait_ptr)dlsym(RTLD_NEXT, "pthread_cond_timedwait");
    real_pthread_cond_signal = (pthread_cond_signal_ptr)dlsym(RTLD_NEXT, "pthread_cond_signal");
    real_pthread_cond_broadcast = (pthread_cond_broadcast_ptr)dlsym(RTLD_NEXT, "pthread_cond_broadcast");
}

void ensure_pthread_init() {
    // int expected = 0;
    // if (jsi_pthread_function_init_completed.compare_exchange_strong(
    //         expected, 1, std::memory_order_acq_rel)) {
    //     jsi_pthread_function_init();
    //     jsi_pthread_function_init_completed.store(2, std::memory_order_release);
    // } else {
    //     while (jsi_pthread_function_init_completed.load(std::memory_order_acquire) != 2) {
    //         std::this_thread::yield();
    //     }
    // }
    if(jsi_pthread_function_init_completed.load() == 0)
    {
        jsi_pthread_function_init();
        jsi_pthread_function_init_completed.store(1);
    }
}

__attribute__((constructor (PTHREAD_WRAPPER_INIT_PRIORITY)))
void jsi_pthread_tracer_init() {
    enable_pthread_collect = EnvConfigHelper::get_enabled("JSI_ENABLE_PTHREAD_COLLECT", false);

    ensure_pthread_init();

    JSI_LOG(JSILOG_INFO, "Initialize JSI pthread Wrapper Library.\n");

    jsi_pthread_wrapper_init_completed.store(1);
}

__attribute__((destructor (PTHREAD_WRAPPER_INIT_PRIORITY)))
void jsi_pthread_tracer_finalize() { 
    jsi_pthread_wrapper_fini_completed.store(1);
    JSI_LOG(JSILOG_INFO, "Finalize JSI PTHREAD Wrapper Library.\n");
}


inline __attribute__((always_inline))
record_child_thread_start_t* jsi_enter_child_thread_start(record_child_thread_start_t* rec, uint16_t MsgType, void *(*start_routine)(void *), void *arg) {
    rec[0].record.MsgType = (int16_t)MsgType;
    uint64_t time = get_tsc_raw();
    rec[0].record.timestamps.enter = time;
    rec[0].record.timestamps.exit = time;
    rec[0].thread_id = _ConfigHelper::get_tid();
    rec[0].start_routine = start_routine;
    rec[0].arg = arg;
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}


inline __attribute__((always_inline))
record_child_thread_finalize_t* jsi_enter_child_thread_finalize(record_child_thread_finalize_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    uint64_t time = get_tsc_raw();
    rec[0].record.timestamps.enter = time;
    rec[0].record.timestamps.exit = time;
    rec[0].thread_id = _ConfigHelper::get_tid();
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

// 定义包装函数所需的参数结构体
struct WrappedStartArgs {
    void* (*original_start)(void*);  
    void* original_args;              
};

void* wrapped_start_routine(void* arg) {
    WrappedStartArgs* args = (WrappedStartArgs*)arg;
    void* (*original_start)(void*) = args->original_start;
    void* original_args = args->original_args;

    // record_init_thread();
    jsi_thread_init();

    void* result = original_start(original_args);
    
    // jsi_thread_data_mark_finalized();
    // record_fini_thread();
    jsi_thread_finalize();

    free(args);

    return result;
}

inline __attribute__((always_inline))
record_pthread_create_t* jsi_enter_pthread_create(record_pthread_create_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_create1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_create(record_pthread_create_t* rec, pthread_t thread, void *(*start_routine)(void *), void *arg) {
    record_pthread_create_t& p_rec = reinterpret_cast<record_pthread_create_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    p_rec.thread = thread;
    p_rec.start_routine = start_routine;
    p_rec.arg = arg;
    // JSI_LOG(JSILOG_INFO, "pthread_create3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_create_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_create_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
record_pthread_join_t* jsi_enter_pthread_join(record_pthread_join_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_join1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_join(record_pthread_join_t* rec, pthread_t thread) {
    record_pthread_join_t& p_rec = reinterpret_cast<record_pthread_join_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    p_rec.thread = thread;
    // JSI_LOG(JSILOG_INFO, "pthread_join3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_join_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_join_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
record_pthread_detach_t* jsi_enter_pthread_detach(record_pthread_detach_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_detach1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_detach(record_pthread_detach_t* rec, pthread_t thread) {
    record_pthread_detach_t& p_rec = reinterpret_cast<record_pthread_detach_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    p_rec.thread = thread;
    // JSI_LOG(JSILOG_INFO, "pthread_detach3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_detach_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_detach_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}


inline __attribute__((always_inline))
record_pthread_exit_t* jsi_enter_pthread_exit(record_pthread_exit_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_exit1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_exit(record_pthread_exit_t* rec) {
    record_pthread_exit_t& p_rec = reinterpret_cast<record_pthread_exit_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_exit3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_exit_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_exit_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
record_pthread_mutex_init_t* jsi_enter_pthread_mutex_init(record_pthread_mutex_init_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_mutex_init1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_mutex_init(record_pthread_mutex_init_t* rec) {
    record_pthread_mutex_init_t& p_rec = reinterpret_cast<record_pthread_mutex_init_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_mutex_init3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_mutex_init_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_mutex_init_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

// pthread_mutex_destroy
inline __attribute__((always_inline))
record_pthread_mutex_destroy_t* jsi_enter_pthread_mutex_destroy(record_pthread_mutex_destroy_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_mutex_destroy1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_mutex_destroy(record_pthread_mutex_destroy_t* rec) {
    record_pthread_mutex_destroy_t& p_rec = reinterpret_cast<record_pthread_mutex_destroy_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_mutex_destroy3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_mutex_destroy_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_mutex_destroy_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

// pthread_mutex_lock
inline __attribute__((always_inline))
record_pthread_mutex_lock_t* jsi_enter_pthread_mutex_lock(record_pthread_mutex_lock_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_mutex_lock1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_mutex_lock(record_pthread_mutex_lock_t* rec) {
    record_pthread_mutex_lock_t& p_rec = reinterpret_cast<record_pthread_mutex_lock_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_mutex_lock3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_mutex_lock_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_mutex_lock_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

// pthread_mutex_trylock
inline __attribute__((always_inline))
record_pthread_mutex_trylock_t* jsi_enter_pthread_mutex_trylock(record_pthread_mutex_trylock_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_mutex_trylock1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_mutex_trylock(record_pthread_mutex_trylock_t* rec) {
    record_pthread_mutex_trylock_t& p_rec = reinterpret_cast<record_pthread_mutex_trylock_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_mutex_trylock3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_mutex_trylock_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_mutex_trylock_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

// pthread_mutex_unlock
inline __attribute__((always_inline))
record_pthread_mutex_unlock_t* jsi_enter_pthread_mutex_unlock(record_pthread_mutex_unlock_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_mutex_unlock1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_mutex_unlock(record_pthread_mutex_unlock_t* rec) {
    record_pthread_mutex_unlock_t& p_rec = reinterpret_cast<record_pthread_mutex_unlock_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_mutex_unlock3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_mutex_unlock_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_mutex_unlock_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
record_pthread_cond_init_t* jsi_enter_pthread_cond_init(record_pthread_cond_init_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_init1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
         rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
         uint64_t* counters = RecordWriterHelper::counters(rec);
         pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_cond_init(record_pthread_cond_init_t* rec) {
    record_pthread_cond_init_t& p_rec = reinterpret_cast<record_pthread_cond_init_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_init3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
         uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_cond_init_t*>(rec));
         pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_cond_init_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
record_pthread_cond_destroy_t* jsi_enter_pthread_cond_destroy(record_pthread_cond_destroy_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_destroy1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_cond_destroy(record_pthread_cond_destroy_t* rec) {
    record_pthread_cond_destroy_t& p_rec = reinterpret_cast<record_pthread_cond_destroy_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_destroy3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_cond_destroy_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_cond_destroy_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}


// pthread_cond_wait
inline __attribute__((always_inline))
record_pthread_cond_wait_t* jsi_enter_pthread_cond_wait(record_pthread_cond_wait_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_wait1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
         rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
         uint64_t* counters = RecordWriterHelper::counters(rec);
         pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_cond_wait(record_pthread_cond_wait_t* rec) {
    record_pthread_cond_wait_t& p_rec = reinterpret_cast<record_pthread_cond_wait_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_wait3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
         uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_cond_wait_t*>(rec));
         pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_cond_wait_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

// pthread_cond_timedwait
inline __attribute__((always_inline))
record_pthread_cond_timedwait_t* jsi_enter_pthread_cond_timedwait(record_pthread_cond_timedwait_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_timedwait1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
         rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
         uint64_t* counters = RecordWriterHelper::counters(rec);
         pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_cond_timedwait(record_pthread_cond_timedwait_t* rec) {
    record_pthread_cond_timedwait_t& p_rec = reinterpret_cast<record_pthread_cond_timedwait_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_timedwait3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
         uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_cond_timedwait_t*>(rec));
         pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_cond_timedwait_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

// pthread_cond_signal
inline __attribute__((always_inline))
record_pthread_cond_signal_t* jsi_enter_pthread_cond_signal(record_pthread_cond_signal_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_signal1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
         rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
         uint64_t* counters = RecordWriterHelper::counters(rec);
         pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_cond_signal(record_pthread_cond_signal_t* rec) {
    record_pthread_cond_signal_t& p_rec = reinterpret_cast<record_pthread_cond_signal_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_signal3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
         uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_cond_signal_t*>(rec));
         pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_cond_signal_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

// pthread_cond_broadcast
inline __attribute__((always_inline))
record_pthread_cond_broadcast_t* jsi_enter_pthread_cond_broadcast(record_pthread_cond_broadcast_t* rec, uint16_t MsgType) {
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    rec[0].thread_id = _ConfigHelper::get_tid();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_broadcast1.\n");
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
         rec[0].record.ctxt = backtrace_context_get();
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
         uint64_t* counters = RecordWriterHelper::counters(rec);
         pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_exit_pthread_cond_broadcast(record_pthread_cond_broadcast_t* rec) {
    record_pthread_cond_broadcast_t& p_rec = reinterpret_cast<record_pthread_cond_broadcast_t*>(rec)[0];
    p_rec.record.timestamps.exit = get_tsc_raw();
    // JSI_LOG(JSILOG_INFO, "pthread_cond_broadcast3.\n");
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
         uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_pthread_cond_broadcast_t*>(rec));
         pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_pthread_cond_broadcast_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

#endif