# SET(CMAKE_C_COMPILER ${MPI_C_COMPILER})
# SET(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
set(JSI_LIBRARY jsi_reader)

SET(HEADER ../../include/compress/compress.h ../../include/instrument/backtrace.h ../../include/compress/snappy_compress.h ../../include/record/record_type.h ../../include/record/record_defines.h ../../include/record/record_reader.h ../../include/record/record_meta.h ../../include/record/record_area.h ../../include/utils/jsi_io.h)
SET(SNAPPY ../../include/utils/snappy/snappy.h ../../include/utils/snappy/snappy.cc ../../include/utils/snappy/snappy-c.h ../../include/utils/snappy/snappy-c.cc ../../include/utils/snappy/snappy-internal.h ../../include/utils/snappy/snappy-sinksource.h ../../include/utils/snappy/snappy-sinksource.cc ../../include/utils/snappy/snappy-stubs-internal.h ../../include/utils/snappy/snappy-stubs-internal.cc ../../include/utils/snappy/snappy-stubs-public.h ../../include/utils/snappy/snappy-stubs-public.h.in)
add_library(jsi_compress SHARED snappy_compress.cpp compress.cpp ../common/record_area.cpp ../common/record_meta.cpp ../reader/record_reader.cpp  ${HEADER} ${SNAPPY})
target_compile_features(jsi_compress PRIVATE cxx_std_17)
target_include_directories(jsi_compress PRIVATE ../../include ../../include/compress ${LIBUNWIND_INCLUDE_DIR} ${PAPI_INCLUDE_DIRS}  ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
target_link_libraries(jsi_compress ${JSI_LIBRARY} ${LIBUNWIND_LIBRARIES} ${PAPI_LIBRARIES} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${MPI_LIBRARIES} dl rt backtrace)

install(TARGETS jsi_compress DESTINATION lib)
