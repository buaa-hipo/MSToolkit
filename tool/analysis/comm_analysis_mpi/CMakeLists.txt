set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

SET(HEADER ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_reader.h ${JSI_INCLUDE}/instrument/backtrace.h ./comm_analysis_mpi.h)

add_executable(comm_analysis_mpi driver.cpp comm_analysis_mpi.cpp ${HEADER})
target_compile_features(comm_analysis_mpi PRIVATE cxx_std_17)
target_include_directories(comm_analysis_mpi PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
target_link_libraries(comm_analysis_mpi ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} OpenMP::OpenMP_CXX ${MPI_LIBRARIES})

install(TARGETS comm_analysis_mpi DESTINATION bin)
