set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

set(HEADER
        ${JSI_INCLUDE}/record/record_type.h
        ${JSI_INCLUDE}/record/record_reader.h
        ${JSI_INCLUDE}/instrument/backtrace.h
        ../graph/dependancy_graph.h
        ./load_balance.h
        )

find_package(TBB REQUIRED)

add_executable(load_balance_analysis
        driver.cpp
        load_balance.cpp
        ../graph/dependancy_graph_omp.cpp
        ../timeline/alignment.cpp
        ${HEADER}
        )
target_compile_features(load_balance_analysis PRIVATE cxx_std_17)
target_include_directories(load_balance_analysis PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(load_balance_analysis ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})
target_link_libraries(load_balance_analysis TBB::tbb)

install(TARGETS load_balance_analysis DESTINATION bin)
