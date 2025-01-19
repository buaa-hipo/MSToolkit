cmake_minimum_required(VERSION 3.21)
# enable_language(HIP)

SET(JSI_RECORD jsi_record)

# Instrumentation library
set(INCLUDE_DIR ../../../include)
set(HEADER ${INCLUDE_DIR}/instrument/backtrace.h ${INCLUDE_DIR}/record/record_type.h ${INCLUDE_DIR}/record/record_writer.h ${INCLUDE_DIR}/utils/jsi_log.h ${INCLUDE_DIR}/utils/tsc_timer.h)
set(ACCL_TRACER_SRCS "rocm_tracer.hip")
set_source_files_properties(${ACCL_TRACER_SRCS} PROPERTIES HIP_SOURCE_PROPERTY_FORMAT 1)
hip_add_library(accl_tracer SHARED ${ACCL_TRACER_SRCS} ${HEADER})
target_compile_features(accl_tracer PRIVATE cxx_std_17)
target_include_directories(accl_tracer PRIVATE ${INCLUDE_DIR} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_include_directories(accl_tracer PRIVATE ${ROCM_PATH}/include ${ROCM_PATH}/roctracer/include)
target_link_libraries(accl_tracer ${ROCM_PATH}/roctracer/lib/libroctracer64.so)
target_link_libraries(accl_tracer ${PAPI_LIBRARIES} ${JSI_RECORD} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})
target_compile_options(accl_tracer PRIVATE ${WARNINGS})

install(TARGETS accl_tracer DESTINATION bin)
