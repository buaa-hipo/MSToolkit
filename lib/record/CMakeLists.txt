# SET(CMAKE_C_COMPILER ${MPI_C_COMPILER})
# SET(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})

# memory cmakelist
SET(JSI_TRACELIB jsi_record)

ADD_DEFINITIONS(-DDEBUG)
set(CMAKE_CXX_STANDARD 20)

set(JSI_INCLUDE ../../include)
# set(HEADER memory_instrument.h ${JSI_INCLUDE}/instrument/backtrace.h ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_writer.h ${JSI_INCLUDE}/utils/jsi_log.h ${JSI_INCLUDE}/utils/tsc_timer.h)

# add_library(memory_wrapper SHARED  memory_wrapper.cpp ${HEADER})
# target_compile_features(memory_wrapper PRIVATE)
# target_include_directories(memory_wrapper PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
# target_link_libraries(memory_wrapper dl g ${JSI_TRACELIB} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${MPI_LIBRARIES})

# install(TARGETS memory_wrapper DESTINATION bin)
# memory cmakelist end

SET(HEADER pthread_instrument.h memory_instrument.h ${JSI_INCLUDE}/utils/jsi_log.h ${JSI_INCLUDE}/utils/tsc_timer.h ../../include/record/record_type.h ../../include/record/record_defines.h ../../include/record/record_writer.h ../../include/instrument/backtrace.h ../../include/instrument/pmu_collector.h ../../include/record/record_meta.h ../../include/record/record_area.h ../../include/utils/jsi_io.h ../../include/utils/snappy/snappy.h ../../include/utils/safe.hpp)
SET(SNAPPY ../../include/utils/snappy/snappy.h ../../include/utils/snappy/snappy.cc ../../include/utils/snappy/snappy-c.h ../../include/utils/snappy/snappy-c.cc ../../include/utils/snappy/snappy-internal.h ../../include/utils/snappy/snappy-sinksource.h ../../include/utils/snappy/snappy-sinksource.cc ../../include/utils/snappy/snappy-stubs-internal.h ../../include/utils/snappy/snappy-stubs-internal.cc ../../include/utils/snappy/snappy-stubs-public.h ../../include/utils/snappy/snappy-stubs-public.h.in)
add_library(jsi_record SHARED safe.cpp error_handle.cpp pthread_wrapper.cpp memory_wrapper.cpp record_writer.cpp ../common/pmu_collector.cpp ../common/record_area.cpp ../common/record_meta.cpp ../common/stg.cpp ${HEADER} ${SNAPPY})
add_library(jsi_record_info SHARED record_type_info.cpp)

# target_compile_features(jsi_record PRIVATE cxx_std_17)
target_compile_features(jsi_record PRIVATE)
target_include_directories(jsi_record PRIVATE ../../include ./ ${LIBUNWIND_INCLUDE_DIR} ${PAPI_INCLUDE_DIRS}  ${LIBELF_INCLUDE_DIRS} ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH} ${LLVMOpenMP_INCLUDE_DIRS})
target_link_libraries(jsi_record g ${LIBUNWIND_LIBRARIES} ${PAPI_LIBRARIES} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${MPI_LIBRARIES} ${LLVMOpenMP_LIBRARIES} dl rt backtrace pthread jsi_record_info)
target_include_directories(jsi_record_info PRIVATE ../../include ./ ${LIBUNWIND_INCLUDE_DIR} ${PAPI_INCLUDE_DIRS}  ${LIBELF_INCLUDE_DIRS} ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH} ${LLVMOpenMP_INCLUDE_DIRS})
# target_link_libraries(jsi_record_info PRIVATE fmt)
install(TARGETS jsi_record DESTINATION lib)
install(TARGETS jsi_record_info DESTINATION lib)
