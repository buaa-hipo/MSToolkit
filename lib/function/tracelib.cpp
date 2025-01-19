#include <cstring>
#include <ctime>
#include <iostream>
#include <stdio.h>
#include <cassert>
#include <thread>
#include <vector>

#include "instrument/backtrace.h"
#include "utils/tsc_timer.h"
#include "utils/jsi_log.h"
#include "record/record_type.h"
#include "record/record_writer.h"
#ifdef ENABLE_PMU
#include "record/record_reader.h"
#include "instrument/pmu_collector.h"
#endif

record_t* rec;

#ifdef AARCH64
thread_local std::vector<record_t*> rec_list;

thread_local uint64_t depth;
thread_local std::vector<uint64_t> saved_lr;
#endif

bool jsi_pmu_enabled = false;
bool jsi_backtrace_enabled = false;

__attribute__((constructor)) void trace_init(void)
{
#ifdef ENABLE_PMU
    const char* pmu_enabled = getenv("JSI_ENABLE_PMU");
    if (pmu_enabled) {
        jsi_pmu_enabled = true;
    }
#endif
#ifdef ENABLE_BACKTRACE
    const char* bt_enabled = getenv("JSI_ENABLE_BACKTRACE");
    if (bt_enabled) {
        jsi_backtrace_enabled = true;
    }
#endif
}

#ifdef AARCH64
extern "C" void trace_entry_func(uint64_t lr) {
  saved_lr.push_back(lr);

  rec_list.push_back((record_t*)RecordWriter::allocate(sizeof(record_t)));
  rec = rec_list[depth];
  rec[0].MsgType = JSI_DEFAULT_FUNC_MSGTYPE;
  rec[0].timestamps.enter = get_tsc_raw();
#ifdef ENABLE_PMU
  if(jsi_pmu_enabled) {
      uint64_t *counters = RecordWriterHelper::counters(rec);
      pmu_collector_get_all(counters);
  }
#endif
#ifdef ENABLE_BACKTRACE
  if(jsi_backtrace_enabled)
      rec[0].ctxt = backtrace_context_get();
#endif
  depth++;
}

extern "C" uint64_t trace_exit_func() {
  depth--;
  uint64_t lr = saved_lr[depth];
  saved_lr.pop_back();

  rec = rec_list[depth];
  rec[0].timestamps.exit = get_tsc_raw();
#ifdef ENABLE_PMU
  if(jsi_pmu_enabled) {
      uint64_t* counters = RecordWriterHelper::counters(rec);
      pmu_collector_get_all(counters + pmu_collector_get_num_events());
  }
#endif
  rec_list.pop_back();
  return lr;
}
#else

extern "C" void trace_entry_func() {
  rec = (record_t*)RecordWriter::allocate(
            sizeof(record_t) + sizeof(uint64_t) * 2 * pmu_collector_get_num_events());
  rec[0].MsgType = JSI_DEFAULT_FUNC_MSGTYPE;
  rec[0].timestamps.enter = get_tsc_raw();
#ifdef ENABLE_PMU
  if(jsi_pmu_enabled) {
      uint64_t *counters = RecordWriterHelper::counters(rec);
      pmu_collector_get_all(counters);
  }
#endif
#ifdef ENABLE_BACKTRACE
  if(jsi_backtrace_enabled)
      rec[0].ctxt = backtrace_context_get();
#endif
}

extern "C" void trace_exit_func() {
  rec[0].timestamps.exit = get_tsc_raw();
#ifdef ENABLE_PMU
    if(jsi_pmu_enabled) {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters + pmu_collector_get_num_events());
    }
#endif
}

#endif // AARCH64

