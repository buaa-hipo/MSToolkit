#include "instrument/MT_PMU_collector.h"
#include <iostream>
using namespace std;

int main() {
    PMU_library_init();
    PMU_thread_init();
    int event_set = PMU_NULL;
    if (PMU_create_eventset(&event_set) != PMU_OK) return 1;
    if (PMU_add_named_event(event_set, "PAPI_TOT_CYC") != PMU_OK) return 1;
    if (PMU_start(event_set) != PMU_OK) return 1;
    PMU_reset(event_set);
    unsigned long value;
    PMU_read(event_set, &value);
    cout << "CYCLE before workload: " << value << endl;
    int tmp = 0;
    for (int i = 0; i < 1000; ++i) {
        tmp += i;
    }
    cout << "tmp = " << tmp << endl;
    PMU_stop(event_set, &value);
    cout << "CYCLE after workload: " << value << endl;
    PMU_cleanup_eventset(event_set);
    PMU_destroy_eventset(&event_set);
    PMU_shutdown();
    return 0;
}