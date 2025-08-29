set(JSI_INCLUDE ../../../include)
set(JSI_LIBRARY jsi_reader)

SET(HEADER ${JSI_INCLUDE}/utils/dwarf_line_info.h)

add_executable(dwarf_line_info_dump dwarf_line_info_dump.cpp ${HEADER})
target_compile_features(dwarf_line_info_dump PRIVATE cxx_std_17)
target_include_directories(dwarf_line_info_dump PRIVATE ${JSI_INCLUDE} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS})
target_link_libraries(dwarf_line_info_dump ${JSI_LIBRARY} ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})

install(TARGETS dwarf_line_info_dump DESTINATION bin)
