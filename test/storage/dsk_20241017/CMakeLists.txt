add_executable(storage_test storage_test.cpp)
target_include_directories(storage_test PUBLIC ../../../lib/pse/include)
target_link_libraries(storage_test PUBLIC FSL ral sql_backend)