add_library(backtrace SHARED backtrace.cpp dwarf_line_info.cpp)
target_include_directories(backtrace PRIVATE ../../include ./ ${LIBUNWIND_INCLUDE_DIR} ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH} ${LLVMOpenMP_INCLUDE_DIRS})
target_link_libraries(backtrace PUBLIC FSL ral sql_backend ${LIBDWARF_LIBRARIES} ${LIBUNWIND_LIBRARIES} ${LLVMOpenMP_LIBRARIES})

add_library(record_meta SHARED record_meta.cpp)
target_include_directories(record_meta PRIVATE ../../include ./ ${MPI_INCLUDE_PATH})
target_link_libraries(record_meta PUBLIC FSL ral sql_backend)

add_library(unified_pmu_collector SHARED pmu_unified.c)
target_include_directories(unified_pmu_collector PRIVATE ../../include ${PAPI_INCLUDE_DIRS})
target_link_libraries(unified_pmu_collector ${PAPI_LIBRARIES})

install(TARGETS backtrace DESTINATION lib)
install(TARGETS record_meta DESTINATION lib)
install(TARGETS unified_pmu_collector DESTINATION lib)
