set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

add_executable(kernel_hotspot driver.cpp)
target_compile_features(kernel_hotspot PRIVATE cxx_std_20)
target_include_directories(kernel_hotspot PRIVATE ${JSI_INCLUDE} ./ ${MPI_INCLUDE_PATH} ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${LLVMOpenMP_INCLUDE_DIRS})
target_link_libraries(kernel_hotspot ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})

install(TARGETS kernel_hotspot DESTINATION bin)
