

ENV_ROOT=/thfs3/home/yanghailong/mt3000_programming_env
GCC_ROOT=/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler

CC=MT-3000-gcc
AR=MT-3000-ar
LD=MT-3000-ld

export LD_LIBRARY_PATH=/thfs3/software/programming_env/mt3000_programming_env/third-party-lib:$LD_LIBRARY_PATH
CFLAGS=-c -O2 -g -gdwarf-2 -fenable-m3000 -ffunction-sections -flax-vector-conversions -I./ -I$(ENV_ROOT)/include -I$(GCC_ROOT)/include/
LDFLAGS= -L$(ENV_ROOT)/lib --gc-sections -Tdsp.lds

DEV_SRC=simple_mpi.dev_expand.c
DEV_OBJ=simple_mpi.dev.o
DEV_EXE=simple_mpi.dev.out
DEV_DAT=simple_mpi.dev.dat
HOST_SRC=simple_mpi.host_expand.cpp
HOST_EXE=simple_mpi.host

JSI_LIBS=/thfs3/home/yanghailong/xzh/JSI-Toolkit/install/lib

DEV_TRACER=${JSI_LIBS}/libmt_dev.a
DEV_PMU=${JSI_LIBS}/libmtpmu.a

MT_LIB=${ENV_ROOT}/lib
PAPI_LIB=/thfs3/home/yanghailong/papi/lib
DWARF_LIB=/thfs3/home/yanghailong/libdwarf/lib
FMT_LIB=/thfs3/home/yanghailong/xzh/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-12.3.0/fmt-10.2.1-3rsp7oz2evm2h5epmxrwqazwzyptgrxr/lib
SPDLOG_LIB=/thfs3/home/yanghailong/xzh/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-12.3.0/spdlog-1.14.1-uimdwzxtxiis2uq7xfy7k4g5ry4bdtad/lib

ALL: $(DEV_EXE) $(HOST_EXE)
	$(GCC_ROOT)/bin/MT-3000-makedat -J $(DEV_EXE)

$(HOST_EXE): $(HOST_SRC)
	g++ -O2 -std=c++17 -DMATRIX -o ${HOST_EXE} -I./ -I$(ENV_ROOT)/include -I/thfs3/home/yanghailong/xzh/JSI-Toolkit/include -L${JSI_LIBS} -L${MT_LIB} -L${DWARF_LIB} -L${PAPI_LIB} -L${SPDLOG_LIB} -L${FMT_LIB} -lc -ldefault_mt_host_callbacks -lmt_host_tracer -lbacktrace -lFSL -ljsi_record -lpapi -ldwarf -lfmt -lspdlog ${HOST_SRC} -g -lpthread -lhthread_host -ldefault_mt_host_callbacks -lmt_host_tracer -lbacktrace -lFSL -ljsi_record -lpapi -ldwarf -lfmt -lspdlog

$(DEV_OBJ): $(DEV_SRC)
	$(GCC_ROOT)/bin/$(CC) $(CFLAGS) $(DEV_SRC) -o $(DEV_OBJ)

$(DEV_EXE): $(DEV_OBJ)
	$(GCC_ROOT)/bin/$(LD) $(LDFLAGS) $(DEV_OBJ) $(GCC_ROOT)/lib/vlib3000.a $(GCC_ROOT)/lib/slib3000.a ${DEV_PMU} ${DEV_TRACER} ${DEV_PMU} ${GCC_ROOT}/lib/libc.a $(ENV_ROOT)/lib/libhthread_device.a ${DEV_PMU} -o $(DEV_EXE)


clean:
	rm ${DEV_EXE} ${HOST_EXE} ${DEV_OBJ} ${DEV_DAT}


