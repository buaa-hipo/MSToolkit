set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

SET(HEADER ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_reader.h ${JSI_INCLUDE}/instrument/backtrace.h ./comm_statistic.h ../graph/dependancy_graph.h)

add_executable(comm_statistic driver.cpp comm_statistic.cpp  ${HEADER})
target_compile_features(comm_statistic PRIVATE cxx_std_17)
target_include_directories(comm_statistic PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(comm_statistic ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})

install(TARGETS comm_statistic DESTINATION bin)