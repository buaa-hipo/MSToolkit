cmake_minimum_required(VERSION 3.0.0)
project(fsl_backend LANGUAGES CXX)

add_library(FSL fsl_lib.cpp fsl_block.cpp)


install(TARGETS FSL 
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)