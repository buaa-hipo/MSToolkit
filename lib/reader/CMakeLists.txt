project(tracelib LANGUAGES CXX)

SET(HEADER ../../include/record/record_type.h ../../include/record/record_defines.h ../../include/record/record_reader.h ../../include/instrument/backtrace.h ../../include/record/record_meta.h ../../include/record/record_area.h ../../include/utils/jsi_io.h ../../include/utils/snappy/snappy.h ../../include/utils/dwarf_line_info.h)
SET(READER_SRCS record_reader.cpp ../common/dwarf_line_info.cpp ../common/record_meta.cpp)

set(HELPER_HEADERS "../../include/record/accl_helper.h")
if (USE_ROCM)
    set(ACCL_SRCS "accl_helper/accl_helper_rocm.hip")
    set_source_files_properties(${ACCL_SRCS} PROPERTIES HIP_SOURCE_PROPERTY_FORMAT 1)
    hip_add_library(jsi_reader SHARED ${ACCL_HEADERS} ${ACCL_SRCS} ${READER_SRCS} ${HELPER_HEADERS} ${HEADER})
    target_link_libraries(jsi_reader ${ROCM_PATH}/roctracer/lib/libroctracer64.so)
    target_include_directories(jsi_reader PUBLIC ${ROCM_PATH}/include ${ROCM_PATH}/roctracer/include)
    message(STATUS "ROCM ACCL ENABLED FOR READER")
else()
    set(ACCL_SRCS "accl_helper/accl_helper_default.cpp")
    add_library(jsi_reader SHARED ${ACCL_HEADERS} ${ACCL_SRCS} ${READER_SRCS} ${HELPER_HEADERS} ${HEADER})
    message(STATUS "ACCL DISABLED FOR READER")
endif()

target_compile_features(jsi_reader PRIVATE cxx_std_17)
target_include_directories(jsi_reader PRIVATE ../../include ./ ${LIBUNWIND_INCLUDE_DIR} ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
target_link_libraries(jsi_reader ${LIBUNWIND_LIBRARIES} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} dl rt OpenMP::OpenMP_CXX ${MPI_LIBRARIES} backtrace)

install(TARGETS jsi_reader DESTINATION lib)
