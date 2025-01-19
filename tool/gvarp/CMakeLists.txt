cmake_minimum_required(VERSION 3.13.0)
project(gvarp LANGUAGES CXX)

find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_DIR} $ENV{LLVM_DIR})

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Verify that the compiler used is the same as in LLVM instrumentation
# Otherwise there might be an ABI incompatibility between our pass and runtime
# library and LLVM.
find_program(LLVM_CLANG "clang" PATHS ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
message(STATUS "Using LLVM clang in ${LLVM_CLANG}")
find_program(LLVM_CLANGXX "clang++" PATHS ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
message(STATUS "Using LLVM clang++ in ${LLVM_CLANGXX}")
find_program(LLVM_OPT "opt" PATHS ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
message(STATUS "Using LLVM opt in ${LLVM_OPT}")
find_program(LLVM_CLANG_OFFLOAD_BUNDLER "clang-offload-bundler" PATHS ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
message(STATUS "Using LLVM clang-offload-bundler in ${LLVM_CLANG_OFFLOAD_BUNDLER}")

include_directories(include)

file(GLOB GVARPAnalysisPassFiles
    "src/llvm-pass/*.cpp"
)
add_library(GVARPAnalysisPass MODULE ${GVARPAnalysisPassFiles})
set_property(TARGET GVARPAnalysisPass PROPERTY CXX_STANDARD 14)
set_target_properties(GVARPAnalysisPass PROPERTIES POSITION_INDEPENDENT_CODE On)
set_target_properties(GVARPAnalysisPass PROPERTIES COMPILE_FLAGS "${PASS_COMPILER_FLAGS} -fno-rtti -g")
set_target_properties(GVARPAnalysisPass PROPERTIES LIBRARY_OUTPUT_DIRECTORY lib)

install(TARGETS GVARPAnalysisPass DESTINATION lib)

configure_file(src/drivers/gvarp-opt.in bin/gvarp-opt @ONLY)
configure_file(src/drivers/gvarp-cc.in bin/gvarp-cc @ONLY)
configure_file(src/drivers/gvarp-c++.in bin/gvarp-c++ @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bin/gvarp-opt 
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE
        DESTINATION bin 
        RENAME gvarp-opt)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bin/gvarp-cc
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE
        DESTINATION bin
        RENAME gvarp-cc)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bin/gvarp-c++ 
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE
        DESTINATION bin 
        RENAME gvarp-c++)
