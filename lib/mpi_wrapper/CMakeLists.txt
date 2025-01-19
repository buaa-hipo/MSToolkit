# Required packages: libunwind, papi, jsoncpp

# SET(CMAKE_C_COMPILER ${MPI_C_COMPILER})
# SET(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
SET(JSI_TRACELIB jsi_record)

#SET(CMAKE_CXX_FLAGS "-Wall -g -O2 -Wno-deprecated-declarations -Wno-sign-compare")
#SET(CMAKE_SHARED_LINKER_FLAGS "-g")
ADD_DEFINITIONS(-DDEBUG)
set(CMAKE_CXX_STANDARD 20)

# shared library
set(WRAP ${PROJECT_SOURCE_DIR}/lib/mpi_wrapper/wrap/wrap.py)
set(WRAP_FLAGS "-f")
include(${PROJECT_SOURCE_DIR}/lib/mpi_wrapper/wrap/WrapConfig.cmake)
set(JSI_INCLUDE ../../include)

# C wrapper generation
add_wrapped_file(mpi_wrapper.cpp mpi_wrapper.w TRUE)

set(HEADER ${JSI_INCLUDE}/instrument/backtrace.h ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_writer.h ${JSI_INCLUDE}/utils/jsi_log.h ${JSI_INCLUDE}/utils/tsc_timer.h)

add_library(mpi_wrapper SHARED mpi_instrument.h ${CMAKE_CURRENT_BINARY_DIR}/mpi_wrapper.cpp ${HEADER})
target_compile_features(mpi_wrapper PRIVATE cxx_std_17)

target_include_directories(mpi_wrapper PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
target_link_libraries(mpi_wrapper ${JSI_TRACELIB} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${MPI_LIBRARIES})

install(TARGETS mpi_wrapper DESTINATION bin)
