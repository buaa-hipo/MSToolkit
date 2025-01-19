SET(JSI_TRACELIB jsi_record)

ADD_DEFINITIONS(-DDEBUG)
set(CMAKE_CXX_STANDARD 20)

set(JSI_INCLUDE ../../include)
set(HEADER memory_instrument.h ${JSI_INCLUDE}/instrument/backtrace.h ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_writer.h ${JSI_INCLUDE}/utils/jsi_log.h ${JSI_INCLUDE}/utils/tsc_timer.h)

add_library(memory_wrapper SHARED  memory_wrapper.cpp ${HEADER})
target_compile_features(memory_wrapper PRIVATE)
target_include_directories(memory_wrapper PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
target_link_libraries(memory_wrapper dl g ${JSI_TRACELIB} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${MPI_LIBRARIES})

install(TARGETS memory_wrapper DESTINATION bin)
