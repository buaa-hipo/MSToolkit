#ifndef __JSI_PMU_COLLECTOR_H__
#define __JSI_PMU_COLLECTOR_H__

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using pmu_val_type = long long;

bool pmu_collector_init();
bool pmu_collector_fini();
uint64_t pmu_collector_get(int i);
int pmu_collector_get_all(uint64_t *);
int pmu_collector_get_num_events();

static inline void parse_pmu_event_list(const char *event_name_list_str, std::vector<std::string> *event_list) {
    char *buf = strdup(event_name_list_str);
    char *event_name = strtok(buf, ",");
    while (event_name != nullptr) {
        event_list->emplace_back(event_name);
        event_name = strtok(nullptr, ",");
    }
    free(buf);
}

#endif