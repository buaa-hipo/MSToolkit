cmake_minimum_required(VERSION 3.13.0)
project(tracelib LANGUAGES CXX)

find_package(MPI REQUIRED)

SET(JSI_TRACELIB jsi_record)

set(TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}" CACHE STRING "Target architecture")
if (TARGET_ARCH MATCHES "^arm64" OR TARGET_ARCH MATCHES "^aarch64")
  message(STATUS "Building for AArch64")
  add_compile_definitions(AARCH64)
endif()

# Instrumentation library
set(HEADER ../../include/instrument/backtrace.h ../../include/record/record_type.h ../../include/record/record_writer.h ../../include/utils/jsi_log.h ../../include/utils/tsc_timer.h)
add_library(tracelib SHARED tracelib.cpp ${HEADER})
target_compile_features(tracelib PRIVATE cxx_std_17)
target_include_directories(tracelib PRIVATE ../../include ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(tracelib ${PAPI_LIBRARIES} ${JSI_TRACELIB} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} backtrace)
target_compile_options(tracelib PRIVATE ${WARNINGS})

install(TARGETS tracelib DESTINATION lib)
