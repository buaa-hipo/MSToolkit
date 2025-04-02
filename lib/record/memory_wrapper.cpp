#ifndef _EXTERN_C_
#define _EXTERN_C_ extern "C"
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_instrument.h"
#include <dlfcn.h>
#include <unistd.h>
#include <execinfo.h>
#include "utils/safe.hpp"

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

thread_local bool memory_wrapper_td_initialized = true;

_EXTERN_C_ void* malloc(size_t size) {
    
    // typedef void*(*FUNC)(size_t);
    // FUNC f = (FUNC)dlsym(RTLD_NEXT, "malloc");
    // pthread_mutex_lock(&init_func_lock);
    if(jsi_memory_function_init_completed.load() == 0)
    {
        jsi_memory_function_init();
        jsi_memory_function_init_completed.store(1);
    }
    // pthread_mutex_unlock(&init_func_lock);
    
    if(!enable_memory_collect || !memory_wrapper_td_initialized)
    {
        jsi_mark_unsafe_enter();
        void* ret = real_malloc(size);
        jsi_mark_unsafe_exit();
        return ret;
    }   

    if(!jsi_safe_enter()) {
        return real_malloc(size);
    }

    // if(sem_prevent_recursion == 0)
    // {
    //     return real_malloc(size);
    // }

    void *ptr = NULL;
    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 &&\
    jsi_memory_wrapper_init_completed.load() != 0 && \
    jsi_memory_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        // sem_prevent_recursion = 0;
        auto ctxt = (record_memory_malloc*) ALLOCATE(
            sizeof(record_memory_malloc) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        ctxt = jsi_enter_malloc(ctxt, event_Memory_Malloc);
        ptr = real_malloc(size);
        jsi_exit_malloc(ctxt, ptr, size);
        // sem_prevent_recursion = 1;
    }
    else
    {
        ptr = real_malloc(size);
    }
    
    jsi_safe_exit();

    return ptr;
}

_EXTERN_C_ void* calloc(size_t n, size_t size) {

    // typedef void*(*FUNC)(size_t, size_t);
    // FUNC f = (FUNC)dlsym(RTLD_NEXT, "calloc");
    if(jsi_memory_function_init_completed.load() == 0)
    {
        return (void*)early_calloc_buf;
        // return NULL;
    }

    if(!enable_memory_collect || !memory_wrapper_td_initialized)
    {
        jsi_mark_unsafe_enter();
        void* ret = real_calloc(n, size);
        jsi_mark_unsafe_exit();
        return ret;
    }   

    if(!jsi_safe_enter()) {
        return real_calloc(n, size);
    }

    // if(sem_prevent_recursion == 0)
    // {
    //     return real_calloc(n, size);
    // }

    void *ptr = NULL;
    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 &&\
    jsi_memory_wrapper_init_completed.load() != 0 && \
    jsi_memory_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        // sem_prevent_recursion = 0;
        auto ctxt = (record_memory_calloc*) ALLOCATE(
            sizeof(record_memory_calloc) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        ctxt = jsi_enter_calloc(ctxt, event_Memory_Calloc);
        ptr = real_calloc(n, size);
        jsi_exit_calloc(ctxt, ptr, n * size);
        // sem_prevent_recursion = 1;
    }
    else
    {
        ptr = real_calloc(n, size);
    }
    
    jsi_safe_exit();

    return ptr;
}

_EXTERN_C_ void* realloc(void* ptr, size_t size) {

    // typedef void*(*FUNC)(void*, size_t);
    // FUNC f = (FUNC)dlsym(RTLD_NEXT, "realloc");
    // pthread_mutex_lock(&init_func_lock);
    if(jsi_memory_function_init_completed.load() == 0)
    {
        jsi_memory_function_init();
        jsi_memory_function_init_completed.store(1);
    }
    // pthread_mutex_unlock(&init_func_lock);
    if(!enable_memory_collect || !memory_wrapper_td_initialized)
    {
        jsi_mark_unsafe_enter();
        void* ret = real_realloc(ptr, size);
        jsi_mark_unsafe_exit();
        return ret;
    }   

    if(!jsi_safe_enter()) {
        return real_realloc(ptr, size);
    }

    // if(sem_prevent_recursion == 0)
    // {
    //     return real_realloc(ptr, size);
    // }

    void *newptr = NULL;
    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 &&\
    jsi_memory_wrapper_init_completed.load() != 0 && \
    jsi_memory_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        // sem_prevent_recursion = 0;
        auto ctxt = (record_memory_realloc*) ALLOCATE(
            sizeof(record_memory_realloc) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        ctxt = jsi_enter_realloc(ctxt, event_Memory_Realloc,ptr);
        newptr = real_realloc(ptr, size);
        jsi_exit_realloc(ctxt, newptr, size);
        // sem_prevent_recursion = 1;
    }
    else
    {
        newptr = real_realloc(ptr, size);
    }
    
    jsi_safe_exit();

    return newptr;
}

_EXTERN_C_ void free(void *ptr) {
    
    // typedef void (*FUNC)(void*);
    // FUNC f = (FUNC)dlsym(RTLD_NEXT, "free");
    // pthread_mutex_lock(&init_func_lock);
    if(jsi_memory_function_init_completed.load() == 0)
    {
        jsi_memory_function_init();
        jsi_memory_function_init_completed.store(1);
    }
    // pthread_mutex_unlock(&init_func_lock);

    if(!enable_memory_collect || !memory_wrapper_td_initialized)
    {
        jsi_mark_unsafe_enter();
        real_free(ptr);
        jsi_mark_unsafe_exit();
        return;
    }   

    if(!jsi_safe_enter()) {
        real_free(ptr);
        return;
    }

    // if(sem_prevent_recursion == 0)
    // {
    //     real_free(ptr);
    //     return;
    // }

    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 &&\
    jsi_memory_wrapper_init_completed.load() != 0 && \
    jsi_memory_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        // sem_prevent_recursion = 0;
        auto ctxt = (record_memory_free*) ALLOCATE(
            sizeof(record_memory_free) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        ctxt = jsi_enter_free(ctxt, event_Memory_Free, ptr);
        real_free(ptr);
        jsi_exit_free(ctxt);
        // sem_prevent_recursion = 1;
        
    }
    else
    {
        real_free(ptr);
    }

    jsi_safe_exit();

}

_EXTERN_C_ void* memalign(size_t alignment, size_t size) {
    
    // typedef void*(*FUNC)(size_t);
    // FUNC f = (FUNC)dlsym(RTLD_NEXT, "memalign");
    // pthread_mutex_lock(&init_func_lock);
    if(jsi_memory_function_init_completed.load() == 0)
    {
        jsi_memory_function_init();
        jsi_memory_function_init_completed.store(1);
    }
    // pthread_mutex_unlock(&init_func_lock);
    
    if(!enable_memory_collect || !memory_wrapper_td_initialized)
    {
        jsi_mark_unsafe_enter();
        void* ret = real_memalign(alignment, size);
        jsi_mark_unsafe_exit();
        return ret;
    }   

    if(!jsi_safe_enter()) {
        return real_memalign(alignment, size);
    }

    // if(sem_prevent_recursion == 0)
    // {
    //     return real_memalign(alignment, size);
    // }

    void *ptr = NULL;
    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 &&\
    jsi_memory_wrapper_init_completed.load() != 0 && \
    jsi_memory_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        // sem_prevent_recursion = 0;
        auto ctxt = (record_memory_memalign*) ALLOCATE(
            sizeof(record_memory_memalign) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        ctxt = jsi_enter_memalign(ctxt, event_Memory_Memalign);
        ptr = real_memalign(alignment, size);
        jsi_exit_memalign(ctxt, ptr, alignment, size);
        // sem_prevent_recursion = 1;
    }
    else
    {
        ptr = real_memalign(alignment, size);
    }
    
    jsi_safe_exit();

    return ptr;
}

_EXTERN_C_ void* aligned_alloc(size_t alignment, size_t size) {
    
    // typedef void*(*FUNC)(size_t);
    // FUNC f = (FUNC)dlsym(RTLD_NEXT, "memalign");
    // pthread_mutex_lock(&init_func_lock);
    if(jsi_memory_function_init_completed.load() == 0)
    {
        jsi_memory_function_init();
        jsi_memory_function_init_completed.store(1);
    }
    // pthread_mutex_unlock(&init_func_lock);
    
    if(!enable_memory_collect || !memory_wrapper_td_initialized)
    {
        jsi_mark_unsafe_enter();
        void* ret = real_aligned_alloc(alignment, size);
        jsi_mark_unsafe_exit();
        return ret;
    }   

    if(!jsi_safe_enter()) {
        return real_aligned_alloc(alignment, size);
    }

    // if(sem_prevent_recursion == 0)
    // {
    //     return real_aligned_alloc(alignment, size);
    // }

    void *ptr = NULL;
    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 &&\
    jsi_memory_wrapper_init_completed.load() != 0 && \
    jsi_memory_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        // sem_prevent_recursion = 0;
        auto ctxt = (record_memory_aligned_alloc*) ALLOCATE(
            sizeof(record_memory_aligned_alloc) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        ctxt = jsi_enter_aligned_alloc(ctxt, event_Memory_Aligned_Alloc);
        ptr = real_aligned_alloc(alignment, size);
        jsi_exit_aligned_alloc(ctxt, ptr, alignment, size);
        // sem_prevent_recursion = 1;
    }
    else
    {
        ptr = real_aligned_alloc(alignment, size);
    }
    
    jsi_safe_exit();

    return ptr;
}

_EXTERN_C_ int posix_memalign(void **memptr, size_t alignment, size_t size) {
    
    // typedef void*(*FUNC)(size_t);
    // FUNC f = (FUNC)dlsym(RTLD_NEXT, "memalign");
    // pthread_mutex_lock(&init_func_lock);
    if(jsi_memory_function_init_completed.load() == 0)
    {
        jsi_memory_function_init();
        jsi_memory_function_init_completed.store(1);
    }
    // pthread_mutex_unlock(&init_func_lock);
    
    if(!enable_memory_collect || !memory_wrapper_td_initialized)
    {
        jsi_mark_unsafe_enter();
        int ret = real_posix_memalign(memptr, alignment, size);
        jsi_mark_unsafe_exit();
        return ret;
    }   

    if(!jsi_safe_enter()) {
        return real_posix_memalign(memptr, alignment, size);
    }

    // if(sem_prevent_recursion == 0)
    // {
    //     return real_posix_memalign(alignment, size);
    // }

    int error_code;
    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 &&\
    jsi_memory_wrapper_init_completed.load() != 0 && \
    jsi_memory_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        // sem_prevent_recursion = 0;
        auto ctxt = (record_memory_posix_memalign*) ALLOCATE(
            sizeof(record_memory_posix_memalign) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        ctxt = jsi_enter_posix_memalign(ctxt, event_Memory_Posix_Memalign);
        error_code = real_posix_memalign(memptr, alignment, size);
        jsi_exit_posix_memalign(ctxt, *memptr, alignment, size, error_code);
        // sem_prevent_recursion = 1;
    }
    else
    {
        error_code = real_posix_memalign(memptr, alignment, size);
    }
    
    jsi_safe_exit();

    return error_code;
}