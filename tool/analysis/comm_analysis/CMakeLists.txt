set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

SET(HEADER ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_reader.h ${JSI_INCLUDE}/instrument/backtrace.h ./comm_analysis.h)

add_executable(comm_analysis driver.cpp comm_analysis.cpp ${HEADER})
target_compile_features(comm_analysis PRIVATE cxx_std_17)
target_include_directories(comm_analysis PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(comm_analysis ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} OpenMP::OpenMP_CXX)

install(TARGETS comm_analysis DESTINATION bin)
