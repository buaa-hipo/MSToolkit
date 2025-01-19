set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

SET(HEADER ${JSI_INCLUDE}/record/record_type.h ${JSI_INCLUDE}/record/record_reader.h ${JSI_INCLUDE}/instrument/backtrace.h ./alignment.h)

add_executable(alignment_test alignment_test.cpp alignment_bf.cpp alignment.cpp ${HEADER})
target_compile_features(alignment_test PRIVATE cxx_std_17)
target_include_directories(alignment_test PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(alignment_test ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})

install(TARGETS alignment_test DESTINATION bin)
