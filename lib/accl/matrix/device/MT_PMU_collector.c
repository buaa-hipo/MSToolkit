#include "instrument/MT_PMU_collector.h"

#include "hthread_device.h"

/**
 *  ALL functions are implemented for single core
 *  GLOBAL variables have 24 copies each
 *  prof_end() would not reset the PMC
 *  hthread_group_destroy() would call prof_end() (presume)
 *  hthread_dat_unload() would not
 */

#define PRINT(...)

ESInfo EventSets[24][MAX_EVENTSET_NUM];
int    ESvalid[24][MAX_EVENTSET_NUM] = {0};
int    ESIndex[24]                   = {0};
int    default_es_index[24];

// char** EventList;

// void PMU_CHECK(pmu_api_status_t status, const char *func_name) {
//     if (status != PMU_OK) {
//         PRINT("ERROR code %d in %s\n", status, func_name);
//         dsp_abort(status);
//     }
// }

void dump_null() {
    dsp_abort(100);
}

pmu_api_status_t PMU_library_init() {
    int core_id = get_core_id();
    for (int i = 0; i < MAX_EVENTSET_NUM; ++i) {
        ESvalid[core_id][i]            = 0;
        EventSets[core_id][i].running  = 0;
        EventSets[core_id][i].eventNum = 0;
    }
    default_es_index[core_id] = PMU_NULL;
    return PMU_OK;
}

pmu_api_status_t PMU_add_event(int index, int event) {
    pmu_api_status_t err = PMU_add_events(index, &event, 1);
    if (err) dsp_abort(err);
    return PMU_OK;
}

pmu_api_status_t PMU_add_events(int index, int *events, int number) {
    int core_id = get_core_id();
    if (index < 0 || ESvalid[core_id][index] == 0) dsp_abort(50);
    if (EventSets[core_id][index].running) dsp_abort(50);
    if (EventSets[core_id][index].eventNum + number > MAX_PMU_NUM) dsp_abort(50);
    for (int i = 0; i < number; ++i) {
        if (events[i] < 0 || events[i] >= 26) dsp_abort(50);  // todo: retain valid event code?
        int pos = EventSets[core_id][index].eventNum;

        EventSets[core_id][index].events[pos] = events[i];
        EventSets[core_id][index].eventNum++;
    }
    return PMU_OK;
}

pmu_api_status_t PMU_add_named_event(int index, const char *event_name) {
    int              event_code;
    pmu_api_status_t err = PMU_event_name_to_code(event_name, &event_code);
    if (err) return err;
    return PMU_add_event(index, event_code);
}

pmu_api_status_t PMU_create_eventset(int *index) {
    if (index == NULL) {
        dump_null();
    }
    int core_id = get_core_id();
    if (*index != PMU_NULL) dsp_abort(50);  // todo: is this right ?
    for (int i = 0; i < MAX_EVENTSET_NUM; ++i) {
        if (ESvalid[core_id][i] == 0) {
            *index                        = i;
            ESvalid[core_id][i]           = 1;
            EventSets[core_id][i].running = 0;
            return PMU_OK;
        }
    }
    PRINT("Max number of event sets reached\n");
    dsp_abort(50);
    return PMU_ESYS;
}

pmu_api_status_t PMU_destroy_eventset(int *index) {
    PRINT("In PMU_destroy_eventset()\n");
    int core_id = get_core_id();
    if (*index < 0 || *index >= MAX_EVENTSET_NUM) dsp_abort(50);
    ESvalid[core_id][*index]            = 0;
    EventSets[core_id][*index].eventNum = 0;
    EventSets[core_id][*index].running  = 0;
    return PMU_OK;
}

pmu_api_status_t PMU_cleanup_eventset(int index) {
    int core_id = get_core_id();
    PRINT("[core-%d] In PMU_cleanup_eventset()\n", core_id);
    if (index < 0 || index >= MAX_EVENTSET_NUM) dsp_abort(50);
    if (ESvalid[core_id][index] == 0) dsp_abort(50);
    EventSets[core_id][index].eventNum = 0;
    return PMU_OK;
}

pmu_api_status_t PMU_start(int index) {
    PRINT("[core-%d] In PMU_start()\n", get_core_id());
    int core_id = get_core_id();
    if (index < 0 || index >= MAX_EVENTSET_NUM) dsp_abort(50);
    if (ESvalid[core_id][index] == 0) dsp_abort(50);
    if (EventSets[core_id][index].running) dsp_abort(50);
    EventSets[core_id][index].running = 1;
    for (int i = 0; i < EventSets[core_id][index].eventNum; ++i) {
        prof_start(EventSets[core_id][index].events[i]);
    }
    return PMU_OK;
}

pmu_api_status_t PMU_stop(int index, unsigned long *values) {
    PRINT("[core-%d] In PMU_stop()\n", get_core_id());
    int core_id = get_core_id();
    if (index < 0 || index >= MAX_EVENTSET_NUM) dsp_abort(50);
    if (ESvalid[core_id][index] == 0) return PMU_OK;
    if (EventSets[core_id][index].running == 0) return PMU_OK;
    for (int i = 0; i < EventSets[core_id][index].eventNum; ++i) {
        values[i] = prof_end(EventSets[core_id][index].events[i]);
    }
    EventSets[core_id][index].running = 0;
    return PMU_OK;
}

pmu_api_status_t PMU_read(int index, unsigned long *values) {
    PRINT("[core-%d] In PMU_read()\n", get_core_id());
    int core_id = get_core_id();
    if (index < 0 || index >= MAX_EVENTSET_NUM) dsp_abort(50);
    if (ESvalid[core_id][index] == 0) dsp_abort(50);
    if (EventSets[core_id][index].running == 0) dsp_abort(50);
    for (int i = 0; i < EventSets[core_id][index].eventNum; ++i) {
        unsigned long res = prof_read(EventSets[core_id][index].events[i]);
        PRINT("[core-%d] Reading event %d: %lu\n", get_core_id(), EventSets[core_id][index].events[i], res);
        values[i] = res;
    }
    return PMU_OK;
}

pmu_api_status_t PMU_reset(int index) {
    PRINT("In PMU_reset()\n");
    int core_id = get_core_id();
    if (index < 0 || index >= MAX_EVENTSET_NUM) dsp_abort(50);
    if (ESvalid[core_id][index] == 0) dsp_abort(50);
    if (EventSets[core_id][index].running == 0) dsp_abort(50);
    for (int i = 0; i < EventSets[core_id][index].eventNum; ++i) {
        prof_end(EventSets[core_id][index].events[i]);
        prof_start(EventSets[core_id][index].events[i]);
    }
    return PMU_OK;
}

void PMU_shutdown() {
    PRINT("In PMU_shutdown()\n");
    int core_id = get_core_id();
    for (int i = 0; i < MAX_EVENTSET_NUM; ++i) {
        if (ESvalid[core_id][i]) {
            if (EventSets[core_id][i].running) {
                for (int j = 0; j < EventSets[core_id][i].eventNum; ++j) {
                    prof_end(EventSets[core_id][i].events[j]);
                }
                EventSets[core_id][i].running = 0;
            }
            PMU_destroy_eventset(&i);
        }
    }
}

/*
 * should only be called when initializing accl_tracer,
 */
void MT_PMU_collector_init() {
    PRINT("In MT_PMU_collector_init()\n");
    int core_id = get_core_id();
    PMU_library_init();
    PMU_create_eventset(&default_es_index[core_id]);
    ESIndex[core_id] = default_es_index[core_id];
    int events[]     = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
    PMU_add_events(default_es_index[core_id], events, 26);
    PMU_start(ESIndex[core_id]);
}

/*
 * should only be called when finalizing accl_tracer
 */
void MT_PMU_collector_fin() {
    PMU_shutdown();
}

void MT_PMU_collector_get_all(unsigned long *value) {
    int core_id = get_core_id();
    if (core_id < 0 || core_id >= 24) {
        PRINT("MT_PMU_collector_get_all failed! core_id out of bounds!\n");
        dsp_abort(1);
    }

    PRINT("Before read in GET "
          "ALL:\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%"
          "lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu\n",
          value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], value[8], value[9], value[10],
          value[11], value[12], value[13], value[14], value[15], value[16], value[17], value[18], value[19], value[20],
          value[21], value[22], value[23], value[24], value[25]);
    if (PMU_read(ESIndex[core_id], value) != PMU_OK) {
        dsp_abort(20);
    }
    PRINT("After read in GET "
          "ALL:\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%"
          "lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu,\n%lu\n",
          value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], value[8], value[9], value[10],
          value[11], value[12], value[13], value[14], value[15], value[16], value[17], value[18], value[19], value[20],
          value[21], value[22], value[23], value[24], value[25]);
}

int MT_PMU_collector_get_num() {
    int core_id = get_core_id();
    if (core_id < 0 || core_id >= 24) dsp_abort(1);
    return EventSets[core_id][ESIndex[core_id]].eventNum;
}

void simple_pmu_start() {
    for (int i = 0; i < MAX_PMU_NUM; ++i) {
        prof_start(i);
    }
}

void simple_pmu_read(unsigned long *array) {
    for (int i = 0; i < MAX_PMU_NUM; ++i) {
        array[i] = prof_read(i);
    }
}

void simple_pmu_end() {
    for (int i = 0; i < MAX_PMU_NUM; ++i) {
        prof_end(i);
    }
}