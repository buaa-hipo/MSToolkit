#include "instrument/MT_PMU_collector.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hthread_device.h"

/**
 *  ALL functions are implemented for single core
 *  GLOBAL variables have 24 copies each
 *  prof_end() would not reset the PMC
 *  hthread_group_destroy() would call prof_end() (presume)
 *  hthread_dat_unload() would not
 */

ESInfo EventSets[24][MAX_EVENTSET_NUM];
int ESvalid[24][MAX_EVENTSET_NUM] = {0};
int ESIndex[24] = {0};
int EventCount[24] = {0};
// char** EventList;

void PMU_CHECK(pmu_api_status_t status, const char *func_name) {
    if (status != PMU_OK) {
        hthread_printf("ERROR code %d in %s\n", status, func_name);
        exit(status);
    }
}

void parse_pmu_event_list(
        const char *event_name_list_str, char ***event_list, int *count) {
    char *str_copy = strdup(event_name_list_str);
    char *token = strtok(str_copy, ",");
    int capacity = 8;
    int index = 0;
    *event_list = (char**)malloc(capacity * sizeof(char *));
    while (token != NULL) {
        if (index >= capacity) {
            // realloc when limit exceeded
            capacity = (int)(capacity * 1.5);
            *event_list = (char **)realloc(*event_list, capacity * sizeof(char*));
        }
        (*event_list)[index] = strdup(token);
        index++;

        token = strtok(NULL, ",");
    }
    *count = index;
    free(str_copy);
}


pmu_api_status_t PMU_library_init() {
    int core_id = get_core_id();
    memset(ESvalid[core_id], 0, sizeof(ESvalid[core_id]));
    memset(EventSets[core_id], -1, sizeof(EventSets[core_id]));
    return PMU_OK;
}

pmu_api_status_t PMU_add_event(int index, int event) {
    pmu_api_status_t err = PMU_add_events(index, &event, 1);
    if (err) return err;
    return PMU_OK;
}

pmu_api_status_t PMU_add_events(int index, int *events, int number) {
    #ifdef DEBUG
    hthread_printf("[core-%d] In PMU_add_events(), index=%d, ESvalid[tid][index]=%d\n", get_core_id(), index, ESvalid[get_core_id()][index]);
    #endif
    int tid = get_core_id();
    if (index < 0 || ESvalid[tid][index] == 0) return PMU_EINVAL;
    if (EventSets[tid][index].running) return PMU_EISRUN;
    if (EventSets[tid][index].eventNum + number > MAX_PMU_NUM) return PMU_EBIGSET;
    for (int i = 0; i < number; ++i) {
        if (events[i] < 0 || events[i] >= 26) return PMU_ENOEVNT;   // todo: retain valid event code?
        int pos = EventSets[tid][index].eventNum;
        if (pos == -1) {
            EventSets[tid][index].eventNum = 1;
            EventSets[tid][index].events[0] = events[i];
        } else {
            EventSets[tid][index].events[pos] = events[i];
            EventSets[tid][index].eventNum++;
        }
    }
    return PMU_OK;
}

pmu_api_status_t PMU_add_named_event(int index, const char *event_name) {
    int event_code;
    pmu_api_status_t err = PMU_event_name_to_code(event_name, &event_code);
    if (err) return err;
    return PMU_add_event(index, event_code);
}

pmu_api_status_t PMU_create_eventset(int *index) {
    #ifdef DEBUG
    hthread_printf("[core-%d] In PMU_create_eventset()\n", get_core_id());
    #endif
    int tid = get_core_id();
    // if (*index < 0 || *index >= MAX_EVENTSET_NUM) return PMU_EINVAL;
    if (*index != PMU_NULL) return PMU_EINVAL;  // todo: is this right ?
    for (int i = 0; i < MAX_EVENTSET_NUM; ++i) {
        if (ESvalid[tid][i] == 0) {
            *index = i;
            ESvalid[tid][i] = 1;
            EventSets[tid][i].running = 0;
            // there is no need to set eventNum to 0
            // it will be used to locate newly added event
            #ifdef DEBUG
            hthread_printf("[core-%d] Got available es id - %d\n", tid, i);
            hthread_printf("[core-%d] ESvalid[tid][i] now is %d\n", tid, ESvalid[tid][i]);
            #endif
            return PMU_OK;
        }
    }
    hthread_printf("Max number of event sets reached\n");
    return PMU_EINVAL;
}

pmu_api_status_t PMU_destroy_eventset(int *index) {
    #ifdef DEBUG
    hthread_printf("In PMU_destroy_eventset()\n");
    #endif
    if (*index < 0 || *index >= MAX_EVENTSET_NUM) return PMU_EINVAL;
    if (ESvalid[get_core_id()][*index] == 0) return PMU_OK;
    return PMU_EINVAL;
}

pmu_api_status_t PMU_cleanup_eventset(int index) {
    #ifdef DEBUG
    hthread_printf("[core-%d] In PMU_cleanup_eventset()\n", get_core_id());
    #endif
    if (index < 0 || index >= MAX_EVENTSET_NUM) return PMU_EINVAL;
    if (ESvalid[get_core_id()][index] == 0) return PMU_EINVAL;
    ESvalid[get_core_id()][index] = 0;
    memset(EventSets[get_core_id()] + index, -1, sizeof(ESInfo));
    EventSets[get_core_id()][index].running = 0;
    return PMU_OK;
}

pmu_api_status_t PMU_start(int index) {
    #ifdef DEBUG
    hthread_printf("[core-%d] In PMU_start()\n", get_core_id());
    #endif
    int tid = get_core_id();
    if (index < 0 || index >= MAX_EVENTSET_NUM) return PMU_EINVAL;
    if (ESvalid[tid][index] == 0) return PMU_EINVAL;
    if (EventSets[tid][index].running) return PMU_EISRUN;
    EventSets[tid][index].running = 1;
    for (int i = 0; i < EventSets[tid][index].eventNum; ++i) {
        prof_start(EventSets[tid][index].events[i]);
    }
    return PMU_OK;
}

pmu_api_status_t PMU_stop(int index, uint64_t *values) {
    #ifdef DEBUG
    hthread_printf("[core-%d] In PMU_stop()\n", get_core_id());
    #endif
    int tid = get_core_id();
    if (index < 0 || index >= MAX_EVENTSET_NUM) return PMU_EINVAL;
    if (ESvalid[tid][index] == 0) return PMU_EINVAL;
    if (EventSets[tid][index].running == 0) return PMU_EINVAL;
    for (int i = 0; i < EventSets[tid][index].eventNum; ++i) {
        values[i] = prof_end(EventSets[tid][index].events[i]);
    }
    EventSets[tid][index].running = 0;
    return PMU_OK;
}

pmu_api_status_t PMU_read(int index, uint64_t *values) {
    #ifdef DEBUG
    hthread_printf("[core-%d] In PMU_read()\n", get_core_id());
    #endif
    int tid = get_core_id();
    if (index < 0 || index >= MAX_EVENTSET_NUM) return PMU_EINVAL;
    if (ESvalid[tid][index] == 0) return PMU_EINVAL;
    if (EventSets[tid][index].running == 0) return PMU_EINVAL;
    for (int i = 0; i < EventSets[tid][index].eventNum; ++i) {
        values[i] = prof_read(EventSets[tid][index].events[i]);
    }
    return PMU_OK;
}

pmu_api_status_t PMU_reset(int index) {
    #ifdef DEBUG
    hthread_printf("In PMU_reset()\n");
    #endif
    int tid = get_core_id();
    if (index < 0 || index >= MAX_EVENTSET_NUM) return PMU_EINVAL;
    if (ESvalid[tid][index] == 0) return PMU_EINVAL;
    if (EventSets[tid][index].running == 0) return PMU_EINVAL;
    for (int i = 0; i < EventSets[tid][index].eventNum; ++i) {
        prof_end(EventSets[tid][index].events[i]);
        prof_start(EventSets[tid][index].events[i]);
    }
    return PMU_OK;
}

void PMU_shutdown() {
    #ifdef DEBUG
    hthread_printf("In PMU_shutdown()\n");
    #endif
    int tid = get_core_id();
    for (int i = 0; i < MAX_EVENTSET_NUM; ++i) {
        if (ESvalid[tid][i]) {
            if (EventSets[tid][i].running) {
                for (int j = 0; j < EventSets[tid][i].eventNum; ++j) {
                    prof_end(EventSets[tid][i].events[j]);
                }
            }
            PMU_cleanup_eventset(i);
        }
    }
}

/*
 * should only be called when initializing accl_tracer,
 */
void MT_PMU_collector_init() {
    #ifdef DEBUG
    hthread_printf("In MT_PMU_collector_init()\n");
    #endif
    int tid = get_core_id();
    int es = PMU_NULL;
    PMU_CHECK(PMU_library_init(), "PMU_library_init");
    PMU_CHECK(PMU_create_eventset(&es), "PMU_create_eventset");
    hthread_printf("[core-%d] Default eventset - %d\n", tid, es);
    ESIndex[tid] = es;
    int events[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
    // EventList = code2name;
    PMU_CHECK(PMU_add_events(es, events, 26), "PMU_add_events");
    EventCount[tid] = 26;
    PMU_start(ESIndex[tid]);
}

/*
 * should only be called when finalizing accl_tracer
 */
void MT_PMU_collector_fin() {
    PMU_shutdown();
}

void MT_PMU_collector_get_all(uint64_t *value) {
    int tid = get_core_id();
    if (tid < 0 || tid >= 24) exit(1);
    PMU_CHECK(PMU_read(ESIndex[tid], value), "PMU_read");
}

int MT_PMU_collector_get_num() {
    int tid = get_core_id();
    if (tid < 0 || tid >= 24) exit(1);
    return EventSets[tid][ESIndex[tid]].eventNum;
}
