#ifdef USE_hipApiArgsInit
#define HIP_PROF_HIP_API_STRING true
#endif
#include <roctracer.h>
#include <roctracer_hip.h>

#include "record/record_type.h"
#include "record/wrap_defines_macro.h"
#include "utils/jsi_log.h"
#ifdef ENABLE_PMU
#include "instrument/pmu_collector.h"
#endif
#include <iostream>

#include "instrument/pmu_collector.h"
#include "rocm_helper.h"
#include "utils/tsc_timer.h"
#include "utils/safe.hpp"

#define ROCTRACER_INNER_BT_DEPTH 2

// #define ROCM_TRACER_INIT_PRIORITY (RECORD_INIT_PRIORITY+50)
// #define ROCM_TRACER_FINI_PRIORITY (RECORD_FINI_PRIORITY+50)

// 2MB trace cache size for async activity events
#define ASYNC_TRACE_CACHE_SIZE (2 * 1024 * 1024)

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
static roctracer_pool_t *pool;
// static RecordWriterExt* writer;

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

// extern std::unique_ptr<pse::ral::DirSectionInterface> &
// get_thread_trace_dir();

void buffer_callback(const char *begin, const char *end, void *arg) {
  jsi_safe_enter_instr();
#ifdef DEBUG
  fprintf(stdout, "#### %p, %p, %lu\n", begin, end,
          ((uint64_t)end - (uint64_t)begin));
  for (const roctracer_record_t *record = (const roctracer_record_t *)begin;
       record < (const roctracer_record_t *)end;
       CHECK(roctracer_next_record(record, &record))) {
    fprintf(stdout, "\t%s\tcorrelation_id(%lu) time_ns(%lu:%lu)\n",
            roctracer_op_string(record->domain, record->op, record->kind),
            record->correlation_id, record->begin_ns, record->end_ns);
  }
#endif
  // uint64_t NChar = ((uint64_t)end-(uint64_t)begin);
  // writer->writeSection(begin, NChar);
  for (const roctracer_record_t *record = (const roctracer_record_t *)begin;
       record < (const roctracer_record_t *)end;
       CHECK(roctracer_next_record(record, &record))) {
    ext_record_accl *r = (ext_record_accl *)ALLOCATE(sizeof(ext_record_accl));
    r->correlation_id = record->correlation_id;
    r->begin_ns = record->begin_ns;
    r->end_ns = record->end_ns;

    auto res = writeStringSection(
        roctracer_op_string(record->domain, record->op, record->kind));
    // JSI_INFO("WRITING string section: [%s] to offset [%d]\n",
    // roctracer_op_string(record->domain, record->op, record->kind), res);
    r->sym_key.key = res;
    // RecordWriter::extStore(GetPid(), r, sizeof(ext_record_accl));
    JSI_DEBUG("Writing ext trace of id-%u\n", GetPid());
    helperExtStore(GetPid(), r, sizeof(ext_record_accl));
  }
  jsi_safe_exit_instr();
}

void activity_init() {
  roctracer_properties_t properties{};
  properties.buffer_callback_fun = buffer_callback;
  properties.buffer_callback_arg = nullptr;
  properties.buffer_size = 1024;
  CHECK(roctracer_open_pool_expl(&properties, &pool));
  CHECK(roctracer_enable_domain_activity_expl(ACTIVITY_DOMAIN_HIP_OPS, pool));
  CHECK(roctracer_enable_domain_activity_expl(ACTIVITY_DOMAIN_HSA_OPS, pool));
  _activity_initialized = true;
}

static size_t get_array_element_size(hipArray_Format type) {
  switch (type) {
    case HIP_AD_FORMAT_UNSIGNED_INT8:
      return 1;
    case HIP_AD_FORMAT_UNSIGNED_INT16:
      return 2;
    case HIP_AD_FORMAT_UNSIGNED_INT32:
      return 4;
    case HIP_AD_FORMAT_SIGNED_INT8:
      return 1;
    case HIP_AD_FORMAT_SIGNED_INT16:
      return 2;
    case HIP_AD_FORMAT_SIGNED_INT32:
      return 4;
    case HIP_AD_FORMAT_HALF:
      return 2;
    case HIP_AD_FORMAT_FLOAT:
      return 4;
    default:
      return 0;
  }
}

void hip_api_callback(uint32_t domain, uint32_t cid, const void *callback_data,
                      void *arg) {
  jsi_safe_enter_instr();
  ASSERT_MSG(domain == ACTIVITY_DOMAIN_HIP_API, "callback domain not match!");
  const hip_api_data_t *data =
      static_cast<const hip_api_data_t *>(callback_data);
#ifdef DEBUG
  fprintf(stdout, "<%s id(%u)\tcorrelation_id(%lu) %s pid(%d)>\n",
          roctracer_op_string(domain, cid, 0), cid, data->correlation_id,
          (data->phase == ACTIVITY_API_PHASE_ENTER) ? "on-enter" : "on-exit",
          GetPid());
#endif
  uint64_t ts = get_tsc_raw();
  if (data->phase == ACTIVITY_API_PHASE_ENTER) {
#ifdef USE_ROCTRACER_PHASE_DATA
    hip_api_data_t *t = const_cast<hip_api_data_t *>(data);
    t->phase_data = reinterpret_cast<uint64_t *>(ts);
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
    switch (cid) {
      case HIP_API_ID_hipLaunchKernel: {
        record_activity_launch_t *rec = (record_activity_launch_t *)ALLOCATE(
            sizeof(record_activity_launch_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipLaunchKernel;
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

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_launch_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }
      case HIP_API_ID_hipExtLaunchKernel: {
        record_activity_launch_t *rec = (record_activity_launch_t *)ALLOCATE(
            sizeof(record_activity_launch_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipExtLaunchKernel;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].sharedMemBytes = data->args.hipExtLaunchKernel.sharedMemBytes;
        rec[0].stream = data->args.hipExtLaunchKernel.stream;
        rec[0].blockNum.x = data->args.hipExtLaunchKernel.numBlocks.x;
        rec[0].blockNum.y = data->args.hipExtLaunchKernel.numBlocks.y;
        rec[0].blockNum.z = data->args.hipExtLaunchKernel.numBlocks.z;
        rec[0].blockDim.x = data->args.hipExtLaunchKernel.dimBlocks.x;
        rec[0].blockDim.y = data->args.hipExtLaunchKernel.dimBlocks.y;
        rec[0].blockDim.z = data->args.hipExtLaunchKernel.dimBlocks.z;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_launch_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipExtLaunchMultiKernelMultiDevice: {
        record_activity_launch_t *rec = (record_activity_launch_t *)ALLOCATE(
            sizeof(record_activity_launch_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipExtLaunchMultiKernelMultiDevice;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].sharedMemBytes = data->args.hipExtLaunchMultiKernelMultiDevice
                                    .launchParamsList->sharedMem;
        rec[0].stream = data->args.hipExtLaunchMultiKernelMultiDevice
                            .launchParamsList->stream;
        rec[0].blockNum.x = data->args.hipExtLaunchMultiKernelMultiDevice
                                .launchParamsList->gridDim.x;
        rec[0].blockNum.y = data->args.hipExtLaunchMultiKernelMultiDevice
                                .launchParamsList->gridDim.y;
        rec[0].blockNum.z = data->args.hipExtLaunchMultiKernelMultiDevice
                                .launchParamsList->gridDim.z;
        rec[0].blockDim.x = data->args.hipExtLaunchMultiKernelMultiDevice
                                .launchParamsList->blockDim.x;
        rec[0].blockDim.y = data->args.hipExtLaunchMultiKernelMultiDevice
                                .launchParamsList->blockDim.y;
        rec[0].blockDim.z = data->args.hipExtLaunchMultiKernelMultiDevice
                                .launchParamsList->blockDim.z;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_launch_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipLaunchCooperativeKernelMultiDevice: {
        record_activity_launch_t *rec = (record_activity_launch_t *)ALLOCATE(
            sizeof(record_activity_launch_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipLaunchCooperativeKernelMultiDevice;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].sharedMemBytes = data->args.hipLaunchCooperativeKernelMultiDevice
                                    .launchParamsList->sharedMem;
        rec[0].stream = data->args.hipLaunchCooperativeKernelMultiDevice
                            .launchParamsList->stream;
        rec[0].blockNum.x = data->args.hipLaunchCooperativeKernelMultiDevice
                                .launchParamsList->gridDim.x;
        rec[0].blockNum.y = data->args.hipLaunchCooperativeKernelMultiDevice
                                .launchParamsList->gridDim.y;
        rec[0].blockNum.z = data->args.hipLaunchCooperativeKernelMultiDevice
                                .launchParamsList->gridDim.z;
        rec[0].blockDim.x = data->args.hipLaunchCooperativeKernelMultiDevice
                                .launchParamsList->blockDim.x;
        rec[0].blockDim.y = data->args.hipLaunchCooperativeKernelMultiDevice
                                .launchParamsList->blockDim.y;
        rec[0].blockDim.z = data->args.hipLaunchCooperativeKernelMultiDevice
                                .launchParamsList->blockDim.z;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_launch_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipExtModuleLaunchKernel: {
        record_activity_launch_t *rec = (record_activity_launch_t *)ALLOCATE(
            sizeof(record_activity_launch_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipExtModuleLaunchKernel;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].sharedMemBytes =
            data->args.hipExtModuleLaunchKernel.sharedMemBytes;
        rec[0].stream = data->args.hipExtModuleLaunchKernel.hStream;
        rec[0].blockNum.x = data->args.hipExtModuleLaunchKernel.globalWorkSizeX;
        rec[0].blockNum.y = data->args.hipExtModuleLaunchKernel.globalWorkSizeY;
        rec[0].blockNum.z = data->args.hipExtModuleLaunchKernel.globalWorkSizeZ;
        rec[0].blockDim.x = data->args.hipExtModuleLaunchKernel.localWorkSizeX;
        rec[0].blockDim.y = data->args.hipExtModuleLaunchKernel.localWorkSizeY;
        rec[0].blockDim.z = data->args.hipExtModuleLaunchKernel.localWorkSizeZ;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_launch_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipHccModuleLaunchKernel: {
        record_activity_launch_t *rec = (record_activity_launch_t *)ALLOCATE(
            sizeof(record_activity_launch_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipHccModuleLaunchKernel;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].sharedMemBytes =
            data->args.hipHccModuleLaunchKernel.sharedMemBytes;
        rec[0].stream = data->args.hipHccModuleLaunchKernel.hStream;
        rec[0].blockNum.x = data->args.hipHccModuleLaunchKernel.globalWorkSizeX;
        rec[0].blockNum.y = data->args.hipHccModuleLaunchKernel.globalWorkSizeY;
        rec[0].blockNum.z = data->args.hipHccModuleLaunchKernel.globalWorkSizeZ;
        rec[0].blockDim.x = data->args.hipHccModuleLaunchKernel.blockDimX;
        rec[0].blockDim.y = data->args.hipHccModuleLaunchKernel.blockDimY;
        rec[0].blockDim.z = data->args.hipHccModuleLaunchKernel.blockDimZ;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_launch_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipLaunchCooperativeKernel: {
        record_activity_launch_t *rec = (record_activity_launch_t *)ALLOCATE(
            sizeof(record_activity_launch_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipLaunchCooperativeKernel;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].sharedMemBytes =
            data->args.hipLaunchCooperativeKernel.sharedMemBytes;
        rec[0].stream = data->args.hipLaunchCooperativeKernel.stream;
        rec[0].blockNum.x = data->args.hipLaunchCooperativeKernel.gridDim.x;
        rec[0].blockNum.y = data->args.hipLaunchCooperativeKernel.gridDim.y;
        rec[0].blockNum.z = data->args.hipLaunchCooperativeKernel.gridDim.z;
        rec[0].blockDim.x = data->args.hipLaunchCooperativeKernel.blockDimX.x;
        rec[0].blockDim.y = data->args.hipLaunchCooperativeKernel.blockDimX.y;
        rec[0].blockDim.z = data->args.hipLaunchCooperativeKernel.blockDimX.z;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_launch_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipLaunchKernel_internal: {
        record_activity_launch_t *rec = (record_activity_launch_t *)ALLOCATE(
            sizeof(record_activity_launch_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipLaunchKernel_internal;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].sharedMemBytes =
            data->args.hipLaunchKernel_internal.sharedMemBytes;
        rec[0].stream = data->args.hipLaunchKernel_internal.stream;
        rec[0].blockNum.x =
            data->args.hipLaunchKernel_internal.numBlocks.x;
        rec[0].blockNum.y =
            data->args.hipLaunchKernel_internal.numBlocks.y;
        rec[0].blockNum.z =
            data->args.hipLaunchKernel_internal.numBlocks.z;
        rec[0].blockDim.x =
            data->args.hipLaunchKernel_internal.dimBlocks.x;
        rec[0].blockDim.y =
            data->args.hipLaunchKernel_internal.dimBlocks.y;
        rec[0].blockDim.z =
            data->args.hipLaunchKernel_internal.dimBlocks.z;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_launch_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

 

      case HIP_API_ID_hipModuleLaunchKernel: {
        record_activity_launch_t *rec = (record_activity_launch_t *)ALLOCATE(
            sizeof(record_activity_launch_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipModuleLaunchKernel;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].sharedMemBytes = data->args.hipModuleLaunchKernel.sharedMemBytes;
        rec[0].stream = data->args.hipModuleLaunchKernel.stream;
        rec[0].blockNum.x = data->args.hipModuleLaunchKernel.gridDimX;
        rec[0].blockNum.y = data->args.hipModuleLaunchKernel.gridDimY;
        rec[0].blockNum.z = data->args.hipModuleLaunchKernel.gridDimZ;
        rec[0].blockDim.x = data->args.hipModuleLaunchKernel.blockDimX;
        rec[0].blockDim.y = data->args.hipModuleLaunchKernel.blockDimY;
        rec[0].blockDim.z = data->args.hipModuleLaunchKernel.blockDimZ;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_launch_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

     
      case HIP_API_ID_hipMemcpy: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpy;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpy.dst;
        rec[0].src = data->args.hipMemcpy.src;
        rec[0].sizeBytes = data->args.hipMemcpy.sizeBytes;
        rec[0].kind = data->args.hipMemcpy.kind;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipDrvMemcpy3D: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipDrvMemcpy3D;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;

        void *dst1 = data->args.hipDrvMemcpy3D.pCopy->dstHost;
        void *dst2 = data->args.hipDrvMemcpy3D.pCopy->dstArray->data;
        if (dst1 == NULL)
          rec[0].dst = dst2;
        else
          rec[0].dst = dst1;

        auto src1 = data->args.hipDrvMemcpy3D.pCopy->srcHost;
        auto src2 = data->args.hipDrvMemcpy3D.pCopy->srcArray->data;
        if (src1 == NULL)
          rec[0].src = src2;
        else
          rec[0].src = src1;

        size_t copy_bytes = data->args.hipDrvMemcpy3D.pCopy->WidthInBytes *
                            data->args.hipDrvMemcpy3D.pCopy->Height *
                            data->args.hipDrvMemcpy3D.pCopy->Depth;
        rec[0].sizeBytes = copy_bytes;

        int kind;
        if (dst1 == NULL && src1 == NULL)
          kind = hipMemcpyHostToHost;
        else if (dst1 == NULL && src1 != NULL)
          kind = hipMemcpyDeviceToHost;
        else if (dst1 != NULL && src1 != NULL)
          kind = hipMemcpyHostToDevice;
        else
          kind = hipMemcpyDeviceToDevice;
        rec[0].kind = kind;

        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipDrvMemcpy2DUnaligned: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipDrvMemcpy2DUnaligned;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;

        void *dst1 = data->args.hipDrvMemcpy2DUnaligned.pCopy->dstHost;
        void *dst2 = data->args.hipDrvMemcpy2DUnaligned.pCopy->dstArray->data;
        if (dst1 == NULL)
          rec[0].dst = dst2;
        else
          rec[0].dst = dst1;

        auto src1 = data->args.hipDrvMemcpy2DUnaligned.pCopy->srcHost;
        auto src2 = data->args.hipDrvMemcpy2DUnaligned.pCopy->srcArray->data;
        if (src1 == NULL)
          rec[0].src = src2;
        else
          rec[0].src = src1;

        size_t copy_bytes =
            data->args.hipDrvMemcpy2DUnaligned.pCopy->WidthInBytes *
            data->args.hipDrvMemcpy2DUnaligned.pCopy->Height;
        rec[0].sizeBytes = copy_bytes;

        int kind;
        if (dst1 == NULL && src1 == NULL)
          kind = hipMemcpyHostToHost;
        else if (dst1 == NULL && src1 != NULL)
          kind = hipMemcpyDeviceToHost;
        else if (dst1 != NULL && src1 != NULL)
          kind = hipMemcpyHostToDevice;
        else
          kind = hipMemcpyDeviceToDevice;
        rec[0].kind = kind;

        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpy2D: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpy2D;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpy2D.dst;
        rec[0].src = data->args.hipMemcpy2D.src;
        rec[0].sizeBytes = data->args.hipMemcpy2D.width * data->args.hipMemcpy2D.height;
        rec[0].kind = data->args.hipMemcpy2D.kind;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpy2DFromArray: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpy2DFromArray;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpy2DFromArray.dst;
        rec[0].src = data->args.hipMemcpy2DFromArray.src->data;
        rec[0].sizeBytes = data->args.hipMemcpy2DFromArray.width *
                           data->args.hipMemcpy2DFromArray.height;
        rec[0].kind = data->args.hipMemcpy2DFromArray.kind;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpy2DToArray: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpy2DToArray;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpy2DToArray.dst->data;
        rec[0].src = data->args.hipMemcpy2DToArray.src;
        rec[0].sizeBytes = data->args.hipMemcpy2DToArray.width *
                           data->args.hipMemcpy2DToArray.height;
        rec[0].kind = data->args.hipMemcpy2DToArray.kind;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpy3D: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpy3D;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpy3D.p->srcArray->data;
        rec[0].src = data->args.hipMemcpy3D.p->dstArray->data;
        // todo: sizeBytes may be wrong
        auto chan_desc = data->args.hipMemcpy3D.p->srcArray->desc;
        auto chan_size =
            (chan_desc.x + chan_desc.y + chan_desc.z + chan_desc.w + 7) / 8;
        auto array_element_size =
            get_array_element_size(data->args.hipMemcpy3D.p->srcArray->Format) *
            data->args.hipMemcpy3D.p->srcArray->NumChannels * chan_size;
        auto ext = data->args.hipMemcpy3D.p->extent;
        rec[0].sizeBytes = ext.width * ext.height * ext.depth * array_element_size;
        rec[0].kind = data->args.hipMemcpy3D.p->kind;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyAtoH: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyAtoH;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyAtoH.dst;
        rec[0].src = data->args.hipMemcpyAtoH.srcArray->data;
        rec[0].sizeBytes = data->args.hipMemcpyAtoH.count;
        rec[0].kind = hipMemcpyDeviceToHost;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyDtoD: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyDtoD;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyDtoD.dst;
        rec[0].src = data->args.hipMemcpyDtoD.src;
        rec[0].sizeBytes = data->args.hipMemcpyDtoD.sizeBytes;
        rec[0].kind = hipMemcpyDeviceToDevice;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyDtoH: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyDtoH;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyDtoH.dst;
        rec[0].src = data->args.hipMemcpyDtoH.src;
        rec[0].sizeBytes = data->args.hipMemcpyDtoH.sizeBytes;
        rec[0].kind = hipMemcpyDeviceToHost;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyFromArray: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyFromArray;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyFromArray.dst;
        rec[0].src = data->args.hipMemcpyFromArray.srcArray->data;
        rec[0].sizeBytes = data->args.hipMemcpyFromArray.count;
        rec[0].kind = data->args.hipMemcpyFromArray.kind;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyFromSymbol: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyFromSymbol;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyFromSymbol.dst;
        rec[0].src = data->args.hipMemcpyFromSymbol.symbol;
        rec[0].sizeBytes = data->args.hipMemcpyFromSymbol.sizeBytes;
        rec[0].kind = data->args.hipMemcpyFromSymbol.kind;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyHtoA: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyHtoA;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyHtoA.dstArray->data;
        rec[0].src = data->args.hipMemcpyHtoA.srcHost;
        rec[0].sizeBytes = data->args.hipMemcpyHtoA.count;
        rec[0].kind = hipMemcpyHostToDevice;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyHtoD: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyHtoD;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyHtoD.dst;
        rec[0].src = data->args.hipMemcpyHtoD.src;
        rec[0].sizeBytes = data->args.hipMemcpyHtoD.sizeBytes;
        rec[0].kind = hipMemcpyHostToDevice;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyParam2D: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyParam2D;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;

        void *dst1 = data->args.hipMemcpyParam2D.pCopy->dstHost;
        void *dst2 = data->args.hipMemcpyParam2D.pCopy->dstArray->data;
        if (dst1 == NULL)
          rec[0].dst = dst2;
        else
          rec[0].dst = dst1;

        auto src1 = data->args.hipMemcpyParam2D.pCopy->srcHost;
        auto src2 = data->args.hipMemcpyParam2D.pCopy->srcArray->data;
        if (src1 == NULL)
          rec[0].src = src2;
        else
          rec[0].src = src1;

        size_t copy_bytes = data->args.hipMemcpyParam2D.pCopy->WidthInBytes *
                            data->args.hipMemcpyParam2D.pCopy->Height;
        rec[0].sizeBytes = copy_bytes;

        int kind;
        if (dst1 == NULL && src1 == NULL)
          kind = hipMemcpyHostToHost;
        else if (dst1 == NULL && src1 != NULL)
          kind = hipMemcpyDeviceToHost;
        else if (dst1 != NULL && src1 != NULL)
          kind = hipMemcpyHostToDevice;
        else
          kind = hipMemcpyDeviceToDevice;
        rec[0].kind = kind;

        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyPeer: {
        // todo: src and dst Device ID ?
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyPeer;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyPeer.dst;
        rec[0].src = data->args.hipMemcpyPeer.src;
        rec[0].sizeBytes = data->args.hipMemcpyPeer.sizeBytes;
        rec[0].kind = hipMemcpyDeviceToDevice;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyToSymbol: {
        record_activity_memcpy_t *rec = (record_activity_memcpy_t *)ALLOCATE(
            sizeof(record_activity_memcpy_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyToSymbol;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyToSymbol.symbol;
        rec[0].src = data->args.hipMemcpyToSymbol.src;
        rec[0].sizeBytes = data->args.hipMemcpyToSymbol.sizeBytes;
        rec[0].kind = data->args.hipMemcpyToSymbol.kind;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyAsync: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyAsync.dst;
        rec[0].src = data->args.hipMemcpyAsync.src;
        rec[0].sizeBytes = data->args.hipMemcpyAsync.sizeBytes;
        rec[0].kind = data->args.hipMemcpyAsync.kind;
        rec[0].stream = data->args.hipMemcpyAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipDrvMemcpy3DAsync: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipDrvMemcpy3DAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;

        void *dst1 = data->args.hipDrvMemcpy3DAsync.pCopy->dstHost;
        void *dst2 = data->args.hipDrvMemcpy3DAsync.pCopy->dstArray->data;
        if (dst1 == NULL)
          rec[0].dst = dst2;
        else
          rec[0].dst = dst1;

        auto src1 = data->args.hipDrvMemcpy3DAsync.pCopy->srcHost;
        auto src2 = data->args.hipDrvMemcpy3DAsync.pCopy->srcArray->data;
        if (src1 == NULL)
          rec[0].src = src2;
        else
          rec[0].src = src1;

        size_t copy_bytes = data->args.hipDrvMemcpy3DAsync.pCopy->WidthInBytes *
                            data->args.hipDrvMemcpy3DAsync.pCopy->Height *
                            data->args.hipDrvMemcpy3DAsync.pCopy->Depth;
        rec[0].sizeBytes = copy_bytes;

        int kind;
        if (dst1 == NULL && src1 == NULL)
          kind = hipMemcpyHostToHost;
        else if (dst1 == NULL && src1 != NULL)
          kind = hipMemcpyDeviceToHost;
        else if (dst1 != NULL && src1 != NULL)
          kind = hipMemcpyHostToDevice;
        else
          kind = hipMemcpyDeviceToDevice;
        rec[0].kind = kind;
        rec[0].stream = data->args.hipDrvMemcpy3DAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpy2DAsync: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpy2DAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpy2DAsync.dst;
        rec[0].src = data->args.hipMemcpy2DAsync.src;
        rec[0].sizeBytes = data->args.hipMemcpy2DAsync.width * data->args.hipMemcpy2DAsync.height;
        rec[0].kind = data->args.hipMemcpy2DAsync.kind;
        rec[0].stream = data->args.hipMemcpy2DAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpy2DFromArrayAsync: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpy2DFromArrayAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpy2DFromArrayAsync.dst;
        rec[0].src = data->args.hipMemcpy2DFromArrayAsync.src->data;
        rec[0].sizeBytes = data->args.hipMemcpy2DFromArrayAsync.width *
                           data->args.hipMemcpy2DFromArrayAsync.height;
        rec[0].kind = data->args.hipMemcpy2DFromArrayAsync.kind;
        rec[0].stream = data->args.hipMemcpy2DFromArrayAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpy3DAsync: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpy3DAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpy3DAsync.p->srcArray->data;
        rec[0].src = data->args.hipMemcpy3DAsync.p->dstArray->data;
        auto chan_desc = data->args.hipMemcpy3D.p->srcArray->desc;
        auto chan_size =
            (chan_desc.x + chan_desc.y + chan_desc.z + chan_desc.w + 7) / 8;
        auto array_element_size =
            get_array_element_size(data->args.hipMemcpy3D.p->srcArray->Format) *
            data->args.hipMemcpy3D.p->srcArray->NumChannels * chan_size;
        auto ext = data->args.hipMemcpy3D.p->extent;
        rec[0].sizeBytes =
            ext.width * ext.height * ext.depth * array_element_size;
        rec[0].kind = data->args.hipMemcpy3DAsync.p->kind;
        rec[0].stream = data->args.hipMemcpy3DAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyDtoDAsync: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyDtoDAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyDtoDAsync.dst;
        rec[0].src = data->args.hipMemcpyDtoDAsync.src;
        rec[0].sizeBytes = data->args.hipMemcpyDtoDAsync.sizeBytes;
        rec[0].kind = hipMemcpyDeviceToDevice;
        rec[0].stream = data->args.hipMemcpyDtoDAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyDtoHAsync: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyDtoHAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyDtoHAsync.dst;
        rec[0].src = data->args.hipMemcpyDtoHAsync.src;
        rec[0].sizeBytes = data->args.hipMemcpyDtoHAsync.sizeBytes;
        rec[0].kind = hipMemcpyDeviceToHost;
        rec[0].stream = data->args.hipMemcpyDtoHAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyFromSymbolAsync: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyFromSymbolAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyFromSymbolAsync.dst;
        rec[0].src = data->args.hipMemcpyFromSymbolAsync.symbol;
        rec[0].sizeBytes = data->args.hipMemcpyFromSymbolAsync.sizeBytes;
        rec[0].kind = data->args.hipMemcpyFromSymbolAsync.kind;
        rec[0].stream = data->args.hipMemcpyFromSymbolAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyHtoDAsync: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyHtoDAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyHtoDAsync.dst;
        rec[0].src = data->args.hipMemcpyHtoDAsync.src;
        rec[0].sizeBytes = data->args.hipMemcpyHtoDAsync.sizeBytes;
        rec[0].kind = hipMemcpyHostToDevice;
        rec[0].stream = data->args.hipMemcpyHtoDAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyParam2DAsync: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyParam2DAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;

        auto dst1 = data->args.hipMemcpyParam2DAsync.pCopy->dstHost;
        auto dst2 = data->args.hipMemcpyParam2DAsync.pCopy->dstArray->data;
        if (dst1 == NULL)
          rec[0].dst = dst2;
        else
          rec[0].dst = dst1;

        auto src1 = data->args.hipMemcpyParam2DAsync.pCopy->srcHost;
        auto src2 = data->args.hipMemcpyParam2DAsync.pCopy->srcArray->data;
        if (src1 == NULL)
          rec[0].src = src2;
        else
          rec[0].src = src1;

        size_t copy_bytes =
            data->args.hipMemcpyParam2DAsync.pCopy->WidthInBytes *
            data->args.hipMemcpyParam2DAsync.pCopy->Height;
        rec[0].sizeBytes = copy_bytes;

        int kind;
        if (dst1 == NULL && src1 == NULL)
          kind = hipMemcpyHostToHost;
        else if (dst1 == NULL && src1 != NULL)
          kind = hipMemcpyDeviceToHost;
        else if (dst1 != NULL && src1 != NULL)
          kind = hipMemcpyHostToDevice;
        else
          kind = hipMemcpyDeviceToDevice;
        rec[0].kind = kind;
        rec[0].stream = data->args.hipMemcpyParam2DAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyPeerAsync: {
        // todo: src and dst Device ID ?
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyPeerAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyPeerAsync.dst;
        rec[0].src = data->args.hipMemcpyPeerAsync.src;
        rec[0].sizeBytes = data->args.hipMemcpyPeerAsync.sizeBytes;
        rec[0].kind = hipMemcpyDeviceToDevice;
        rec[0].stream = data->args.hipMemcpyPeerAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyToSymbolAsync: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyToSymbolAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyToSymbolAsync.symbol;
        rec[0].src = data->args.hipMemcpyToSymbolAsync.src;
        rec[0].sizeBytes = data->args.hipMemcpyToSymbolAsync.sizeBytes;
        rec[0].kind = data->args.hipMemcpyToSymbolAsync.kind;
        rec[0].stream = data->args.hipMemcpyToSymbolAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemcpyWithStream: {
        record_activity_memcpy_async_t *rec =
            (record_activity_memcpy_async_t *)ALLOCATE(
                sizeof(record_activity_memcpy_async_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemcpyWithStream;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].dst = data->args.hipMemcpyWithStream.dst;
        rec[0].src = data->args.hipMemcpyWithStream.src;
        rec[0].sizeBytes = data->args.hipMemcpyWithStream.sizeBytes;
        rec[0].kind = data->args.hipMemcpyWithStream.kind;
        rec[0].stream = data->args.hipMemcpyWithStream.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_memcpy_async_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipArray3DCreate: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipArray3DCreate;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipArray3DCreate.array__val->data;
        auto dsc = data->args.hipArray3DCreate.pAllocateArray;
        rec[0].sizeBytes = dsc->Width * dsc->Height * dsc->Depth *
                           dsc->NumChannels *
                           get_array_element_size(dsc->Format);
        rec[0].kind = HIP_ARRAY;
        rec[0].cpu_id = -1;
        rec[0].stream = 0;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipArrayCreate: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipArrayCreate;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipArrayCreate.pHandle__val->data;
        auto dsc = data->args.hipArrayCreate.pAllocateArray;
        rec[0].sizeBytes = dsc->Width * dsc->Height * dsc->NumChannels *
                           get_array_element_size(dsc->Format);
        rec[0].kind = HIP_ARRAY;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipExtHostMalloc: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipExtHostMalloc;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipExtHostMalloc.ptr__val;
        rec[0].sizeBytes = data->args.hipExtHostMalloc.sizeBytes;
        rec[0].kind = HIP_HOST;
        rec[0].stream = 0;
        rec[0].cpu_id = data->args.hipExtHostMalloc.cpuId;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipExtMallocWithFlags: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipExtMallocWithFlags;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipExtMallocWithFlags.ptr__val;
        rec[0].sizeBytes = data->args.hipExtMallocWithFlags.sizeBytes;
        rec[0].kind = HIP_DEVICE;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipHostAlloc: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipHostAlloc;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipHostAlloc.ptr__val;
        rec[0].sizeBytes = data->args.hipHostAlloc.size;
        rec[0].kind = HIP_HOST;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipHostMalloc: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipHostMalloc;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipHostMalloc.ptr__val;
        rec[0].sizeBytes = data->args.hipHostMalloc.size;
        rec[0].kind = HIP_HOST;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMalloc: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMalloc;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMalloc.ptr__val;
        rec[0].sizeBytes = data->args.hipMalloc.size;
        rec[0].kind = HIP_DEVICE;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMalloc3D: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMalloc3D;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMalloc3D.pitchedDevPtr->ptr;
        auto ext = data->args.hipMalloc3D.extent;
        rec[0].sizeBytes = ext.width * ext.height * ext.depth;
        rec[0].kind = HIP_DEVICE;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMalloc3DArray: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        // todo: sizeBytes may be wrong.
        rec[0].record.MsgType = event_hipMalloc3DArray;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMalloc3DArray.array__val->data;
        auto ext = data->args.hipMalloc3DArray.extent;
        auto channel_dsc = data->args.hipMalloc3DArray.desc__val;
        rec[0].sizeBytes =
            ext.width * ext.height * ext.depth *
            (channel_dsc.x + channel_dsc.y + channel_dsc.z + channel_dsc.w) / 8;
        rec[0].kind = HIP_ARRAY;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMallocArray: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        // todo: sizeBytes may be wrong.
        rec[0].record.MsgType = event_hipMallocArray;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMallocArray.array__val->data;
        auto channel_dsc = data->args.hipMallocArray.desc__val;
        rec[0].sizeBytes = data->args.hipMallocArray.width *
                           data->args.hipMallocArray.height *
                           (channel_dsc.x + channel_dsc.y + channel_dsc.z +
                            channel_dsc.w + 7) /
                           8;
        rec[0].kind = HIP_ARRAY;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMallocAsync: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMallocAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMallocAsync.dev_ptr__val;
        rec[0].sizeBytes = data->args.hipMallocAsync.size;
        rec[0].kind = HIP_DEVICE;
        rec[0].stream = data->args.hipMallocAsync.stream;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMallocFromPoolAsync: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMallocFromPoolAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMallocFromPoolAsync.dev_ptr__val;
        rec[0].sizeBytes = data->args.hipMallocFromPoolAsync.size;
        rec[0].kind = HIP_DEVICE;
        rec[0].stream = data->args.hipMallocFromPoolAsync.stream;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMallocHost: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMallocHost;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMallocHost.ptr__val;
        rec[0].sizeBytes = data->args.hipMallocHost.size;
        rec[0].kind = HIP_HOST;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMallocManaged: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMallocManaged;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMallocManaged.dev_ptr__val;
        rec[0].sizeBytes = data->args.hipMallocManaged.size;
        rec[0].kind = HIP_DEVICE;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMallocMipmappedArray: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        // todo: sizeBytes may be wrong.
        rec[0].record.MsgType = event_hipMallocMipmappedArray;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr =
            data->args.hipMallocMipmappedArray.mipmappedArray__val->data;
        auto array_type_size = get_array_element_size(
            data->args.hipMallocMipmappedArray.mipmappedArray__val->format);
        auto ext = data->args.hipMallocMipmappedArray.extent;
        auto channel_dsc = data->args.hipMallocMipmappedArray.desc__val;
        rec[0].sizeBytes =
            ext.width * ext.height * ext.depth *
            (channel_dsc.x + channel_dsc.y + channel_dsc.z + channel_dsc.w) / 8;
        rec[0].kind = HIP_ARRAY;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMallocPitch: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMallocPitch;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMallocPitch.ptr__val;
        rec[0].sizeBytes =
            data->args.hipMallocPitch.width * data->args.hipMallocPitch.height;
        rec[0].kind = HIP_DEVICE;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemAllocHost: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemAllocHost;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMemAllocHost.ptr__val;
        rec[0].sizeBytes = data->args.hipMemAllocHost.size;
        rec[0].kind = HIP_HOST;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemAllocPitch: {
        record_activity_mem_alloc_t *rec =
            (record_activity_mem_alloc_t *)ALLOCATE(
                sizeof(record_activity_mem_alloc_t) +
                sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemAllocPitch;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMemAllocPitch.dptr__val;
        rec[0].sizeBytes = data->args.hipMemAllocPitch.widthInBytes *
                           data->args.hipMemAllocPitch.height;
        rec[0].kind = HIP_DEVICE;
        rec[0].stream = 0;
        rec[0].cpu_id = -1;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_mem_alloc_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipArrayDestroy: {
        record_activity_free_t *rec = (record_activity_free_t *)ALLOCATE(
            sizeof(record_activity_free_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipArrayDestroy;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipArrayDestroy.array->data;
        auto chan_desc = data->args.hipArrayDestroy.array->desc;
        auto chan_size =
            (chan_desc.x + chan_desc.y + chan_desc.z + chan_desc.w + 7) / 8;
        auto array_element_size =
            get_array_element_size(data->args.hipArrayDestroy.array->Format) *
            data->args.hipArrayDestroy.array->NumChannels * chan_size;
        rec[0].sizeBytes = data->args.hipArrayDestroy.array->width *
                           data->args.hipArrayDestroy.array->height *
                           data->args.hipArrayDestroy.array->depth *
                           array_element_size;
        rec[0].kind = HIP_ARRAY;
        rec[0].stream = 0;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_free_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMipmappedArrayDestroy: {
        record_activity_free_t *rec = (record_activity_free_t *)ALLOCATE(
            sizeof(record_activity_free_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMipmappedArrayDestroy;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMipmappedArrayDestroy.hMipmappedArray->data;
        auto chan_desc =
            data->args.hipMipmappedArrayDestroy.hMipmappedArray->desc;
        auto chan_size =
            (chan_desc.x + chan_desc.y + chan_desc.z + chan_desc.w + 7) / 8;
        #ifdef DTK_24
        auto array_element_size =
            get_array_element_size(
                data->args.hipMipmappedArrayDestroy.hMipmappedArray->format) *
            data->args.hipMipmappedArrayDestroy.hMipmappedArray->num_channels *
            chan_size;
        #else
        auto array_element_size =
            get_array_element_size(
                data->args.hipMipmappedArrayDestroy.hMipmappedArray->format) * chan_size;
        #endif
        rec[0].sizeBytes =
            data->args.hipMipmappedArrayDestroy.hMipmappedArray->width *
            data->args.hipMipmappedArrayDestroy.hMipmappedArray->height *
            data->args.hipMipmappedArrayDestroy.hMipmappedArray->depth *
            array_element_size;
        rec[0].kind = HIP_ARRAY;
        rec[0].stream = 0;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_free_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipFree: {
        record_activity_free_t *rec = (record_activity_free_t *)ALLOCATE(
            sizeof(record_activity_free_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipFree;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipFree.ptr;
        rec[0].sizeBytes = data->args.hipFree._sizeBytes;
        rec[0].kind = HIP_DEVICE;
        rec[0].stream = 0;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_free_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipFreeArray: {
        record_activity_free_t *rec = (record_activity_free_t *)ALLOCATE(
            sizeof(record_activity_free_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipFreeArray;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipFreeArray.array->data;
        auto chan_desc = data->args.hipFreeArray.array->desc;
        auto chan_size =
            (chan_desc.x + chan_desc.y + chan_desc.z + chan_desc.w + 7) / 8;
        auto array_element_size =
            get_array_element_size(data->args.hipFreeArray.array->Format) *
            data->args.hipFreeArray.array->NumChannels * chan_size;
        rec[0].sizeBytes = data->args.hipFreeArray.array->width *
                           data->args.hipFreeArray.array->height *
                           data->args.hipFreeArray.array->depth * array_element_size;
        rec[0].kind = HIP_ARRAY;
        rec[0].stream = 0;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_free_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipFreeAsync: {
        record_activity_free_t *rec = (record_activity_free_t *)ALLOCATE(
            sizeof(record_activity_free_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipFreeAsync;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipFreeAsync.dev_ptr;
        rec[0].sizeBytes = 0;
        rec[0].kind = HIP_DEVICE;
        rec[0].stream = data->args.hipFreeAsync.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_free_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipFreeHost: {
        record_activity_free_t *rec = (record_activity_free_t *)ALLOCATE(
            sizeof(record_activity_free_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipFreeHost;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipFreeHost.ptr;
        rec[0].sizeBytes = 0;
        rec[0].kind = HIP_HOST;
        rec[0].stream = 0;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_free_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipFreeMipmappedArray: {
        record_activity_free_t *rec = (record_activity_free_t *)ALLOCATE(
            sizeof(record_activity_free_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipFreeMipmappedArray;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipFreeMipmappedArray.mipmappedArray->data;
        auto chan_desc = data->args.hipFreeMipmappedArray.mipmappedArray->desc;
        auto chan_size =
            (chan_desc.x + chan_desc.y + chan_desc.z + chan_desc.w + 7) / 8;
#ifdef DTK_24
        auto array_element_size =
            get_array_element_size(
                data->args.hipFreeMipmappedArray.mipmappedArray->format) *
            data->args.hipFreeMipmappedArray.mipmappedArray->num_channels *
            chan_size;
#else
        auto array_element_size =
            get_array_element_size(
                data->args.hipFreeMipmappedArray.mipmappedArray->format) * chan_size;
#endif
        rec[0].sizeBytes =
            data->args.hipFreeMipmappedArray.mipmappedArray->width *
            data->args.hipFreeMipmappedArray.mipmappedArray->height *
            data->args.hipFreeMipmappedArray.mipmappedArray->depth *
            array_element_size;
        rec[0].kind = HIP_ARRAY;
        rec[0].stream = 0;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_free_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipHostFree: {
        record_activity_free_t *rec = (record_activity_free_t *)ALLOCATE(
            sizeof(record_activity_free_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipHostFree;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipHostFree.ptr;
        rec[0].sizeBytes = data->args.hipHostFree._sizeBytes;
        rec[0].kind = HIP_HOST;
        rec[0].stream = 0;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_free_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipMemAddressFree: {
        record_activity_free_t *rec = (record_activity_free_t *)ALLOCATE(
            sizeof(record_activity_free_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipMemAddressFree;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].ptr = data->args.hipMemAddressFree.devPtr;
        rec[0].sizeBytes = data->args.hipMemAddressFree.size;
        rec[0].kind = HIP_DEVICE;
        rec[0].stream = 0;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_free_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipEventSynchronize: {
        record_activity_wait_t *rec = (record_activity_wait_t *)ALLOCATE(
            sizeof(record_activity_wait_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipEventSynchronize;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].event = data->args.hipEventSynchronize.event;
        rec[0].stream = nullptr;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_wait_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipStreamSynchronize: {
        record_activity_wait_t *rec = (record_activity_wait_t *)ALLOCATE(
            sizeof(record_activity_wait_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipStreamSynchronize;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].event = nullptr;
        rec[0].stream = data->args.hipStreamSynchronize.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_wait_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipStreamWaitEvent: {
        record_activity_wait_t *rec = (record_activity_wait_t *)ALLOCATE(
            sizeof(record_activity_wait_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipStreamWaitEvent;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].event = data->args.hipStreamWaitEvent.event;
        rec[0].event = data->args.hipStreamWaitEvent.stream;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_wait_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipEventCreate: {
        record_activity_event_t *rec = (record_activity_event_t *)ALLOCATE(
            sizeof(record_activity_event_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipEventCreate;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].stream = nullptr;
        rec[0].event = data->args.hipEventCreate.event__val;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_event_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipEventCreateWithFlags: {
        record_activity_event_t *rec = (record_activity_event_t *)ALLOCATE(
            sizeof(record_activity_event_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipEventCreateWithFlags;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].stream = nullptr;
        rec[0].event = data->args.hipEventCreateWithFlags.event__val;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_event_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipEventDestroy: {
        record_activity_event_t *rec = (record_activity_event_t *)ALLOCATE(
            sizeof(record_activity_event_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipEventDestroy;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].stream = nullptr;
        rec[0].event = data->args.hipEventDestroy.event;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_event_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      case HIP_API_ID_hipEventRecord: {
        record_activity_event_t *rec = (record_activity_event_t *)ALLOCATE(
            sizeof(record_activity_event_t) +
            sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = event_hipEventRecord;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].stream = data->args.hipEventRecord.stream;
        rec[0].event = data->args.hipEventRecord.event;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_event_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
        break;
      }

      default: {
        record_activity_t *rec = (record_activity_t *)ALLOCATE(
            sizeof(record_activity_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
        if (_enable_pmu) {
          uint64_t *counters = reinterpret_cast<uint64_t *>(rec + 1);
          memcpy(counters, ts_buff_ring[ri].pmu,
                 jsi_pmu_num * sizeof(uint64_t));
          pmu_collector_get_all(counters + jsi_pmu_num);
        }
#endif
        rec[0].record.MsgType = cid + event_hipDeviceEnablePeerAccess - HIP_API_ID_hipDeviceEnablePeerAccess;
        rec[0].record.ctxt = ctxt;
        rec[0].correlation_id = data->correlation_id;
        rec[0].record.timestamps.enter = enter_ts;
        rec[0].record.timestamps.exit = ts;

        helperTraceStore((void *)(rec));

        DEALLOCATE(rec, sizeof(record_activity_t) +
                            sizeof(uint64_t) * 2 * jsi_pmu_num);
      }
    }
  }
  jsi_safe_exit_instr();
}

// __attribute__((constructor (ROCM_TRACER_INIT_PRIORITY)))
__attribute__((constructor)) void jsi_accl_tracer_init() {
  JSI_LOG(JSILOG_INFO, "Initialize JSI ACCL Tracer <ROCM> Library (PID=%d).\n",
          GetPid());
  CHECK(roctracer_set_properties(ACTIVITY_DOMAIN_HIP_API, NULL));
  CHECK(roctracer_enable_domain_callback(ACTIVITY_DOMAIN_HIP_API,
                                         hip_api_callback, nullptr));
  activity_init();
  
  JSI_LOG(JSILOG_INFO, "ROCM TRACER INITIALIZING!\n");

  // RecordWriter::metaSectionStart("ACCL TRACE META");
  // RecordWriter::metaStore<std::string>("ACCL_DEVICE_TYPE", "HYGON");
  // RecordWriter::metaSectionEnd("ACCL TRACE META");
  rocmMetaSet();

  _enable_backtrace = jsi_backtrace_enabled;
  _enable_pmu = jsi_pmu_enabled;
}

// __attribute__((destructor (ROCM_TRACER_FINI_PRIORITY)))
__attribute__((destructor)) void jsi_accl_tracer_finalize() {
  JSI_LOG(JSILOG_INFO, "Finalize JSI ACCL Tracer <ROCM> Library (PID=%d).\n",
          GetPid());
  if (_activity_initialized) {
    roctracer_flush_activity_expl(pool);
    //   writer->flushCache();
    //   delete writer;
  }
}