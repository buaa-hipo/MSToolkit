#ifndef __INSTRUMENT_MEMORY__
#define __INSTRUMENT_MEMORY__

#include "record/record_type.h"
#include "record/record_writer.h"
#include "instrument/pmu_collector.h"

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

#define MEMORY_WRAPPER_INIT_PRIORITY (RECORD_INIT_PRIORITY+200)
#define MEMORY_WRAPPER_FINI_PRIORITY (RECORD_FINI_PRIORITY+200)

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

std::atomic<int> jsi_memory_wrapper_init_completed(0);
std::atomic<int> jsi_memory_wrapper_fini_completed(0);
std::atomic<int> jsi_memory_function_init_completed(0);
// int sem_prevent_recursion = 1;

// static pthread_mutex_t malloc_lock = PTHREAD_MUTEX_INITIALIZER;
// static pthread_mutex_t calloc_lock = PTHREAD_MUTEX_INITIALIZER;
// static pthread_mutex_t realloc_lock = PTHREAD_MUTEX_INITIALIZER;
// static pthread_mutex_t free_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t init_func_lock = PTHREAD_MUTEX_INITIALIZER;
static thread_local int sem_prevent_recursion = 1;

static thread_local int mpi_initialized = 0;
static thread_local int mpi_finalized = 0;  

int mem_rank = 0;

static char early_calloc_buf[128];

static void *(*real_malloc)(size_t) = (void*(*)(size_t))0x2;
static void *(*real_calloc)(size_t, size_t) = (void*(*)(size_t, size_t))0x3;
static void *(*real_realloc)(void*, size_t) = (void*(*)(void*, size_t))0x4;
static void (*real_free)(void*) = (void(*)(void*))0x5;
// static int (*real_MPI_Init)(int*, char***) = (int (*)(int*, char***))0x6;
// static int (*real_MPI_Finalize)() = (int (*)())0x7;

std::atomic<int> allocate_time(0);


void jsi_memory_function_init()
{
    // void* handle = dlopen("libc.so.6", RTLD_LAZY);
    real_malloc = (void*(*)(size_t))dlsym(RTLD_NEXT, "malloc");
    real_calloc = (void*(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
    real_realloc = (void*(*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
    real_free = (void(*)(void*))dlsym(RTLD_NEXT, "free");
    // real_MPI_Init = (int (*)(int*, char***))dlsym(RTLD_NEXT, "MPI_Init");
    // real_MPI_Finalize = (int (*)())dlsym(RTLD_NEXT, "MPI_Finalize");
    // dlclose(handle);
}

__attribute__((constructor (MEMORY_WRAPPER_INIT_PRIORITY)))
void jsi_memory_tracer_init(){
    // write(1, "jsi_memory_tracer_init!!!!!!!!!!!!!!!!!!!!!!\n", 45);

    pthread_mutex_lock(&init_func_lock);
    if(jsi_memory_function_init_completed.load() == 0)
    {
        jsi_memory_function_init();
        jsi_memory_function_init_completed.store(1);
    }
    pthread_mutex_unlock(&init_func_lock);


    JSI_LOG(JSILOG_INFO, "Initialize JSI MEMORY Wrapper Library.\n");

    jsi_memory_wrapper_init_completed.store(1);
}

__attribute__((destructor (MEMORY_WRAPPER_FINI_PRIORITY)))
void jsi_memory_tracer_finalize(){
    // write(1, "jsi_memory_tracer_finalize!!!!!!!!!!!!!!!!!!!!!!\n", 49);
    jsi_memory_wrapper_fini_completed.store(1);

    std::ifstream file("/proc/meminfo");
    std::unordered_map<std::string, std::int64_t> memInfo;
    std::string line;
    if (!file.is_open()) {
        std::cerr << "Cannot open /proc/meminfo" << std::endl;
        return;
    }
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        std::int64_t value;
        std::string unit;
        iss >> key >> value >> unit; // unit might be "kB"
        if (key != "") {
            key.pop_back(); // remove :
            memInfo[key] = value;
        }
    }
    file.close();
    std::string keys[] = {"MemTotal", "MemFree", "MemAvailable", "Shmem", "Buffers", "Cached"};
    RecordWriter::metaSectionStart("MEMORY_INFO");
    RecordWriter::metaStore<int>("rank", mem_rank);
    RecordWriter::metaStore<int64_t>("memory_total", memInfo["MemTotal"]);
    RecordWriter::metaStore<int64_t>("memory_free", memInfo["MemFree"]);
    RecordWriter::metaStore<int64_t>("memory_available", memInfo["MemAvailable"]);
    RecordWriter::metaSectionEnd("MEMORY_INFO");

    JSI_LOG(JSILOG_INFO, "Finalize JSI MEMORY Wrapper Library.\n");
    RecordWriter::metaSectionStart("MEMORY_INFO_END");
    RecordWriter::metaStore<int>("allocate_time", allocate_time);
    RecordWriter::metaSectionEnd("MEMORY_INFO_END");
}

inline __attribute__((always_inline))
record_memory_malloc* jsi_enter_malloc(record_memory_malloc* rec, uint16_t MsgType) {
    allocate_time += 1;
    // record_memory_malloc* rec = (record_memory_malloc*) alloca(
    //         sizeof(record_memory_malloc) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    // rec[0].type = record_memory::MALLOC;
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
void jsi_exit_malloc(record_memory_malloc* rec, const void* ptr, size_t size_bytes) {
    record_memory_malloc& r = reinterpret_cast<record_memory_malloc*>(rec)[0];
    r.record.timestamps.exit = get_tsc_raw();
    r.ptr = (const void*) ptr;
    r.size_bytes = size_bytes;
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_memory_malloc*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_memory_malloc) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
record_memory_calloc* jsi_enter_calloc(record_memory_calloc* rec, uint16_t MsgType) {
    allocate_time += 1;
    // record_memory* rec = (record_memory*) ALLOCATE(
    //         sizeof(record_memory) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    // rec[0].type = record_memory::CALLOC;
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
void jsi_exit_calloc(record_memory_calloc* rec, const void* ptr, size_t size_bytes) {
    record_memory_calloc& r = reinterpret_cast<record_memory_calloc*>(rec)[0];
    r.record.timestamps.exit = get_tsc_raw();
    r.ptr = (const void*) ptr;
    r.size_bytes = size_bytes;
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_memory_calloc*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_memory_calloc) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
record_memory_realloc* jsi_enter_realloc(record_memory_realloc* rec, uint16_t MsgType, const void* ptr) {
    allocate_time += 1;
    // record_memory* rec = (record_memory*) ALLOCATE(
    //         sizeof(record_memory) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    // rec[0].type = record_memory::REALLOC;
    rec[0].ptr = (const void*) ptr;
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
void jsi_exit_realloc(record_memory_realloc* rec, const void* newptr, size_t size_bytes) {
    record_memory_realloc& r = reinterpret_cast<record_memory_realloc*>(rec)[0];
    r.record.timestamps.exit = get_tsc_raw();
    r.newptr = (const void*) newptr;
    r.size_bytes = size_bytes;
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_memory_realloc*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_memory_realloc) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
record_memory_free* jsi_enter_free(record_memory_free* rec, uint16_t MsgType,const void* ptr) {
    // printf("jsi_enter_free\n");
    allocate_time += 1;
    // record_memory* rec = (record_memory*) ALLOCATE(
    //         sizeof(record_memory) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].record.MsgType = (int16_t)MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
    // rec[0].type = record_memory::FREE;
    rec[0].ptr = (const void*) ptr;
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
void jsi_exit_free(record_memory_free* rec) {
    // printf("jsi_exit_free\n");
    record_memory_free& r = reinterpret_cast<record_memory_free*>(rec)[0];
    r.record.timestamps.exit = get_tsc_raw();
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_memory_free*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((const record_t*)rec);
    DEALLOCATE(rec, sizeof(record_memory_free) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

#endif