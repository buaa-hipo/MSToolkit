ADD_DEFINITIONS(-DDEBUG)
set(CMAKE_CXX_STANDARD 20)

set(JSI_INCLUDE ../../include)
set(JSI_TRACELIB jsi_record)
set(HEADER 
    ${JSI_INCLUDE}/instrument/backtrace.h 
    ${JSI_INCLUDE}/record/record_type.h 
    ${JSI_INCLUDE}/record/record_writer.h 
    ${JSI_INCLUDE}/utils/jsi_log.h 
    ${JSI_INCLUDE}/utils/tsc_timer.h
)
if(LLVMOpenMP_FOUND)
    add_library(openmp_wrapper SHARED ompt_callback.cpp)

    target_include_directories(openmp_wrapper PRIVATE 
        ${JSI_INCLUDE} 
        ./ 
        ${LIBDWARF_INCLUDE_DIRS} 
        ${LIBELF_INCLUDE_DIRS} 
        ${LLVMOpenMP_INCLUDE_DIRS}
        ${OMP_INCLUDE} 
    )

    target_link_libraries(openmp_wrapper 
        ${JSI_TRACELIB}
        ${LIBDWARF_LIBRARIES} 
        ${LIBELF_LIBRARIES} 
        ${LLVMOpenMP_LIBRARIES}
    )

    install(TARGETS openmp_wrapper DESTINATION bin)
else()
    message(STATUS "LLVMOpenMP not found, skipping the compilation of 'my_library'.")
endif()