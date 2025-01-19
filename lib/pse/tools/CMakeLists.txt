cmake_minimum_required(VERSION 3.0.0)
project(toosl)

add_executable(sls sls.cpp)
target_link_libraries(sls PUBLIC FSL ${BACKWARD_LIB})