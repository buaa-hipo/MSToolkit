set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

SET(HEADER ./memory_analysis.h ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_reader.h ${JSI_INCLUDE}/instrument/backtrace.h )

add_executable(memory_analysis driver.cpp memory_analysis.cpp ${HEADER})
target_compile_features(memory_analysis PRIVATE cxx_std_17)
target_include_directories(memory_analysis PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(memory_analysis ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})

install(TARGETS memory_analysis DESTINATION bin)