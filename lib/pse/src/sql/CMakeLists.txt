cmake_minimum_required(VERSION 3.0.0)
project(sqlBackend)

add_library(sql_backend driver.cpp)
target_link_libraries(sql_backend PRIVATE ${SQLITE_LIB})
# target_link_libraries(sql_backend PRIVATE ${PFR_LIB})
message(INFO "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
