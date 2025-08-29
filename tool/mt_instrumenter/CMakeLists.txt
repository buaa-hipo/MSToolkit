
cmake_minimum_required(VERSION 3.8)

project(mt_instrumenter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(mt_dev_instrumenter dev_instrumenter.cpp)
add_executable(mt_host_instrumenter host_instrumenter.cpp)

target_compile_options(mt_dev_instrumenter PRIVATE
        -fno-exceptions
        -fno-rtti
        -D_GNU_SOURCE
        -D__STDC_CONSTANT_MACROS
        -D__STDC_FORMAT_MACROS
        -D__STDC_LIMIT_MACROS
)

target_compile_options(mt_host_instrumenter PRIVATE
        -fno-exceptions
        -fno-rtti
        -D_GNU_SOURCE
        -D__STDC_CONSTANT_MACROS
        -D__STDC_FORMAT_MACROS
        -D__STDC_LIMIT_MACROS
)

target_link_libraries(mt_host_instrumenter PRIVATE
        pthread
        m
        z
)

target_link_libraries(mt_dev_instrumenter PRIVATE
        pthread
        m
        z
)

# 查找 Clang 头文件目录
find_path(CLANG_INCLUDE_DIR
    NAMES clang-c/Index.h
    HINTS
        /usr/include
        /usr/local/include
        /opt/local/include
        /sw/include
        ENV CLANG_INCLUDE_DIR
)

# 查找 Clang 库文件
find_path(CLANG_LIBRARY_DIR
    NAMES libclang.so
    HINTS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
        ENV CLANG_LIB_DIR
)

# 检查是否找到 Clang 头文件和库文件
if(CLANG_INCLUDE_DIR AND CLANG_LIBRARY_DIR)
    set(CLANG_FOUND TRUE)
    message(STATUS "Found Clang include directory: ${CLANG_INCLUDE_DIR}")
    message(STATUS "Found Clang library: ${CLANG_LIBRARY_DIR}")
    # 将 Clang 头文件目录添加到包含路径
    include_directories(${CLANG_INCLUDE_DIR})
    set(CLANG_LIBS ${CLANG_LIBRARY_DIR}/libclang.so ${CLANG_LIBRARY_DIR}/libclang-cpp.so)
    target_link_libraries(mt_host_instrumenter PRIVATE ${CLANG_LIBS})
    target_link_libraries(mt_dev_instrumenter PRIVATE ${CLANG_LIBS})
else()
    set(CLANG_FOUND FALSE)
    message(FATAL_ERROR "Could not find Clang.")
endif()

# file(GLOB CLANG_LIBS "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/llvm-18.1.3-ceg5xkdbtl7fomzbshor3xnwx6qaoiyx/lib/libclang*")
# set(CLANG_LIBS "${CLANG_LIB}/libclang.so ${CLANG_LIB}/libclang-cpp.so")
# message(STATUS "LINKING - ")
# message(STATUS ${CLANG_LIBS})

# if(CLANG_LIBS)
#     target_link_libraries(mt_host_instrumenter PRIVATE ${CLANG_LIBS})
#     target_link_libraries(mt_dev_instrumenter PRIVATE ${CLANG_LIBS})
# else()
#     message(FATAL_ERROR "Could not find clang library")
# endif()

# target_compile_options(mt_dev_instrumenter PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g)
# target_compile_options(mt_host_instrumenter PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g)

install(TARGETS mt_host_instrumenter DESTINATION bin)
install(TARGETS mt_dev_instrumenter DESTINATION bin)
install(PROGRAMS instrument_dev.sh DESTINATION bin RENAME instrument_dev)
install(PROGRAMS instrument_host.sh DESTINATION bin RENAME instrument_host)