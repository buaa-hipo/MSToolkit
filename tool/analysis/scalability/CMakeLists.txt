set(JSI_INCLUDE ../../../include)


add_executable(scalability_analysis driver.cpp pbg.cpp ../timeline/alignment.cpp)

target_compile_features(scalability_analysis PRIVATE cxx_std_20)

target_include_directories(scalability_analysis PRIVATE ./)

target_include_directories(scalability_analysis PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${LLVMOpenMP_INCLUDE_DIRS})
target_link_libraries(scalability_analysis jsi_reader ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES} ${LLVMOpenMP_LIBRARIES})

install(TARGETS scalability_analysis DESTINATION bin)
