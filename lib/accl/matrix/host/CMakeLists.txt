cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)

# find_package(fmt CONFIG REQUIRED)
# link_libraries(fmt::fmt)
# get_target_property(fmt_INCLUDE_DIRS fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
# message(INFO "fmt_INCLUDE_DIRS: ${fmt_INCLUDE_DIRS}")

# include_directories(${fmt_INCLUDE_DIRS})

find_package(spdlog CONFIG REQUIRED)
link_libraries(spdlog::spdlog_header_only)
# link_libraries(spdlog::spdlog)
# get_target_property(spdlog_INCLUDE_DIRS spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
# message(INFO "spdlog_INCLUDE_DIRS: ${spdlog_INCLUDE_DIRS}")
# include_directories(${spdlog_INCLUDE_DIRS})

set(MATRIX_INCLUDE_DIR ../../../../include)

set(MT_ENV_ROOT /thfs3/software/programming_env/mt3000_programming_env)
set(MT_HTHREADS_ROOT ${MT_ENV_ROOT}/hthreads)
set(NEW_HTHREAD_LIB_DIR /thfs3/home/yanghailong/mt3000_programming_env/lib)

set(ACCL_TRACER accl_tracer.cpp)
set(ACCL_MEM_POOL_SRC mt_memory_pool.cpp)
set(ACCL_CALLBACKS mt_tracer_host_callbacks.cpp)
set(ACCL_HOST_INSTRUMENT instrumented_func_host.cpp)

add_library(mt_host_tracer SHARED ${ACCL_TRACER} ${ACCL_MEM_POOL_SRC} ${ACCL_HOST_INSTRUMENT})
add_library(default_mt_host_callbacks SHARED ${ACCL_CALLBACKS})

target_compile_features(mt_host_tracer PRIVATE cxx_std_20)
target_compile_features(default_mt_host_callbacks PRIVATE cxx_std_20)

target_include_directories(mt_host_tracer PRIVATE ${MATRIX_INCLUDE_DIR} ./ ${MT_HTHREADS_ROOT}/include/ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
target_include_directories(default_mt_host_callbacks PRIVATE ${MATRIX_INCLUDE_DIR} ./ ${MT_HTHREADS_ROOT}/include/ ${LIBDWARF_INCLUDE_DIRS} ${LIBELF_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})

target_link_libraries(mt_host_tracer jsi_record ${NEW_HTHREAD_LIB_DIR}/libhthread_host.so ${LIBDWARF_LIBRARIES} ${LIBELF_LIBRARIES})

# target_compile_options(mt_host_tracer PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g)
# target_compile_options(default_mt_host_callbacks PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g)

install(TARGETS mt_host_tracer default_mt_host_callbacks DESTINATION lib)

