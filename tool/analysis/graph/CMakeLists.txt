set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

SET(HEADER ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_reader.h ${JSI_INCLUDE}/instrument/backtrace.h ./dependancy_graph.h ./json_writer.h ../timeline/alignment.h)

add_executable(graph_analysis driver.cpp dependancy_graph.cpp ../timeline/alignment.cpp ${HEADER})
add_executable(graph_analysis_mt driver.cpp dependancy_graph_omp.cpp ../timeline/alignment.cpp ${HEADER})
target_compile_features(graph_analysis PRIVATE cxx_std_17)
target_compile_features(graph_analysis_mt PRIVATE cxx_std_17)
target_include_directories(graph_analysis PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_include_directories(graph_analysis_mt PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(graph_analysis ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})
target_link_libraries(graph_analysis_mt ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})

install(TARGETS graph_analysis DESTINATION bin)
install(TARGETS graph_analysis_mt DESTINATION bin)
