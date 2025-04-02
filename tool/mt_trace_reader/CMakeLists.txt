cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)

set(HEADER ${INCLUDE_DIR}/record/record_type.h)

add_definitions(-DMATRIX)

set(ACCL_READER mt_trace_reader.cpp)

set(MATRIX_INCLUDE_DIR ../../include/)

add_executable(mt_trace_reader ${ACCL_READER})

target_include_directories(mt_trace_reader PRIVATE ${MATRIX_INCLUDE_DIR} ${LIBDWARF_INCLUDE_DIRS} ${LIBUNWIND_INCLUDE_DIR} ./ ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})

install(TARGETS mt_trace_reader DESTINATION tools)

