#include "instrument/pmu_collector.h"

#include <papi.h>

#include <cstdlib>
#include <cstring>
#include <vector>

#include "pthread.h"
#include "record/record_writer.h"
#include "utils/jsi_log.h"

namespace {
bool _pmu_initialized = false;
int _eventset = PAPI_NULL;
std::vector<int> _event_codes;
pmu_val_type *_values;
}// namespace

// Load PMU collection from environment variable: JSI_COLLECT_PMU_EVENT
// Currently, only support collection of 1 event, return false when failed
// The initialization need to do:
// 1) read the configured pmu event and registered it via PAPI
// 2) clear the pmu counter to make it count from 0
// Here we assume we are in 64-bit platforms, and the pmu counter is also 64-bit,
// thus the pmu counter can be considered as never overflowed by the 64-bit counter
bool pmu_collector_init() {
    if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
        JSI_WARN("PAPI_library_init failed.\n");
        return false;
    }
    if (PAPI_thread_init(pthread_self) != PAPI_OK) {
        JSI_WARN("PAPI_thread_init failed.\n");
        return false;
    }

    _eventset = PAPI_NULL;

    if (PAPI_create_eventset(&_eventset) != PAPI_OK) {
        JSI_WARN("Could not create PAPI event_name _eventset.\n");
        return false;
    }

    const char *env_event_name_list = getenv("JSI_COLLECT_PMU_EVENT");
    if (env_event_name_list == nullptr) {
        JSI_WARN("PMU event not specified, please set environment variable `JSI_COLLECT_PMU_EVENT`.\n");
        return false;
    }

    std::vector<std::string> _event_list;
    parse_pmu_event_list(env_event_name_list, &_event_list);
    for (const auto &event_name: _event_list) {
        int event_code;
        int r = PAPI_event_name_to_code(event_name.c_str(), &event_code);
        if (r != PAPI_OK) {
            JSI_WARN("Could not get event code of PAPI event named %s, error code == %d\n",
                     event_name.c_str(), r);
            return false;
        }

        JSI_INFO("event code == %x\n", event_code);

        r = PAPI_add_event(_eventset, event_code);
        if (r != PAPI_OK) {
            JSI_WARN("PAPI_add_event failed with error code: %d\n", r);
            return false;
        }

        _event_codes.push_back(event_code);
    }
    _event_list.clear();

    if (_event_codes.size() == 0) {
        JSI_WARN("PMU event list is empty, please properly set environment variable "
                 "`JSI_COLLECT_PMU_EVENT` to a comma-separated event list (e.g. `PAPI_TOT_INS,PAPI_BR_INS`).\n");
        return false;
    }

    if (_event_codes.size() > 4) {
        JSI_WARN("Could not collect more than 4 events (trying to collect %zu)\n",
                 _event_codes.size());
        return false;
    }
    JSI_INFO("[PMU Collector]%zu event(s) to be collected.\n", _event_codes.size());

    RecordWriter::metaSectionStart("PMU COLLECTOR");
    RecordWriter::metaStore<int>("PMU_NUM_EVENTS", (int) _event_codes.size());
    RecordWriter::metaStore("PMU_EVENT_LIST", env_event_name_list);
    RecordWriter::metaSectionEnd("PMU COLLECTOR");

    _values = reinterpret_cast<pmu_val_type *>(malloc(sizeof(pmu_val_type) * _event_codes.size()));

    PAPI_start(_eventset);
    PAPI_reset(_eventset);

    _pmu_initialized = true;
    return true;
}

// finalize and free the workspace, return false when failed
bool pmu_collector_fini() {
    int r = PAPI_stop(_eventset, _values);
    if (r != PAPI_OK) {
        JSI_WARN("PAPI_stop failed with error code: %d\n", r);
        return false;
    }

    r = PAPI_cleanup_eventset(_eventset);
    if (r != PAPI_OK) {
        JSI_WARN("PAPI_cleanup_eventset failed with error code: %d (%s:%d)\n", r, __FILE__,
                 __LINE__);
        return false;
    }

    r = PAPI_destroy_eventset(&_eventset);
    if (r != PAPI_OK) {
        JSI_WARN("PAPI_destroy_eventset failed with error code: %d (%s:%d)\n", r, __FILE__,
                 __LINE__);
        return false;
    }

    PAPI_shutdown();

    _event_codes.clear();
    free(_values);
    _eventset = PAPI_NULL;
    _pmu_initialized = false;
    return true;
}

// read the counter value of the i-th registered event
uint64_t pmu_collector_get(int i) {
    if (!_pmu_initialized) {
        JSI_ERROR("PMU collector not initialized. Aborting...\n");
    }

    if (i >= _event_codes.size()) {
        JSI_ERROR("Not registered event %d. Aborting...\n", i);
    }

    int r = PAPI_read(_eventset, _values);
    if (r != PAPI_OK) {
        JSI_WARN("PAPI_read failed with error code: %d (%s:%d)\n", r, __FILE__, __LINE__);
    }

    return _values[i];
}

// read all counter values of the registered events
// return the number of read events.
int pmu_collector_get_all(uint64_t *ptr) {
    if (!_pmu_initialized) {
        JSI_ERROR("PMU collector not initialized. Aborting...\n");
    }

    int r = PAPI_read(_eventset, reinterpret_cast<long long *>(ptr));
    if (r != PAPI_OK) {
        JSI_WARN("PAPI_read failed with error code: %d (%s:%d)\n", r, __FILE__, __LINE__);
    }

    return (int) _event_codes.size();
}

int pmu_collector_get_num_events() {
    if (!_pmu_initialized) {
        return 0;
    }
    return (int) _event_codes.size();
}
