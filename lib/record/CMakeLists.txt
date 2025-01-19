# SET(CMAKE_C_COMPILER ${MPI_C_COMPILER})
# SET(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})

SET(HEADER ../../include/record/record_type.h ../../include/record/record_defines.h ../../include/record/record_writer.h ../../include/instrument/backtrace.h ../../include/instrument/pmu_collector.h ../../include/record/record_meta.h ../../include/record/record_area.h ../../include/utils/jsi_io.h ../../include/utils/snappy/snappy.h)
SET(SNAPPY ../../include/utils/snappy/snappy.h ../../include/utils/snappy/snappy.cc ../../include/utils/snappy/snappy-c.h ../../include/utils/snappy/snappy-c.cc ../../include/utils/snappy/snappy-internal.h ../../include/utils/snappy/snappy-sinksource.h ../../include/utils/snappy/snappy-sinksource.cc ../../include/utils/snappy/snappy-stubs-internal.h ../../include/utils/snappy/snappy-stubs-internal.cc ../../include/utils/snappy/snappy-stubs-public.h ../../include/utils/snappy/snappy-stubs-public.h.in)
add_library(jsi_record SHARED record_writer.cpp ../common/pmu_collector.cpp ../common/record_area.cpp ../common/record_meta.cpp ../common/stg.cpp ${HEADER} ${SNAPPY})
target_compile_features(jsi_record PRIVATE cxx_std_17)
target_include_directories(jsi_record PRIVATE ../../include ./ ${LIBUNWIND_INCLUDE_DIR} ${PAPI_INCLUDE_DIRS}  ${LIBELF_INCLUDE_DIRS} ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
target_link_libraries(jsi_record ${LIBUNWIND_LIBRARIES} ${PAPI_LIBRARIES} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${MPI_LIBRARIES} dl rt backtrace)

install(TARGETS jsi_record DESTINATION lib)
