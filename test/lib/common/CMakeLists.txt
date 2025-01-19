# SET(CMAKE_C_COMPILER ${MPI_C_COMPILER})
# SET(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
set(JSI_LIBRARY jsi_reader)

add_executable(backtrace_test backtrace_test.cpp)
add_executable(sampling_code2measure sampling_code2measure.cpp)
target_compile_features(backtrace_test PRIVATE cxx_std_20)
target_include_directories(backtrace_test PRIVATE ../../../include ./ ${LIBUNWIND_INCLUDE_DIR} ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(backtrace_test ${JSI_LIBRARY} ${LIBUNWIND_LIBRARIES} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} dl) 
target_compile_options(backtrace_test PRIVATE -fsanitize=address)
target_link_options(backtrace_test PRIVATE -fsanitize=address)

SET(SNAPPY ../../../include/utils/snappy/snappy.h ../../../include/utils/snappy/snappy.cc ../../../include/utils/snappy/snappy-c.h ../../../include/utils/snappy/snappy-c.cc ../../../include/utils/snappy/snappy-internal.h ../../../include/utils/snappy/snappy-sinksource.h ../../../include/utils/snappy/snappy-sinksource.cc ../../../include/utils/snappy/snappy-stubs-internal.h ../../../include/utils/snappy/snappy-stubs-internal.cc ../../../include/utils/snappy/snappy-stubs-public.h ../../../include/utils/snappy/snappy-stubs-public.h.in)
add_executable(pmu_collector_test pmu_collector_test.cpp
        ../../../lib/common/pmu_collector.cpp
        ../../../lib/common/record_meta.cpp
        ../../../lib/record/record_writer.cpp
        ../../../lib/common/record_area.cpp
        ${SNAPPY}
        )
target_include_directories(pmu_collector_test PRIVATE ../../../include ./ ${PAPI_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
target_link_libraries(pmu_collector_test ${PAPI_LIBRARIES} ${MPI_LIBRARIES} backtrace)
target_link_libraries(pmu_collector_test rt)
target_compile_options(pmu_collector_test PRIVATE -fsanitize=address)
target_link_options(pmu_collector_test PRIVATE -fsanitize=address)
