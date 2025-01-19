
set(JSI_INCLUDE ../../include)
set(PSE_INCLUDE ../../lib/pse/include)
set(CONVERT_LIBRARY ${JSI_LIBRARY} ${MPI_LIBRARIES} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${OTF2_LIBRARIES})
set(PSE_LIBRARY ral FSL sql_backend)
add_executable(jsiconvert jsiconvert.cpp)
add_executable(jsiextract jsiextract.cpp)

target_include_directories(jsiextract SYSTEM PRIVATE ${OTF2_INCLUDE_DIRS} ${magic_enum_INCLUDE_DIRS})
target_include_directories(jsiextract PRIVATE ${JSI_INCLUDE} ${LIBUNWIND_INCLUDE_DIR} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH} ${PSE_INCLUDE})
target_link_directories(jsiextract PRIVATE ${OTF2_LIBRARY_DIRS})
target_link_libraries(jsiextract ${CONVERT_LIBRARY} otf2 jsi_reader)

target_link_libraries(jsiextract ${PSE_LIBRARY})
target_link_libraries(jsiconvert ${PSE_LIBRARY})
target_include_directories(jsiconvert PRIVATE ${JSI_INCLUDE} ${LIBUNWIND_INCLUDE_DIR} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH} ${PSE_INCLUDE})
target_link_libraries(jsiconvert jsi_reader ${CONVERT_LIBRARY})

install(TARGETS jsiconvert DESTINATION bin)