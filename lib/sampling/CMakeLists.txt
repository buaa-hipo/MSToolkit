SET(JSI_TRACELIB jsi_record)

ADD_DEFINITIONS(-DDEBUG)
set(CMAKE_CXX_STANDARD 20)

set(JSI_INCLUDE ../../include)
SET(HEADER ${JSI_INCLUDE}/instrument/pmu_collector.h ${JSI_INCLUDE}/record/record_writer.h ${JSI_INCLUDE}/utils/jsi_log.h ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/wrap_defines.h ${JSI_INCLUDE}/utils/tsc_timer.h)

add_library(sampling SHARED  pmu_sampling.cpp ${HEADER})
target_compile_features(sampling PRIVATE)
target_include_directories(sampling PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH} ${PAPI_INCLUDE_DIRS})
target_link_libraries(sampling ${JSI_TRACELIB} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${MPI_LIBRARIES} ${PAPI_LIBRARIES})

install(TARGETS sampling DESTINATION bin)
