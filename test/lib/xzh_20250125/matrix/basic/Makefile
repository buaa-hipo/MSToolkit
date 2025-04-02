

ENV_ROOT=/root/MT3000_env/mt3000_programming_env/hthreads
GCC_ROOT=/root/MT3000_env/mt3000_programming_env/dsp_compiler

CC=MT-3000-gcc
AR=MT-3000-ar
LD=MT-3000-ld

export LD_LIBRARY_PATH=/root/MT3000_env/mt3000_programming_env/third-party-lib:$LD_LIBRARY_PATH
CFLAGS=-c -O2 -g -gdwarf-2 -fenable-m3000 -ffunction-sections -flax-vector-conversions -I./ -I$(ENV_ROOT)/include -I$(GCC_ROOT)/include/ -I/root/xzh/JSI-Toolkit/include/
LDFLAGS= -L$(ENV_ROOT)/lib --gc-sections -Tdsp.lds

DEV_SRC=daxpy.dev_expand.c
DEV_OBJ=daxpy.dev.o
DEV_EXE=daxpy.dev.out
DEV_DAT=daxpy.dev.dat
HOST_SRC=daxpy.host_expand.cpp
HOST_EXE=daxpy.host_expand

JSI_LIBS=/root/xzh/JSI-Toolkit/install/lib/

DEV_TRACER=/root/xzh/JSI-Toolkit/install/lib/libmt_dev.a
DEV_PMU=/root/xzh/JSI-Toolkit/install/lib/libmtpmu.a

SPDLOG=/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/spdlog-1.14.1-sdjeo5cbhuoishhtvc7y2ad74ykmeh4t/lib/
FMT=/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/fmt-10.2.1-3geqfbtb55ivrmkex5gtmr4gn54rx2ge/lib/
MPI=/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/openmpi-5.0.3-q3r56nciefv25xdqgkqce3irx3od5ye7/lib/

ALL: $(DEV_EXE) $(HOST_EXE)
	$(GCC_ROOT)/bin/MT-3000-makedat -J $(DEV_EXE)

$(HOST_EXE): $(HOST_SRC)
	g++ -O2 -std=c++17 -DMATRIX -o ${HOST_EXE} -I./ -I$(ENV_ROOT)/include -I/root/xzh/JSI-Toolkit/include -L${JSI_LIBS} -L/root/MT3000_env/mt3000_programming_env/hthreads/lib/ -L/root/papi/lib -L/root/libdwarf/lib -L${SPDLOG} -L${FMT} -L${MPI} -ldefault_mt_host_callbacks -lmt_host_tracer -lbacktrace -lFSL -ljsi_record -lpapi -ldwarf -lfmt -lspdlog -lmpi ${HOST_SRC} -lpthread -lhthread_host -g

$(DEV_OBJ): $(DEV_SRC)
	$(GCC_ROOT)/bin/$(CC) $(CFLAGS) $(DEV_SRC) -o $(DEV_OBJ)

$(DEV_EXE): $(DEV_OBJ)
	$(GCC_ROOT)/bin/$(LD) $(LDFLAGS) $(DEV_OBJ) $(GCC_ROOT)/lib/vlib3000.a $(GCC_ROOT)/lib/slib3000.a ${DEV_PMU} ${DEV_TRACER} ${DEV_PMU} /root/MT3000_env/mt3000_programming_env/dsp_compiler/lib/libc.a $(ENV_ROOT)/lib/libhthread_device.a ${DEV_PMU} -o $(DEV_EXE)


clean:
	rm ${DEV_EXE} ${HOST_EXE} ${DEV_OBJ} ${DEV_DAT}


