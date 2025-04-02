#include "utils/tsc_timer.h"
#include "utils/jsi_log.h"
#include "record/record_reader.h"
#include "record/record_type.h"
#include "record/record_writer.h"
#include "utils/safe.hpp"
#ifdef ENABLE_PMU
#include "instrument/pmu_collector.h"
#endif

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdexcept>

#include <omp.h>
#include <ompt.h>

static ompt_set_callback_t ompt_set_callback = NULL;
/*
 * OMPT callbacks implementation
 */
static void on_ompt_callback_thread_begin(
    ompt_thread_t thread_type,
    ompt_data_t *thread_data)
{
    JSI_LOG(JSILOG_INFO, "[OMPT INFO] OMPT THREAD BEGIN CALLBACK!\n");
    JSI_LOG(JSILOG_INFO, "%d %d \n", in_tool, thread_data_initialized);
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_thread_begin_t *rec = (ext_record_ompt_thread_begin_t *)ALLOCATE(sizeof(ext_record_ompt_thread_begin_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Thread_Begin;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_thread_begin specific data
    rec[0].thread_type = thread_type;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_thread_begin_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_thread_end(
    ompt_data_t *thread_data)
{
    JSI_LOG(JSILOG_INFO, "[OMPT INFO] OMPT THREAD END CALLBACK!\n");
    JSI_LOG(JSILOG_INFO, "%d %d \n", in_tool, thread_data_initialized);
    if (!jsi_safe_enter()) {
        return;
    }
    
    ext_record_ompt_thread_end_t *rec = (ext_record_ompt_thread_end_t *)ALLOCATE(sizeof(ext_record_ompt_thread_end_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Thread_End;
    rec[0].thread_id = _ConfigHelper::get_pid();
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_thread_end_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_parallel_begin(
    ompt_data_t *encountering_task_data,
    const ompt_frame_t *encountering_task_frame,
    ompt_data_t *parallel_data,
    unsigned int requested_parallelism,
    int flags,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_parallel_begin_t *rec = (ext_record_ompt_parallel_begin_t *)ALLOCATE(sizeof(ext_record_ompt_parallel_begin_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Parallel_Begin;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_parallel_begin specific data
    rec[0].encountering_task_id = encountering_task_data ? encountering_task_data->value : -1;;
    rec[0].parallel_id = parallel_data ? parallel_data->value : -1;
    rec[0].requested_parallelism = requested_parallelism;
    rec[0].flags = flags;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_parallel_begin_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_parallel_end(
    ompt_data_t *parallel_data,
    ompt_data_t *encountering_task_data,
    int flags,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_parallel_end_t *rec = (ext_record_ompt_parallel_end_t *)ALLOCATE(sizeof(ext_record_ompt_parallel_end_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Parallel_End;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_parallel_end specific data
    rec[0].parallel_id = parallel_data ? parallel_data->value : -1;
    rec[0].encountering_task_id = encountering_task_data ? encountering_task_data->value : -1;
    rec[0].flags = flags;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_parallel_end_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_work(
    ompt_work_t wstype,
    ompt_scope_endpoint_t endpoint,
    ompt_data_t *parallel_data,
    ompt_data_t *task_data,
    uint64_t count,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_work_t *rec = (ext_record_ompt_work_t *)ALLOCATE(sizeof(ext_record_ompt_work_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Work;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_work specific data
    rec[0].wstype = wstype;
    rec[0].endpoint = endpoint;
    rec[0].parallel_id = parallel_data ? parallel_data->value : -1;
    rec[0].task_id = task_data ? task_data->value : -1;
    rec[0].count = count;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_work_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_task_create(
    ompt_data_t *encountering_task_data,
    const ompt_frame_t *encountering_task_frame,
    ompt_data_t *new_task_data,
    int flags,
    int has_dependences,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_task_create_t *rec = (ext_record_ompt_task_create_t *)ALLOCATE(sizeof(ext_record_ompt_task_create_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Task_Create;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_task_create specific data
    rec[0].encountering_task_id = encountering_task_data ? encountering_task_data->value : -1;
    rec[0].new_task_id = new_task_data ? new_task_data->value : -1;
    rec[0].flags = flags;
    rec[0].has_dependences = has_dependences;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_task_create_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_dependences(
    ompt_data_t *task_data,
    const ompt_dependence_t *deps,
    int ndeps)
{
    if (!jsi_safe_enter()) {
        return;
    }
    const ompt_dependence_type_t invalid_dependence_type = static_cast<ompt_dependence_type_t>(0);
    ext_record_ompt_dependences_t *rec = (ext_record_ompt_dependences_t *)ALLOCATE(sizeof(ext_record_ompt_dependences_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Dependences;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_dependences specific data
    rec[0].task_id = task_data ? task_data->value : -1;
    rec[0].dep_variable = deps ? deps->variable.value : -1;
    rec[0].dep_type = deps ? deps->dependence_type : invalid_dependence_type;
    rec[0].ndeps = ndeps;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_dependences_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_task_dependence(
    ompt_data_t *src_task_data,
    ompt_data_t *sink_task_data)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_task_dependence_t *rec = (ext_record_ompt_task_dependence_t *)ALLOCATE(sizeof(ext_record_ompt_task_dependence_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Task_Dependence;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_task_dependence specific data
    rec[0].src_task_id = src_task_data ? src_task_data->value : -1;
    rec[0].sink_task_id = sink_task_data ? sink_task_data->value : -1;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_task_dependence_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_task_schedule(
    ompt_data_t *prior_task_data,
    ompt_task_status_t prior_task_status,
    ompt_data_t *next_task_data)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_task_schedule_t *rec = (ext_record_ompt_task_schedule_t *)ALLOCATE(sizeof(ext_record_ompt_task_schedule_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Task_Schedule;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_task_schedule specific data
    rec[0].prior_task_id = prior_task_data ? prior_task_data->value : -1;
    rec[0].prior_task_status = prior_task_status;
    rec[0].next_task_id = next_task_data ? next_task_data->value : -1;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_task_schedule_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_implicit_task(
    ompt_scope_endpoint_t endpoint,
    ompt_data_t *parallel_data,
    ompt_data_t *task_data,
    unsigned int actual_parallelism,
    unsigned int index,
    int flags)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_implicit_task_t *rec = (ext_record_ompt_implicit_task_t *)ALLOCATE(sizeof(ext_record_ompt_implicit_task_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Implicit_Task;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_implicit_task specific data
    rec[0].endpoint = endpoint;
    rec[0].parallel_id = parallel_data ? parallel_data->value : -1;
    rec[0].task_id = task_data ? task_data->value : -1;
    rec[0].actual_parallelism = actual_parallelism;
    rec[0].index = index;
    rec[0].flags = flags;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_implicit_task_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_master(
    ompt_scope_endpoint_t endpoint,
    ompt_data_t *parallel_data,
    ompt_data_t *task_data,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_master_t *rec = (ext_record_ompt_master_t *)ALLOCATE(sizeof(ext_record_ompt_master_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Master;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_master specific data
    rec[0].endpoint = endpoint;
    rec[0].parallel_id = parallel_data ? parallel_data->value : -1;
    rec[0].task_id = task_data ? task_data->value : -1;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_master_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_sync_region(
    ompt_sync_region_t kind,
    ompt_scope_endpoint_t endpoint,
    ompt_data_t *parallel_data,
    ompt_data_t *task_data,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_sync_region_t *rec = (ext_record_ompt_sync_region_t *)ALLOCATE(sizeof(ext_record_ompt_sync_region_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Sync_Region;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_sync_region specific data
    rec[0].kind = kind;
    rec[0].endpoint = endpoint;
    rec[0].parallel_id = parallel_data ? parallel_data->value : -1;
    rec[0].task_id = task_data ? task_data->value : -1;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_sync_region_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_sync_region_wait(
    ompt_sync_region_t kind,
    ompt_scope_endpoint_t endpoint,
    ompt_data_t *parallel_data,
    ompt_data_t *task_data,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_sync_region_t *rec = (ext_record_ompt_sync_region_t *)ALLOCATE(sizeof(ext_record_ompt_sync_region_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Sync_Region_Wait;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_sync_region specific data
    rec[0].kind = kind;
    rec[0].endpoint = endpoint;
    rec[0].parallel_id = parallel_data ? parallel_data->value : -1;
    rec[0].task_id = task_data ? task_data->value : -1;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_sync_region_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_mutex_acquire(
    ompt_mutex_t kind,
    unsigned int hint,
    unsigned int impl,
    ompt_wait_id_t wait_id,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_mutex_acquire_t *rec = (ext_record_ompt_mutex_acquire_t *)ALLOCATE(sizeof(ext_record_ompt_mutex_acquire_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Mutex_Acquire;
    rec[0].thread_id = _ConfigHelper::get_pid();
    JSI_INFO("on_ompt_callback_mutex_acquire: %d\n", rec[0].thread_id);
    // ompt_callback_mutex_acquire specific data
    rec[0].kind = kind;
    rec[0].hint = hint;
    rec[0].impl = impl;
    rec[0].wait_id = wait_id;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_mutex_acquire_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_lock_init(
    ompt_mutex_t kind,
    unsigned int hint,
    unsigned int impl,
    ompt_wait_id_t wait_id,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_mutex_acquire_t *rec = (ext_record_ompt_mutex_acquire_t *)ALLOCATE(sizeof(ext_record_ompt_mutex_acquire_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Lock_Init;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_mutex_acquire specific data
    rec[0].kind = kind;
    rec[0].hint = hint;
    rec[0].impl = impl;
    rec[0].wait_id = wait_id;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_mutex_acquire_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_mutex_acquired(
    ompt_mutex_t kind,
    ompt_wait_id_t wait_id,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_mutex_t *rec = (ext_record_ompt_mutex_t *)ALLOCATE(sizeof(ext_record_ompt_mutex_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Mutex_Acquired;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_mutex specific data
    rec[0].kind = kind;
    rec[0].wait_id = wait_id;
    rec[0].codeptr_ra = codeptr_ra;
    JSI_INFO("on_ompt_callback_mutex_acquired: %d\n", rec[0].thread_id);
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_mutex_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_mutex_released(
    ompt_mutex_t kind,
    ompt_wait_id_t wait_id,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_mutex_t *rec = (ext_record_ompt_mutex_t *)ALLOCATE(sizeof(ext_record_ompt_mutex_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Mutex_Released;
    rec[0].thread_id = _ConfigHelper::get_pid();
    JSI_INFO("on_ompt_callback_mutex_released: %d\n", rec[0].thread_id);
    // ompt_callback_mutex specific data
    rec[0].kind = kind;
    rec[0].wait_id = wait_id;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_mutex_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_lock_destroy(
    ompt_mutex_t kind,
    ompt_wait_id_t wait_id,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_mutex_t *rec = (ext_record_ompt_mutex_t *)ALLOCATE(sizeof(ext_record_ompt_mutex_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Lock_Destroy;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_mutex specific data
    rec[0].kind = kind;
    rec[0].wait_id = wait_id;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_mutex_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit(); 
}

static void on_ompt_callback_nest_lock(
    ompt_scope_endpoint_t endpoint,
    ompt_wait_id_t wait_id,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_nest_lock_t *rec = (ext_record_ompt_nest_lock_t *)ALLOCATE(sizeof(ext_record_ompt_nest_lock_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Nest_Lock;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_nest_lock specific data
    rec[0].endpoint = endpoint;
    rec[0].wait_id = wait_id;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_nest_lock_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_flush(
    ompt_data_t *thread_data,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_flush_t *rec = (ext_record_ompt_flush_t *)ALLOCATE(sizeof(ext_record_ompt_flush_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Flush;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_flush specific data
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_flush_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

static void on_ompt_callback_cancel(
    ompt_data_t *task_data,
    int flags,
    const void *codeptr_ra)
{
    if (!jsi_safe_enter()) {
        return;
    }
    ext_record_ompt_cancel_t *rec = (ext_record_ompt_cancel_t *)ALLOCATE(sizeof(ext_record_ompt_cancel_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    uint64_t t = get_tsc_raw();
    rec[0].record.timestamps.enter = t;
    rec[0].record.timestamps.exit = t;
    rec[0].record.MsgType = event_OMPT_Cancel;
    rec[0].thread_id = _ConfigHelper::get_pid();
    // ompt_callback_cancel specific data
    rec[0].task_id = task_data ? task_data->value : -1;
    rec[0].flags = flags;
    rec[0].codeptr_ra = codeptr_ra;
#ifdef ENABLE_BACKTRACE
    backtrace_context_t ctxt = 0;
    if (jsi_backtrace_enabled)
    {
        ctxt = backtrace_context_get();
    }
    rec[0].record.ctxt = ctxt;
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled)
    {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(ext_record_ompt_cancel_t) + 2 * sizeof(uint64_t) * jsi_pmu_num);
    jsi_safe_exit();
}

#define register_callback_t(name, type)                             \
    do                                                              \
    {                                                               \
        type f_##name = &on_##name;                                 \
        if (ompt_set_callback(name, (ompt_callback_t)f_##name) ==   \
            ompt_set_never)                                         \
            printf("0: Could not register callback '" #name "'\n"); \
    } while (0)

#define register_callback(name) register_callback_t(name, name##_t)

int ompt_initialize(
    ompt_function_lookup_t lookup,
    int initial_device_num,
    ompt_data_t *tool_data)
{
    // register Entry Points in the OMPT Callback Interface
    JSI_LOG(JSILOG_INFO, "Initialize JSI OpenMP Wrapper Library.\n");
    RecordWriter::metaSectionStart("MPI_COMM_WORLD");
    RecordWriter::metaStore<int64_t>("MPI_COMM_WORLD", 0);
    RecordWriter::metaStore<int>("rank", 0);
    RecordWriter::metaStore<int>("size", 0);
    RecordWriter::metaSectionEnd("MPI_COMM_WORLD");


    ompt_set_callback = (ompt_set_callback_t)lookup("ompt_set_callback");

    // register OMPT Callbacks
    register_callback(ompt_callback_thread_begin);
    register_callback(ompt_callback_thread_end);
    register_callback(ompt_callback_parallel_begin);
    register_callback(ompt_callback_parallel_end);
    register_callback(ompt_callback_work);
    register_callback(ompt_callback_task_create);
    register_callback(ompt_callback_dependences);
    register_callback(ompt_callback_task_dependence);
    register_callback(ompt_callback_task_schedule);
    register_callback(ompt_callback_master);
    register_callback(ompt_callback_mutex_acquire);
    register_callback_t(ompt_callback_lock_init, ompt_callback_mutex_acquire_t);
    register_callback_t(ompt_callback_mutex_acquired, ompt_callback_mutex_t);
    register_callback_t(ompt_callback_mutex_released, ompt_callback_mutex_t);
    register_callback_t(ompt_callback_lock_destroy, ompt_callback_mutex_t);
    register_callback(ompt_callback_nest_lock);
    register_callback(ompt_callback_flush);
    register_callback(ompt_callback_cancel);

    return 1; // success
}

void ompt_finalize(ompt_data_t *tool_data) {
    JSI_LOG(JSILOG_INFO, "Finalize JSI OpenMP Wrapper Library.\n");
}

#ifdef __cplusplus
extern "C"
{
#endif
    ompt_start_tool_result_t *ompt_start_tool(
        unsigned int omp_version,
        const char *runtime_version)
    {
        static ompt_start_tool_result_t ompt_start_tool_result = {&ompt_initialize, &ompt_finalize, 0};
        return &ompt_start_tool_result;
    }
#ifdef __cplusplus
}
#endif