cmake_minimum_required(VERSION 3.21)

SET(JSI_RECORD jsi_record)

enable_language(HIP)
# Instrumentation library
set(INCLUDE_DIR ../../../include)
# set(HEADER ${INCLUDE_DIR}/instrument/backtrace.h ${INCLUDE_DIR}/record/record_type.h ${INCLUDE_DIR}/record/record_writer.h ${INCLUDE_DIR}/utils/jsi_log.h ${INCLUDE_DIR}/utils/tsc_timer.h)
set(ACCL_TRACER_SRCS "rocm_tracer.hip")

set(ACCL_HELPER_SRCS "rocm_helper.cpp")
# set_source_files_properties(${ACCL_TRACER_SRCS} PROPERTIES HIP_SOURCE_PROPERTY_FORMAT 1)

# hip_add_library(accl_tracer SHARED ${ACCL_TRACER_SRCS} ${HEADER})
hip_add_library(accl_tracer SHARED ${ACCL_TRACER_SRCS})
#target_compile_options(accl_tracer PUBLIC -DDTK_24)
target_compile_options(accl_tracer PUBLIC -DDTK_23)
set_target_properties(accl_tracer PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)


# target_compile_features(accl_tracer PRIVATE cxx_std_20)

find_package(fmt CONFIG REQUIRED)
# get_target_property(fmt_INCLUDE_DIRS fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
# message(INFO "fmt_INCLUDE_DIRS: ${fmt_INCLUDE_DIRS}")
# link_libraries(fmt::fmt)
# include_directories(${fmt_INCLUDE_DIRS})

find_package(spdlog CONFIG REQUIRED)
link_libraries(spdlog::spdlog)

add_library(accl_helper SHARED ${ACCL_HELPER_SRCS})
target_include_directories(accl_helper PRIVATE ${INCLUDE_DIR} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${LLVMOpenMP_INCLUDE_DIRS})
# target_link_libraries(accl_helper ${PAPI_LIBRARIES} ${JSI_RECORD} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})
target_link_libraries(accl_helper ${PAPI_LIBRARIES} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${LLVMOpenMP_LIBRARIES})

target_include_directories(accl_tracer PRIVATE ${INCLUDE_DIR} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${LLVMOpenMP_INCLUDE_DIRS})
target_include_directories(accl_tracer PRIVATE ${ROCM_PATH}/include ${ROCM_PATH}/roctracer/include)
target_link_libraries(accl_tracer ${ROCM_PATH}/roctracer/lib/libroctracer64.so)
target_link_libraries(accl_tracer ${PAPI_LIBRARIES} ${JSI_RECORD} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${LLVMOpenMP_LIBRARIES})
target_compile_options(accl_tracer PRIVATE ${WARNINGS})

install(TARGETS accl_tracer DESTINATION bin)
install(TARGETS accl_helper DESTINATION bin)
