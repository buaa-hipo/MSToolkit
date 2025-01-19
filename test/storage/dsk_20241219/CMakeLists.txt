

set(JSI_INCLUDE ../../../include)
set(CONVERT_LIBRARY ${JSI_LIBRARY} ${MPI_LIBRARIES} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${OTF2_LIBRARIES})

add_executable(storage_interface_write_test storage_interface_write_test.cpp)
target_include_directories(storage_interface_write_test PUBLIC ../../../lib/pse/include)
target_link_libraries(storage_interface_write_test PUBLIC FSL ral sql_backend jsi_record)
target_include_directories(storage_interface_write_test PRIVATE ${JSI_INCLUDE} ${LIBUNWIND_INCLUDE_DIR} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH} ${PSE_INCLUDE})

add_executable(storage_interface_read_test storage_interface_read_test.cpp)
target_include_directories(storage_interface_read_test PUBLIC ../../../lib/pse/include)
target_link_libraries(storage_interface_read_test PUBLIC FSL ral sql_backend)
target_include_directories(storage_interface_read_test PRIVATE ${JSI_INCLUDE} ${LIBUNWIND_INCLUDE_DIR} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH} ${PSE_INCLUDE})