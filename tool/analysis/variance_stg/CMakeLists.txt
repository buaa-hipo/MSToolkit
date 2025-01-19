set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

SET(HEADER ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_reader.h ${JSI_INCLUDE}/instrument/backtrace.h ./variance.h ../graph/dependancy_graph.h ../timeline/alignment.h)

add_executable(variance_stg driver.cpp variance.cpp ../graph/dependancy_graph.cpp ../timeline/alignment_bf.cpp ${HEADER})
target_compile_features(variance_stg PRIVATE cxx_std_17)
target_include_directories(variance_stg PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(variance_stg ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})


install(TARGETS variance_stg DESTINATION bin)