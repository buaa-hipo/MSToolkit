cmake_minimum_required(VERSION 3.0.0)
project(test)

add_executable(fsl_test fsl_test.cpp)
target_link_libraries(fsl_test PUBLIC FSL ${BACKWARD_LIB})

add_executable(fsl_multi_section_test fsl_multi_section_test.cpp)
target_link_libraries(fsl_multi_section_test PUBLIC FSL ${BACKWARD_LIB})

add_executable(fsl_fork_test fsl_fork_test.cpp)
target_link_libraries(fsl_fork_test PUBLIC FSL ${BACKWARD_LIB})

add_executable(fsl_interface_test fsl_interface_test.cpp)
target_link_libraries(fsl_interface_test PUBLIC FSL ${BACKWARD_LIB})

add_executable(ral_section_test ral_section_test.cpp)
target_link_libraries(ral_section_test PUBLIC ral FSL sql_backend ${BACKWARD_LIB})

add_executable(sql_interface_test sql_interface_test.cpp)
target_link_libraries(sql_interface_test PUBLIC sql_backend ${BACKWARD_LIB})

add_executable(fsl_large_file_test fsl_large_file_test.cpp)
target_link_libraries(fsl_large_file_test PUBLIC FSL)
target_link_libraries(fsl_large_file_test PUBLIC ${BACKWARD_LIB})

add_executable(sql_test sql_test.cpp)
target_link_libraries(sql_test PUBLIC sql_backend ${BACKWARD_LIB})

add_executable(typename_test typename_test.cpp)

add_executable(flock_test flock_test.cpp)

add_executable(atomic_test atomic_test.cpp)

add_executable(fsl_mt_large_test fsl_mt_large_test.cpp)
target_link_libraries(fsl_mt_large_test PUBLIC FSL ${BACKWARD_LIB})

add_executable(iterator_test iterator_test.cpp)
target_link_libraries(iterator_test PUBLIC FSL ${BACKWARD_LIB} ral sql_backend)
add_executable(ranges_test ranges_test.cpp)
target_link_libraries(ranges_test PUBLIC FSL ${BACKWARD_LIB} ral sql_backend)
add_executable(regroup_test regroup_test.cpp)
target_link_libraries(regroup_test PUBLIC FSL ${BACKWARD_LIB} ral sql_backend)
add_executable(parallel_range_algs_for_vec parallel_range_algs_for_vec.cpp)
target_link_libraries(parallel_range_algs_for_vec PUBLIC FSL ${BACKWARD_LIB} ral sql_backend)
add_executable(view_utils_test view_utils_test.cpp)
target_link_libraries(view_utils_test PUBLIC FSL ${BACKWARD_LIB} ral sql_backend)

add_executable(extractor_iter_test extractor_iter_test.cpp)
target_link_libraries(extractor_iter_test PUBLIC FSL ${BACKWARD_LIB} ral sql_backend)
add_executable(detect_field_test detect_field_test.cpp)
target_link_libraries(detect_field_test PUBLIC ${BACKWARD_LIB}  )

add_executable(new_backend_test new_backend_test.cpp)
target_link_libraries(new_backend_test PUBLIC FSL ${BACKWARD_LIB} ral sql_backend)

add_executable(dir_iterator_test dir_iterator_test.cpp)
target_link_libraries(dir_iterator_test PUBLIC FSL ${BACKWARD_LIB} ral sql_backend)

add_executable(new_extractor_test new_extractor_test.cpp)
target_link_libraries(new_extractor_test PUBLIC FSL ${BACKWARD_LIB} ral sql_backend)
# include(Catch)
# catch_discover_tests(iterator_test)