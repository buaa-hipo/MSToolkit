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

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

_EXTERN_C_ void* malloc(size_t size) {
    
    // typedef void*(*FUNC)(size_t);
    // FUNC f = (FUNC)dlsym(RTLD_NEXT, "malloc");
    pthread_mutex_lock(&init_func_lock);
    if(jsi_memory_function_init_completed.load() == 0)
    {
        jsi_memory_function_init();
        jsi_memory_function_init_completed.store(1);
    }
    pthread_mutex_unlock(&init_func_lock);

    if(sem_prevent_recursion == 0)
    {
        return real_malloc(size);
    }

    // MPI_Initialized(&mpi_initialized);
    // MPI_Finalized(&mpi_finalized);
    void *ptr = NULL;
    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_memory_wrapper_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 &&\
    jsi_memory_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        sem_prevent_recursion = 0;
        // pthread_mutex_lock(&malloc_lock);
        auto ctxt = (record_memory_malloc*) ALLOCATE(
            sizeof(record_memory_malloc) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        ctxt = jsi_enter_malloc(ctxt, event_Memory_Malloc);
        ptr = real_malloc(size);
        jsi_exit_malloc(ctxt, ptr, size);
        // pthread_mutex_unlock(&malloc_lock);
        sem_prevent_recursion = 1;
    }
    else
    {
        ptr = real_malloc(size);
    }

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

    if(sem_prevent_recursion == 0)
    {
        return real_calloc(n, size);
    }

    // MPI_Initialized(&mpi_initialized);
    // MPI_Finalized(&mpi_finalized);
    void *ptr = NULL;
    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_memory_wrapper_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 && \
    jsi_memory_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        sem_prevent_recursion = 0;
        // pthread_mutex_lock(&calloc_lock);
        auto ctxt = (record_memory_calloc*) ALLOCATE(
            sizeof(record_memory_calloc) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        ctxt = jsi_enter_calloc(ctxt, event_Memory_Calloc);
        ptr = real_calloc(n, size);
        jsi_exit_calloc(ctxt, ptr, n * size);
        // pthread_mutex_unlock(&calloc_lock);
        sem_prevent_recursion = 1;
    }
    else
    {
        ptr = real_calloc(n, size);
    }

    return ptr;
}

_EXTERN_C_ void* realloc(void* ptr, size_t size) {

    // typedef void*(*FUNC)(void*, size_t);
    // FUNC f = (FUNC)dlsym(RTLD_NEXT, "realloc");
    pthread_mutex_lock(&init_func_lock);
    if(jsi_memory_function_init_completed.load() == 0)
    {
        jsi_memory_function_init();
        jsi_memory_function_init_completed.store(1);
    }
    pthread_mutex_unlock(&init_func_lock);

    if(sem_prevent_recursion == 0)
    {
        return real_realloc(ptr, size);
    }

    // MPI_Initialized(&mpi_initialized);
    // MPI_Finalized(&mpi_finalized);
    void *newptr = NULL;
    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_memory_wrapper_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 && \
    jsi_memory_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        sem_prevent_recursion = 0;
        // pthread_mutex_lock(&realloc_lock);
        auto ctxt = (record_memory_realloc*) ALLOCATE(
            sizeof(record_memory_realloc) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        ctxt = jsi_enter_realloc(ctxt, event_Memory_Realloc,ptr);
        newptr = real_realloc(ptr, size);
        jsi_exit_realloc(ctxt, newptr, size);
        // pthread_mutex_unlock(&realloc_lock);
        sem_prevent_recursion = 1;
    }
    else
    {
        newptr = real_realloc(ptr, size);
    }

    return newptr;
}

_EXTERN_C_ void free(void *ptr) {
    
    // typedef void (*FUNC)(void*);
    // FUNC f = (FUNC)dlsym(RTLD_NEXT, "free");
    pthread_mutex_lock(&init_func_lock);
    if(jsi_memory_function_init_completed.load() == 0)
    {
        jsi_memory_function_init();
        jsi_memory_function_init_completed.store(1);
    }
    pthread_mutex_unlock(&init_func_lock);

    if(sem_prevent_recursion == 0)
    {
        real_free(ptr);
        return;
    }

    // MPI_Initialized(&mpi_initialized);
    // MPI_Finalized(&mpi_finalized);
    if(\
    jsi_record_writer_init_completed.load() != 0 && \
    jsi_memory_wrapper_init_completed.load() != 0 && \
    jsi_record_writer_fini_completed.load() != 1 && \
    jsi_memory_wrapper_fini_completed.load() != 1 && \
    jsi_record_writer_thread_local_wrapper_completed.load() != 1)
    {
        // char a = (jsi_memory_wrapper_init_completed == 0) ? '0' : '1';
        // write(1, "in free: ", 9);
        // write(1, &a, 1);
        // write(1, "\n", 1);
        // pthread_mutex_lock(&free_lock);
        sem_prevent_recursion = 0;
        auto ctxt = (record_memory_free*) ALLOCATE(
            sizeof(record_memory_free) + sizeof(uint64_t) * 2 * jsi_pmu_num);
        ctxt = jsi_enter_free(ctxt, event_Memory_Free, ptr);
        real_free(ptr);
        jsi_exit_free(ctxt);
        sem_prevent_recursion = 1;
        // pthread_mutex_unlock(&free_lock);
    }
    else
    {
        real_free(ptr);
    }

}