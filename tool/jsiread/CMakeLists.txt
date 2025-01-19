# SET(CMAKE_C_COMPILER ${MPI_C_COMPILER})
# SET(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})

set(JSI_INCLUDE ../../include)
set(JSI_LIBRARY jsi_reader)

SET(HEADER ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_reader.h ${JSI_INCLUDE}/instrument/backtrace.h)

add_executable(jsiread jsiread.cpp ${HEADER})
target_compile_features(jsiread PRIVATE cxx_std_17)
target_include_directories(jsiread PRIVATE ${JSI_INCLUDE} ${LIBUNWIND_INCLUDE_DIR} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
target_link_libraries(jsiread ${JSI_LIBRARY} OpenMP::OpenMP_CXX ${MPI_LIBRARIES})

install(TARGETS jsiread DESTINATION bin)
