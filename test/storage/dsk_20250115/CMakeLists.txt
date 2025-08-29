

set(JSI_INCLUDE ../../../include)
set(CONVERT_LIBRARY ${JSI_LIBRARY} ${MPI_LIBRARIES} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${OTF2_LIBRARIES})

add_executable(record_reader_test record_reader_test.cpp)
target_include_directories(record_reader_test PUBLIC ../../../lib/pse/include)
target_link_libraries(record_reader_test PUBLIC FSL ral sql_backend  jsi_reader) 
target_include_directories(record_reader_test PRIVATE ${JSI_INCLUDE} ${LIBUNWIND_INCLUDE_DIR} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH} ${PSE_INCLUDE})

add_executable(write_trace write_trace.cpp)
target_include_directories(write_trace PUBLIC ../../../lib/pse/include)
target_link_libraries(write_trace PUBLIC FSL ral sql_backend jsi_record)
target_include_directories(write_trace PRIVATE ${JSI_INCLUDE} ${LIBUNWIND_INCLUDE_DIR} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH} ${PSE_INCLUDE})