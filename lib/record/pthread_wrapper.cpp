#ifndef _EXTERN_C_
#define _EXTERN_C_ extern "C"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <execinfo.h>

#include "pthread_instrument.h"
#include "utils/safe.hpp"

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

_EXTERN_C_ int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
    void *(*start_routine)(void *), void *arg) {
    ensure_pthread_init();

    int ret = -1;

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        jsi_mark_unsafe_enter();
        ret = real_pthread_create(thread, attr, start_routine, arg);
        jsi_mark_unsafe_exit();
        return ret;
    }

    WrappedStartArgs* wrapper_args = nullptr;
    if (wrapper_args = new WrappedStartArgs()) {
        wrapper_args->original_start = start_routine;
        wrapper_args->original_args = arg;
    } else {
        JSI_LOG(JSILOG_INFO, "pthread_create WrappedStartArgs Malloc ERROR.\n");
        jsi_safe_exit();
        jsi_mark_unsafe_enter();
        ret = real_pthread_create(thread, attr, start_routine, arg);
        jsi_mark_unsafe_exit();
        return ret;
    }

    if (\
        jsi_record_writer_init_completed.load() != 0 && \
        jsi_pthread_wrapper_init_completed.load() != 0 && \
        jsi_record_writer_fini_completed.load() != 1 && \
        jsi_pthread_wrapper_fini_completed.load() != 1 && \
        jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        auto rec = (record_pthread_create_t*) ALLOCATE(
            sizeof(record_pthread_create_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        // JSI_LOG(JSILOG_INFO, "pthread_create2.\n");
        rec = jsi_enter_pthread_create(rec, event_Pthread_Create);
        jsi_mark_unsafe_enter();
        ret = real_pthread_create(thread, attr, wrapped_start_routine, wrapper_args);
        jsi_mark_unsafe_exit();
        jsi_exit_pthread_create(rec, *thread, start_routine, arg);
    }
    else
    {
        jsi_mark_unsafe_enter();
        ret = real_pthread_create(thread, attr, start_routine, arg);
        jsi_mark_unsafe_exit();
    }
    real_pthread_mutex_lock(&threads_lock);
    threads->insert(*thread);
    real_pthread_mutex_unlock(&threads_lock);
    JSI_LOG(JSILOG_INFO, "create THREDID %p\n", *thread);
    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ int pthread_join(pthread_t thread, void **retval) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_join(thread, retval);
    }
    
    int ret = -1;
    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_pthread_wrapper_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 &&\
    jsi_pthread_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        auto rec = (record_pthread_join_t*) ALLOCATE(
            sizeof(record_pthread_join_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        // JSI_LOG(JSILOG_INFO, "pthread_join2.\n");
        rec = jsi_enter_pthread_join(rec, event_Pthread_Join);
        ret = real_pthread_join(thread, retval);
        jsi_exit_pthread_join(rec, thread);
    }
    else
    {
        ret = real_pthread_join(thread, retval);
    }
    // auto addr = threads.data();
    // JSI_LOG(JSILOG_INFO, "Address: %p\n", addr);
    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ int pthread_detach(pthread_t thread) {
    ensure_pthread_init();
    
    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_detach(thread);
    }

    int ret = -1;
    if (\
        jsi_record_writer_init_completed.load() != 0 && \
        jsi_pthread_wrapper_init_completed.load() != 0 && \
        jsi_record_writer_fini_completed.load() != 1 &&\
        jsi_pthread_wrapper_fini_completed.load() != 1 && \
        jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        auto rec = (record_pthread_detach_t*) ALLOCATE(
            sizeof(record_pthread_detach_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        // JSI_LOG(JSILOG_INFO, "pthread_detach2.\n");
        rec = jsi_enter_pthread_detach(rec, event_Pthread_Detach);
        ret = real_pthread_detach(thread);
        jsi_exit_pthread_detach(rec, thread);
    }
    else
    {
        ret = real_pthread_detach(thread);
    }

    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ void pthread_exit(void *retval) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        real_pthread_exit(retval);
    }

    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_pthread_wrapper_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 &&\
    jsi_pthread_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        auto rec = (record_pthread_exit_t*) ALLOCATE(
            sizeof(record_pthread_exit_t) + sizeof(uint64_t) * jsi_pmu_num);
        // JSI_LOG(JSILOG_INFO, "pthread_exit2.\n");
        jsi_enter_pthread_exit(rec, event_Pthread_Exit);
        real_pthread_exit(retval);
    }
    else
    {
        real_pthread_exit(retval);
    }
    // Never reach here
    JSI_ERROR("[BUG] pthread_exit: SHOULD NOT REACH HERE.\n");
    // jsi_safe_exit();
    
    // return;
}

_EXTERN_C_ int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_mutex_init(mutex, attr);
    }
    
    int ret = -1;
    if (jsi_record_writer_init_completed.load() != 0 &&
        jsi_pthread_wrapper_init_completed.load() != 0 &&
        jsi_record_writer_fini_completed.load() != 1 &&
        jsi_pthread_wrapper_fini_completed.load() != 1 &&
        jsi_record_writer_thread_local_wrapper_completed.load() != 1) 
    {
        auto rec = (record_pthread_mutex_init_t*) ALLOCATE(
            sizeof(record_pthread_mutex_init_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        // JSI_LOG(JSILOG_INFO, "pthread_mutex_init2.\n");
        rec = jsi_enter_pthread_mutex_init(rec, event_Pthread_Mutex_Init);
        ret = real_pthread_mutex_init(mutex, attr);
        jsi_exit_pthread_mutex_init(rec);
    } else {
        ret = real_pthread_mutex_init(mutex, attr);
    }

    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_mutex_destroy(mutex);
    }

    int ret = -1;
    if (jsi_record_writer_init_completed.load() != 0 &&
        jsi_pthread_wrapper_init_completed.load() != 0 &&
        jsi_record_writer_fini_completed.load() != 1 &&
        jsi_pthread_wrapper_fini_completed.load() != 1 &&
        jsi_record_writer_thread_local_wrapper_completed.load() != 1) 
    {

         auto rec = (record_pthread_mutex_destroy_t*) ALLOCATE(
             sizeof(record_pthread_mutex_destroy_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        //  JSI_LOG(JSILOG_INFO, "pthread_mutex_destroy2.\n");
         rec = jsi_enter_pthread_mutex_destroy(rec, event_Pthread_Mutex_Destroy);
         ret = real_pthread_mutex_destroy(mutex);
         jsi_exit_pthread_mutex_destroy(rec);

    } else {
         ret = real_pthread_mutex_destroy(mutex);
    }

    // for (auto t : threads) {
    //     JSI_LOG(JSILOG_INFO, "THREDID %p\n", t);
    // }

    jsi_safe_exit();
    return ret;
}

_EXTERN_C_ int pthread_mutex_lock(pthread_mutex_t *mutex) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_mutex_lock(mutex);
    }

    int ret = -1;
    if (jsi_record_writer_init_completed.load() != 0 &&
        jsi_pthread_wrapper_init_completed.load() != 0 &&
        jsi_record_writer_fini_completed.load() != 1 &&
        jsi_pthread_wrapper_fini_completed.load() != 1 &&
        jsi_record_writer_thread_local_wrapper_completed.load() != 1) 
    {

         auto rec = (record_pthread_mutex_lock_t*) ALLOCATE(
             sizeof(record_pthread_mutex_lock_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        //  JSI_LOG(JSILOG_INFO, "pthread_mutex_lock2.\n");
         rec = jsi_enter_pthread_mutex_lock(rec, event_Pthread_Mutex_Lock);
         ret = real_pthread_mutex_lock(mutex);
         jsi_exit_pthread_mutex_lock(rec);

    } else {
         ret = real_pthread_mutex_lock(mutex);
    }

    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_mutex_trylock(mutex);
    }
    
    int ret = -1;
    if (jsi_record_writer_init_completed.load() != 0 &&
        jsi_pthread_wrapper_init_completed.load() != 0 &&
        jsi_record_writer_fini_completed.load() != 1 &&
        jsi_pthread_wrapper_fini_completed.load() != 1 &&
        jsi_record_writer_thread_local_wrapper_completed.load() != 1) 
    {

         auto rec = (record_pthread_mutex_trylock_t*) ALLOCATE(
             sizeof(record_pthread_mutex_trylock_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        //  JSI_LOG(JSILOG_INFO, "pthread_mutex_trylock2.\n");
         rec = jsi_enter_pthread_mutex_trylock(rec, event_Pthread_Mutex_Trylock);
         ret = real_pthread_mutex_trylock(mutex);
         jsi_exit_pthread_mutex_trylock(rec);

    } else {
         ret = real_pthread_mutex_trylock(mutex);
    }

    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ int pthread_mutex_unlock(pthread_mutex_t *mutex) {    
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_mutex_unlock(mutex);
    }
    
    int ret = -1;
    if (jsi_record_writer_init_completed.load() != 0 &&
        jsi_pthread_wrapper_init_completed.load() != 0 &&
        jsi_record_writer_fini_completed.load() != 1 &&
        jsi_pthread_wrapper_fini_completed.load() != 1 &&
        jsi_record_writer_thread_local_wrapper_completed.load() != 1) 
    {

         auto rec = (record_pthread_mutex_unlock_t*) ALLOCATE(
             sizeof(record_pthread_mutex_unlock_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        //  JSI_LOG(JSILOG_INFO, "pthread_mutex_unlock2.\n");
         rec = jsi_enter_pthread_mutex_unlock(rec, event_Pthread_Mutex_Unlock);
         ret = real_pthread_mutex_unlock(mutex);
         jsi_exit_pthread_mutex_unlock(rec);

    } else {
         ret = real_pthread_mutex_unlock(mutex);
    }

    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_cond_init(cond, attr);
    }
    
    int ret = -1;
    if (jsi_record_writer_init_completed.load() != 0 &&
        jsi_pthread_wrapper_init_completed.load() != 0 &&
        jsi_record_writer_fini_completed.load() != 1 &&
        jsi_pthread_wrapper_fini_completed.load() != 1 &&
        jsi_record_writer_thread_local_wrapper_completed.load() != 1) 
    {

         auto rec = (record_pthread_cond_init_t*) ALLOCATE(
             sizeof(record_pthread_cond_init_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        //  JSI_LOG(JSILOG_INFO, "pthread_cond_init2.\n");
         rec = jsi_enter_pthread_cond_init(rec, event_Pthread_Cond_Init);
         ret = real_pthread_cond_init(cond, attr);
         jsi_exit_pthread_cond_init(rec);

    } else {
         ret = real_pthread_cond_init(cond, attr);
    }

    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ int pthread_cond_destroy(pthread_cond_t *cond) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_cond_destroy(cond);
    }
    
    int ret = -1;
    if (jsi_record_writer_init_completed.load() != 0 &&
         jsi_pthread_wrapper_init_completed.load() != 0 &&
         jsi_record_writer_fini_completed.load() != 1 &&
         jsi_pthread_wrapper_fini_completed.load() != 1 &&
         jsi_record_writer_thread_local_wrapper_completed.load() != 1) 
    {

         auto rec = (record_pthread_cond_destroy_t*) ALLOCATE(
             sizeof(record_pthread_cond_destroy_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
         rec = jsi_enter_pthread_cond_destroy(rec, event_Pthread_Cond_Destroy);
         ret = real_pthread_cond_destroy(cond);
         jsi_exit_pthread_cond_destroy(rec);

    } else {
         ret = real_pthread_cond_destroy(cond);
    }

    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_cond_wait(cond, mutex);
    }
    
    int ret = -1;
    if (jsi_record_writer_init_completed.load() != 0 &&
         jsi_pthread_wrapper_init_completed.load() != 0 &&
         jsi_record_writer_fini_completed.load() != 1 &&
         jsi_pthread_wrapper_fini_completed.load() != 1 &&
         jsi_record_writer_thread_local_wrapper_completed.load() != 1) 
    {

         auto rec = (record_pthread_cond_wait_t*) ALLOCATE(
             sizeof(record_pthread_cond_wait_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        //  JSI_LOG(JSILOG_INFO, "pthread_cond_wait2.\n");
         rec = jsi_enter_pthread_cond_wait(rec, event_Pthread_Cond_Wait);
         ret = real_pthread_cond_wait(cond, mutex);
         jsi_exit_pthread_cond_wait(rec);

    } else {
         ret = real_pthread_cond_wait(cond, mutex);
    }

    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_cond_timedwait(cond, mutex, abstime);
    }
    
    int ret = -1;
    if (jsi_record_writer_init_completed.load() != 0 &&
         jsi_pthread_wrapper_init_completed.load() != 0 &&
         jsi_record_writer_fini_completed.load() != 1 &&
         jsi_pthread_wrapper_fini_completed.load() != 1 &&
         jsi_record_writer_thread_local_wrapper_completed.load() != 1) 
    {

         auto rec = (record_pthread_cond_timedwait_t*) ALLOCATE(
             sizeof(record_pthread_cond_timedwait_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        //  JSI_LOG(JSILOG_INFO, "pthread_cond_timedwait2.\n");
         rec = jsi_enter_pthread_cond_timedwait(rec, event_Pthread_Cond_Timedwait);
         ret = real_pthread_cond_timedwait(cond, mutex, abstime);
         jsi_exit_pthread_cond_timedwait(rec);

    } else {
         ret = real_pthread_cond_timedwait(cond, mutex, abstime);
    }

    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ int pthread_cond_signal(pthread_cond_t *cond) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_cond_signal(cond);
    }
    
    int ret = -1;
    if (jsi_record_writer_init_completed.load() != 0 &&
         jsi_pthread_wrapper_init_completed.load() != 0 &&
         jsi_record_writer_fini_completed.load() != 1 &&
         jsi_pthread_wrapper_fini_completed.load() != 1 &&
         jsi_record_writer_thread_local_wrapper_completed.load() != 1) 
    {

         auto rec = (record_pthread_cond_signal_t*) ALLOCATE(
             sizeof(record_pthread_cond_signal_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        //  JSI_LOG(JSILOG_INFO, "pthread_cond_signal2.\n");
         rec = jsi_enter_pthread_cond_signal(rec, event_Pthread_Cond_Signal);
         ret = real_pthread_cond_signal(cond);
         jsi_exit_pthread_cond_signal(rec);

    } else {
         ret = real_pthread_cond_signal(cond);
    }

    jsi_safe_exit();

    return ret;
}

_EXTERN_C_ int pthread_cond_broadcast(pthread_cond_t *cond) {
    ensure_pthread_init();

    if (enable_pthread_collect == false || !jsi_safe_enter()) {
        return real_pthread_cond_broadcast(cond);
    }
    
    int ret = -1;
    if (jsi_record_writer_init_completed.load() != 0 &&
         jsi_pthread_wrapper_init_completed.load() != 0 &&
         jsi_record_writer_fini_completed.load() != 1 &&
         jsi_pthread_wrapper_fini_completed.load() != 1 &&
         jsi_record_writer_thread_local_wrapper_completed.load() != 1) 
    {

         auto rec = (record_pthread_cond_broadcast_t*) ALLOCATE(
             sizeof(record_pthread_cond_broadcast_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        //  JSI_LOG(JSILOG_INFO, "pthread_cond_broadcast2.\n");
         rec = jsi_enter_pthread_cond_broadcast(rec, event_Pthread_Cond_Broadcast);
         ret = real_pthread_cond_broadcast(cond);
         jsi_exit_pthread_cond_broadcast(rec);

    } else {
         ret = real_pthread_cond_broadcast(cond);
    }

    jsi_safe_exit();

    return ret;
}


