set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

SET(HEADER ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_reader.h ${JSI_INCLUDE}/instrument/backtrace.h ./dependancy_graph.h ./json_writer.h ../timeline/alignment.h)

add_executable(timeslice_analysis driver.cpp dependancy_graph.cpp ../timeline/alignment.cpp ${HEADER})

target_compile_features(timeslice_analysis PRIVATE cxx_std_17)
target_include_directories(timeslice_analysis PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(timeslice_analysis ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})

install(TARGETS timeslice_analysis DESTINATION bin)