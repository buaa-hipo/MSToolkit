cmake_minimum_required(VERSION 3.0.0)
project(record_abstraction_layer LANGUAGES CXX)

add_library(ral backend.cpp)


install(TARGETS ral
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)