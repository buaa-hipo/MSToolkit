# SET(CMAKE_C_COMPILER ${MPI_C_COMPILER})
# SET(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})

set(JSI_INCLUDE ../../include)
set(JSI_LIBRARY jsi_compress)

SET(HEADER ${JSI_INCLUDE}/compress/compress.h ${JSI_INCLUDE}/compress/snappy_compress.h)

add_executable(jsidecompress jsidecompress.cpp ${HEADER})
target_compile_features(jsidecompress PRIVATE cxx_std_17)
target_include_directories(jsidecompress PRIVATE ${JSI_INCLUDE} ${LIBUNWIND_INCLUDE_DIR} ./ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
target_link_libraries(jsidecompress ${JSI_LIBRARY} ${MPI_LIBRARIES})

install(TARGETS jsidecompress DESTINATION bin)
