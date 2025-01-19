cmake_minimum_required(VERSION 3.0.0)
project(ProfilingStorageEngine VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fsanitize=undefined -fsanitize=leak ${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-g ${CMAKE_CXX_FLAGS_RELEASE}")

set(CMAKE_COLOR_DIAGNOSTICS ON)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++20 ${CMAKE_CXX_FLAGS} -fopenmp")

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)

set(ENV{PKG_CONFIG_PATH} $prefix)
find_package(PkgConfig)
pkg_search_module(sqlite REQUIRED sqlite3)
# pkg_search_module(Boost REQUIRED Boost)
find_package(Boost REQUIRED)
pkg_search_module(magic_enum REQUIRED magic_enum)
# pkg_search_module(range-v3 REQUIRED range_v3)
find_package(spdlog CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
# find_package(magic_enum REQUIRED)
set(SQLITE_LIB sqlite3)
set(PFR_LIB Boost::boost)
set(BACKWARD_LIB "")

else()

find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS pfr)
find_package(Backward CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
set(SQLITE_LIB unofficial::sqlite3::sqlite3)
set(PFR_LIB Boost::pfr)
set(BACKWARD_LIB Backward::Backward)

endif()

include_directories(./include)
link_libraries(fmt::fmt)
link_libraries(spdlog::spdlog)
# link_libraries(magic_enum)
# link_libraries(Boost)
# include_directories(${Boost_INCLUDE_DIRS})

include_directories(${magic_enum_INCLUDE_DIRS})
get_target_property(RANGE_V3_INCLUDE_DIRS range-v3 INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${RANGE_V3_INCLUDE_DIRS})
# message(INFO "Magic_enum_INCLUDE_DIRS: ${Magic_enum_INCLUDE_DIRS}")
# message(INFO "Magic_enum_INCLUDE_DIRS: ${MAGIC_ENUM_INCLUDE_DIRS}")
# message(INFO "Magic_enum_INCLUDE_DIRS: ${magicenum_INCLUDE_DIRS}")
# message(INFO "Magic_enum_INCLUDE_DIRS: ${magic-enum_INCLUDE_DIRS}")

message(INFO " RANGE V3: ${RANGE_V3_INCLUDE_DIRS}")
message(INFO " RANGE V3: ${RANGE_INCLUDE_DIRS}")
message(INFO " RANGE V3: ${range_INCLUDE_DIRS}")
message(INFO " RANGE V3: ${range_v3_INCLUDE_DIRS}")
message(INFO " RANGE V3: ${Range_v3_INCLUDE_DIRS}")
add_compile_definitions(SPDLOG_COMPILED_LIB)
add_compile_definitions(SPDLOG_SOURCE_LOCATION)
add_compile_options(-fPIC)


add_compile_definitions(PSE_ENABLE_SQLITE_BACKEND)

# add_definitions(-DBACKWARD_HAS_DW=1)
add_subdirectory(src/fsl)
add_subdirectory(src/sql)
add_subdirectory(src/ral)
add_subdirectory(test)
add_subdirectory(tools)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
