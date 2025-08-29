
add_executable(pmu_unified_test PMU_test.cpp)

target_include_directories(pmu_unified_test PRIVATE ../../../include)

# target_link_directories(pmu_unified_test PRIVATE /root/papi/lib ../../../install/lib)
target_link_directories(pmu_unified_test PRIVATE ${PAPI_LIBRARIES} ../../../install/lib)

target_link_libraries(pmu_unified_test unified_pmu_collector papi)
