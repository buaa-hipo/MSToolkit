#include "instrument/MT_PMU_collector.h"
#include <pthread.h>
#include <papi.h>

int PMU_library_init() { return PAPI_library_init(PAPI_VER_CURRENT); }

int PMU_thread_init() { return PAPI_thread_init(pthread_self); }

int PMU_add_event(int index, int Event) { return PAPI_add_event(index, Event); }

int PMU_add_events(int EventSet, int *Events, int number) {
  return PAPI_add_events(EventSet, Events, number);
}

int PMU_add_named_event(int EventSet, const char *EventName) {
  return PAPI_add_named_event(EventSet, EventName);
}

int PMU_create_eventset(int *EventSet) {
  return PAPI_create_eventset(EventSet);
}

int PMU_destroy_eventset(int *EventSet) {
  return PAPI_destroy_eventset(EventSet);
}

int PMU_cleanup_eventset(int EventSet) {
  return PAPI_cleanup_eventset(EventSet);
}

int PMU_start(int EventSet) { return PAPI_start(EventSet); }

int PMU_stop(int EventSet, unsigned long *values) {
  return PAPI_stop(EventSet, (long long*)values);
}

int PMU_read(int EventSet, unsigned long *values) {
  return PAPI_read(EventSet, (long long*)values);
}

int PMU_reset(int EventSet) { return PAPI_reset(EventSet); }

void PMU_shutdown() { PAPI_shutdown(); }
