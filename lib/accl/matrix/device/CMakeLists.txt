
set(MT_ENV_ROOT /thfs3/software/programming_env/mt3000_programming_env)
set(MT_DSP_ROOT ${MT_ENV_ROOT}/dsp_compiler)
set(MT_HTHREADS_ROOT ${MT_ENV_ROOT}/hthreads)

include_directories(../../../../include/)
include_directories(${MT_HTHREADS_ROOT}/include/)
include_directories(${MT_DSP_ROOT}/include/)

message(STATUS "MT_DSP_ROOT=${MT_DSP_ROOT}")
message(STATUS "MT_HTHREADS_ROOT=${MT_HTHREADS_ROOT}")
message(STATUS "Install directory: ${CMAKE_INSTALL_PREFIX}")

set(CMAKE_C_COMPILER ${MT_DSP_ROOT}/bin/MT-3000-gcc)
set(CMAKE_AR ${MT_DSP_ROOT}/bin/MT-3000-ar)
set(CMAKE_LINKER ${MT_DSP_ROOT}/bin/MT-3000-ld)

set(SOURCES instrumented_func_dev.c mt_double_buffer.c mt_tracer_dev_callbacks.c)

if (NOT NO_BACKTRACE)
    set(USE_BT "-DENABLE_BACKTRACE")
endif()

# add_custom_command(
#     OUTPUT MT_PMU_collector.o
#     COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/thfs3/software/programming_env/mt3000_programming_env/third-party-lib ${CMAKE_C_COMPILER} -c -O0 -Wall -g -gdwarf-2 -fenable-m3000 -ffunction-sections -flax-vector-conversions -I../../../../include -I${MT_HTHREADS_ROOT}/include/ -I${MT_DSP_ROOT}/include -o MT_PMU_collector.o MT_PMU_collector.c
#     DEPENDS MT_PMU_collector.c 
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#     COMMENT "Compiling PMU collector"
# )

add_custom_target(
    mtpmu
    # COMMAND sh -c pwd
    COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/thfs3/software/programming_env/mt3000_programming_env/third-party-lib ${CMAKE_C_COMPILER} -c -O0 -Wall -g -gdwarf-2 -fenable-m3000 -ffunction-sections -flax-vector-conversions ${USE_BT} -I../../../../include -I${MT_HTHREADS_ROOT}/include/ -I${MT_DSP_ROOT}/include -o MT_PMU_collector.o MT_PMU_collector.c
    COMMAND ${CMAKE_AR} rcs libmtpmu.a MT_PMU_collector.o
    # COMMAND rm ${CMAKE_CURRENT_SOURCE_DIR}/MT_PMU_collector.o
    DEPENDS
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compiling PMU collector"
    COMMENT "Making static library - libmtpmu.a"
)

set(OBJECT_FILES)

foreach(SRC_FILE ${SOURCES})
    get_filename_component(OBJ_FILE ${SRC_FILE} NAME_WE)
    add_custom_command(
        OUTPUT ${OBJ_FILE}.o
        COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/thfs3/software/programming_env/mt3000_programming_env/third-party-lib ${CMAKE_C_COMPILER} -c -O0 -Wall -g -gdwarf-2 -fenable-m3000 -ffunction-sections -flax-vector-conversions ${USE_BT} -I../../../../include -I${MT_HTHREADS_ROOT}/include/ -I${MT_DSP_ROOT}/include -L./ -o ${OBJ_FILE}.o ${SRC_FILE}
        DEPENDS mtpmu
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compiling ${OBJ_FILE}"
    )
    list(APPEND OBJECT_FILES ${OBJ_FILE}.o)
endforeach()

add_custom_command(
    OUTPUT libmt_dev.a
#     COMMAND sh -c pwd
#     COMMAND sh -c ls
    COMMAND ${CMAKE_COMMAND} -E env ${CMAKE_AR} rcs libmt_dev.a ${OBJECT_FILES} libmtpmu.a
    COMMAND rm ${OBJECT_FILES}
    DEPENDS ${OBJECT_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(mt_dev_tracer ALL DEPENDS libmt_dev.a)
install(FILES libmt_dev.a DESTINATION lib)
install(FILES libmtpmu.a DESTINATION lib)