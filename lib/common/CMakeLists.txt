add_library(backtrace SHARED backtrace.cpp dwarf_line_info.cpp)
target_include_directories(backtrace PRIVATE ../../include ./ ${LIBUNWIND_INCLUDE_DIR} ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
target_link_libraries(backtrace PUBLIC FSL ral sql_backend ${LIBDWARF_LIBRARIES} ${LIBUNWIND_LIBRARIES})

add_library(record_meta SHARED record_meta.cpp)
target_include_directories(record_meta PRIVATE ../../include ./ ${MPI_INCLUDE_PATH})
target_link_libraries(record_meta PUBLIC FSL ral sql_backend)

install(TARGETS backtrace DESTINATION lib)
install(TARGETS record_meta DESTINATION lib)
