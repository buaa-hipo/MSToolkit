#include "instrument/pmu_collector.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <thread>

#include "utils/jsi_log.h"

#undef NDEBUG

#define PMU_EVENT "PAPI_TOT_CYC"

int main() {
    setenv("JSI_COLLECT_PMU_EVENT", PMU_EVENT, true);
    assert(pmu_collector_init());

    auto val1 = pmu_collector_get(0);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    uint64_t val2;
    int r = pmu_collector_get_all(&val2);

    assert(r == 1);
    assert(val1 <= val2);
    JSI_INFO("PAPI Event count diff: %lu\n", val2 - val1);

    assert(pmu_collector_fini());

    return 0;
}
