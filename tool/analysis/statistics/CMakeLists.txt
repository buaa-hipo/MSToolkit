set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

SET(HEADER ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_reader.h ${JSI_INCLUDE}/instrument/backtrace.h ./statistic.h ./topdown_tree.h ./bottomup_tree.h)

add_executable(statistic_driver statistic_driver.cpp statistic.cpp topdown_tree.cpp bottomup_tree.cpp ${HEADER})
target_compile_features(statistic_driver PRIVATE cxx_std_17)
target_include_directories(statistic_driver PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(statistic_driver ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})

install(TARGETS statistic_driver DESTINATION bin)
