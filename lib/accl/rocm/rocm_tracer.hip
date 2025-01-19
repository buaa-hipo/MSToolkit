#ifdef USE_hipApiArgsInit
#define HIP_PROF_HIP_API_STRING true
#endif
#include <roctracer.h>
#include <roctracer_hip.h>
#include "utils/jsi_log.h"
#include "record/record_type.h"
#include "record/record_writer.h"
#ifdef ENABLE_PMU
#include "instrument/pmu_collector.h"
#endif
#include "record/record_reader.h"
#include "instrument/backtrace.h"
#include "utils/tsc_timer.h"
#include <iostream>

#define ROCTRACER_INNER_BT_DEPTH 2

#define ROCM_TRACER_INIT_PRIORITY (RECORD_INIT_PRIORITY+50)
#define ROCM_TRACER_FINI_PRIORITY (RECORD_FINI_PRIORITY+50)

// 2MB trace cache size for async activity events
#define ASYNC_TRACE_CACHE_SIZE (2*1024*1024)

inline void CHECK(roctracer_status_t status) {
  if (status != ROCTRACER_STATUS_SUCCESS) {
    std::cerr << roctracer_error_string() << std::endl;
    abort();
  }
}

uint32_t GetPid() {
  static auto pid = getpid();
  return pid;
}

static bool _activity_initialized = false;
static bool _enable_backtrace = false;
static bool _enable_pmu = false;
static roctracer_pool_t* pool;
static RecordWriterExt* writer;

#ifndef USE_ROCTRACER_PHASE_DATA
/* The old version of roctracer API did not support user-defined phase data 
 * within the callback data, so we use a ring buffer instead. */
#define RING_SIZE 100
// TODO: make pmu dynamically allocated cache
#define MAX_PMU_SIZE 10
struct {
  uint64_t ts;
  uint64_t pmu[MAX_PMU_SIZE];
} ts_buff_ring[RING_SIZE] = {0};
bool ts_valid_ring[RING_SIZE] = {0};
#endif

void buffer_callback(const char* begin, const char* end, void* arg) {
#ifdef DEBUG
  fprintf(stdout, "#### %p, %p, %lu\n", begin, end, ((uint64_t)end-(uint64_t)begin));
  for (const roctracer_record_t* record = (const roctracer_record_t*)begin;
       record < (const roctracer_record_t*)end; CHECK(roctracer_next_record(record, &record))) {
    fprintf(stdout, "\t%s\tcorrelation_id(%lu) time_ns(%lu:%lu)\n",
            roctracer_op_string(record->domain, record->op, record->kind), record->correlation_id,
            record->begin_ns, record->end_ns);
  }
#endif
  // uint64_t NChar = ((uint64_t)end-(uint64_t)begin);
  // writer->writeSection(begin, NChar);
  for (const roctracer_record_t* record = (const roctracer_record_t*)begin;
       record < (const roctracer_record_t*)end; CHECK(roctracer_next_record(record, &record))) {
        // storing writer
        ext_record_accl* r = (ext_record_accl*)writer->allocate(sizeof(ext_record_accl));
        r->sym_key.accl_key.domain = record->domain;
        r->sym_key.accl_key.op = record->op;
        r->sym_key.accl_key.kind = record->kind;
        r->correlation_id = record->correlation_id;
        r->begin_ns = record->begin_ns;
        r->end_ns = record->end_ns;
        // op string buffering
        writer->writeStringBuffer(r->sym_key.key, roctracer_op_string(record->domain, record->op, record->kind));
  }
}

void activity_init() {
    roctracer_properties_t properties{};
    properties.buffer_callback_fun = buffer_callback;
    properties.buffer_callback_arg = nullptr;
    properties.buffer_size = 1024;
    CHECK(roctracer_open_pool_expl(&properties, &pool));
    CHECK(roctracer_enable_domain_activity_expl(ACTIVITY_DOMAIN_HIP_OPS, pool));
    CHECK(roctracer_enable_domain_activity_expl(ACTIVITY_DOMAIN_HSA_OPS, pool));
    writer = new RecordWriterExt();
    writer->init(JSI_TRACE_ACCL_ROCM);
    writer->setCacheSize(ASYNC_TRACE_CACHE_SIZE);
    _activity_initialized = true;
}

void hip_api_callback(uint32_t domain, uint32_t cid, const void* callback_data, void* arg) {
  ASSERT_MSG(domain==ACTIVITY_DOMAIN_HIP_API, "callback domain not match!");
  const hip_api_data_t* data = static_cast<const hip_api_data_t*>(callback_data);
#ifdef DEBUG
  fprintf(stdout, "<%s id(%u)\tcorrelation_id(%lu) %s pid(%d)>\n",
          roctracer_op_string(domain, cid, 0), cid, data->correlation_id,
          (data->phase == ACTIVITY_API_PHASE_ENTER) ? "on-enter" : "on-exit", GetPid());
#endif
  uint64_t ts = get_tsc_raw();
  if (data->phase == ACTIVITY_API_PHASE_ENTER) {
#ifdef USE_ROCTRACER_PHASE_DATA
    hip_api_data_t* t = const_cast<hip_api_data_t*>(data);
    t->phase_data = reinterpret_cast<uint64_t*>(ts);
#else
    size_t ri = data->correlation_id % RING_SIZE;
    if (!ts_valid_ring[ri]) {
      ts_buff_ring[ri].ts = ts;
      if (_enable_pmu) {
        pmu_collector_get_all(ts_buff_ring[ri].pmu);
      }
      ts_valid_ring[ri] = true;
    } else {
      JSI_ERROR("TS BUFF RING COLLISION DETECTED. USE LARGER RING_SIZE!");
    }
#endif
  } else {
#ifdef USE_ROCTRACER_PHASE_DATA
    uint64_t enter_ts = reinterpret_cast<uint64_t>(data->phase_data);
#else
    size_t ri = data->correlation_id % RING_SIZE;
    uint64_t enter_ts = ts_buff_ring[ri].ts;
    if (!ts_valid_ring[ri]) {
      JSI_ERROR("The timestamp in the ring buffer is invalid!");
    } else {
      ts_valid_ring[ri] = false;
    }
#endif
    backtrace_context_t ctxt = 0;
    if (_enable_backtrace) {
      ctxt = backtrace_context_get(ROCTRACER_INNER_BT_DEPTH);
    }
    switch(cid) {
      case HIP_API_ID_hipLaunchKernel: {
        record_activity_launch_t* rec = (record_activity_launch_t*) RecordWriter::allocate(
          sizeof(record_activity_launch_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
          if (_enable_pmu) {
              uint64_t* counters = RecordWriterHelper::counters(rec);
              memcpy(counters, ts_buff_ring[ri].pmu, jsi_pmu_num*sizeof(uint64_t));
              pmu_collector_get_all(counters+jsi_pmu_num);
          }
#endif
        rec[0].record.MsgType = JSI_GET_ACCL_API_MSGTYPE(cid);
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].sharedMemBytes = data->args.hipLaunchKernel.sharedMemBytes;
        rec[0].stream = data->args.hipLaunchKernel.stream;
        rec[0].blockNum.x = data->args.hipLaunchKernel.numBlocks.x;
        rec[0].blockNum.y = data->args.hipLaunchKernel.numBlocks.y;
        rec[0].blockNum.z = data->args.hipLaunchKernel.numBlocks.z;
        rec[0].blockDim.x = data->args.hipLaunchKernel.dimBlocks.x;
        rec[0].blockDim.y = data->args.hipLaunchKernel.dimBlocks.y;
        rec[0].blockDim.z = data->args.hipLaunchKernel.dimBlocks.z;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;
        /*
        printf("%s (args=%p, *args=%p): %p %p %u\n", 
                  hipApiString((hip_api_id_t)cid, data),
                  data->args.hipLaunchKernel.args, 
                  *data->args.hipLaunchKernel.args, 
                  *reinterpret_cast<int**>((reinterpret_cast<uint8_t*>(*data->args.hipLaunchKernel.args)-0)),
                  *reinterpret_cast<int**>((reinterpret_cast<uint8_t*>(*data->args.hipLaunchKernel.args)-sizeof(int*))),
                  *reinterpret_cast<unsigned int*>(reinterpret_cast<uint8_t*>(*data->args.hipLaunchKernel.args)-sizeof(int*)-)
                  );
        */
        break;
      }
      case HIP_API_ID_hipMemcpy: {
        record_activity_memcpy_t* rec = (record_activity_memcpy_t*) RecordWriter::allocate(
          sizeof(record_activity_memcpy_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
          if (_enable_pmu) {
              uint64_t* counters = RecordWriterHelper::counters(rec);
              memcpy(counters, ts_buff_ring[ri].pmu, jsi_pmu_num*sizeof(uint64_t));
              pmu_collector_get_all(counters+jsi_pmu_num);
          }
#endif
        rec[0].record.MsgType = JSI_GET_ACCL_API_MSGTYPE(cid);
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpy.dst;
        rec[0].src = data->args.hipMemcpy.src;
        rec[0].sizeBytes = data->args.hipMemcpy.sizeBytes;
        rec[0].kind = data->args.hipMemcpy.kind;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;
        break;
      }
      case HIP_API_ID_hipMemcpyAsync: {
        record_activity_memcpy_async_t* rec = (record_activity_memcpy_async_t*) RecordWriter::allocate(
          sizeof(record_activity_memcpy_async_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
          if (_enable_pmu) {
              uint64_t* counters = RecordWriterHelper::counters(rec);
              memcpy(counters, ts_buff_ring[ri].pmu, jsi_pmu_num*sizeof(uint64_t));
              pmu_collector_get_all(counters+jsi_pmu_num);
          }
#endif
        rec[0].record.MsgType = JSI_GET_ACCL_API_MSGTYPE(cid);
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyAsync.dst;
        rec[0].src = data->args.hipMemcpyAsync.src;
        rec[0].sizeBytes = data->args.hipMemcpyAsync.sizeBytes;
        rec[0].kind = data->args.hipMemcpyAsync.kind;
        rec[0].stream = data->args.hipMemcpyAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;
        break;
      }
      default: {
        record_activity_t* rec = (record_activity_t*) RecordWriter::allocate(
          sizeof(record_activity_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
          if (_enable_pmu) {
              uint64_t* counters = RecordWriterHelper::counters(rec);
              memcpy(counters, ts_buff_ring[ri].pmu, jsi_pmu_num*sizeof(uint64_t));
              pmu_collector_get_all(counters+jsi_pmu_num);
          }
#endif
        rec[0].record.MsgType = JSI_GET_ACCL_API_MSGTYPE(cid);
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;
      }
    }
  }
}

__attribute__((constructor (ROCM_TRACER_INIT_PRIORITY)))
void jsi_accl_tracer_init(){
    JSI_LOG(JSILOG_INFO, "Initialize JSI ACCL Tracer <ROCM> Library (PID=%d).\n", GetPid());
    CHECK(roctracer_set_properties(ACTIVITY_DOMAIN_HIP_API, NULL));
    CHECK(roctracer_enable_domain_callback(ACTIVITY_DOMAIN_HIP_API, hip_api_callback, nullptr));
    activity_init();

    _enable_backtrace = jsi_backtrace_enabled;
    _enable_pmu = jsi_pmu_enabled;
}

__attribute__((destructor (ROCM_TRACER_FINI_PRIORITY)))
void jsi_accl_tracer_finalize(){
    JSI_LOG(JSILOG_INFO, "Finalize JSI ACCL Tracer <ROCM> Library (PID=%d).\n", GetPid());
    if (_activity_initialized) {
      roctracer_flush_activity_expl(pool);
      writer->flushCache();
      delete writer;
    }
}