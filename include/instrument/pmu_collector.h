#ifndef __JSI_PMU_COLLECTOR_H__
#define __JSI_PMU_COLLECTOR_H__

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <regex>


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

static inline void parse_dev_pmu_events_list(const char *events_str, std::vector<std::string> *event_list) {
    if (events_str == nullptr) {
        return;
    }
    std::string events = events_str;
    std::regex pattern("hthread:::([^,]*)");
    std::smatch match;

    auto start = events.cbegin();
    while (std::regex_search(start, events.cend(), match, pattern)) {
        if (match.size() > 1) {
            event_list->emplace_back(match.str(1));
        }
        start = match.suffix().first;
    }
}

#endif