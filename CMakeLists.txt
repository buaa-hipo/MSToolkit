cmake_minimum_required(VERSION 3.8)
project(JSIToolkit)

option(USE_ROCM                "Enable ROCM tracing & profiling"                OFF)
option(USE_LIBUNWIND           "Enable Libunwind backtracing"                   ON)
option(USE_ALLOCA              "Enable GNU C extension alloca"                  ON)

set(CXX_LEAST_VERSION 12.3)
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${CXX_LEAST_VERSION})
    message(FATAL_ERROR "C++ Compiler version must be at least ${CXX_LEAST_VERSION}")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++20")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
try_compile(HAS_FS "${CMAKE_BINARY_DIR}/temp" 
"${CMAKE_SOURCE_DIR}/cmake/tests/has_filesystem.cc" 
            CMAKE_FLAGS -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON
            LINK_LIBRARIES stdc++fs)
if(HAS_FS)
    message(STATUS "Compiler has filesystem support")
else()
#   .... You could also try searching for boost::filesystem here.
    message(FATAL_ERROR "Compiler is missing filesystem capabilities")
endif(HAS_FS)

EXECUTE_PROCESS(COMMAND uname -a
        TIMEOUT 5
	OUTPUT_VARIABLE OS_TYPE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
MESSAGE(STATUS "BUILD OS_TYPE ${OS_TYPE}")

if (OS_TYPE MATCHES ".*oe.*")
  message(STATUS "Building for OS:openEuler")
  add_compile_definitions(EULER)
endif()

if (OS_TYPE MATCHES ".*ky.*")
  message(STATUS "Building for OS:kylin")
  add_compile_definitions(KYLIN)
endif()

if (USE_ALLOCA)
  add_compile_definitions(USE_ALLOCA)
endif()

if (NOT NO_BACKTRACE)
    add_compile_definitions(ENABLE_BACKTRACE)
    if (USE_LIBUNWIND)
        add_compile_definitions( USE_LIBUNWIND )
        find_package(Libunwind REQUIRED)
    endif()
    message(STATUS "BACKTRACE ENABLED")
endif ()

if (NOT NO_PMU)
    add_compile_definitions(ENABLE_PMU)
    message(STATUS "PMU COLLECTING ENABLED")
endif ()

if (ARCH_HYGON_C86)
    add_compile_definitions(HYGON_C86)
endif ()

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
# generate headers from scripts
set(GENERATE_WRAPPER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_scripts/compile_type_defines.py")
message(STATUS "Generating wrapper headers: ${GENERATE_WRAPPER_SCRIPT}")
execute_process(
  COMMAND ${Python3_EXECUTABLE} ${GENERATE_WRAPPER_SCRIPT} --workdir=${CMAKE_CURRENT_SOURCE_DIR}/
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _output
  ERROR_VARIABLE _error
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT _result EQUAL 0)
  message(FATAL_ERROR "Failed to execute Python script: ${_error}")
else()
  message(STATUS "Wrapper Generate Success")
endif()

find_package(OTF2 REQUIRED)
find_package(MPI REQUIRED)
# find_package(Dyninst REQUIRED)
find_package(PAPI REQUIRED)
find_package (LibElf REQUIRED)
find_package (LibDwarf REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)

set(ENV{PKG_CONFIG_PATH} $prefix)
find_package(PkgConfig)
pkg_search_module(magic_enum REQUIRED magic_enum)
pkg_search_module(sqlite REQUIRED sqlite3)
include_directories(${sqlite_INCLUDE_DIRS})
link_directories(${sqlite_LIBRARY_DIRS})


include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

if (USE_ROCM)
  set(CMAKE_MODULE_PATH "$ENV{ROCM_PATH}/hip/cmake" ${CMAKE_MODULE_PATH})
  find_package(HIP   REQUIRED)
  find_package(ROCM REQUIRED)
  if(NOT ROCM_PATH OR ROCM_PATH STREQUAL "")
      if(DEFINED ENV{ROCM_PATH} AND NOT $ENV{ROCM_PATH} STREQUAL "")
          set(ROCM_PATH $ENV{ROCM_PATH})
      else()
          set(ROCM_PATH ${HIP_ROOT_DIR})
      endif()
  endif()
  add_compile_definitions( ROCM )
  message(STATUS "ROCM PATH: " ${ROCM_PATH})
endif ()

include_directories(./lib/pse/include/)

add_subdirectory(lib)
add_subdirectory(tool)

# Test
# add_subdirectory(test)
