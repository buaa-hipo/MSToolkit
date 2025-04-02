

ENV_ROOT=/thfs3/home/yanghailong/mt3000_programming_env
GCC_ROOT=/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler

CC=MT-3000-gcc
AR=MT-3000-ar
LD=MT-3000-ld

export LD_LIBRARY_PATH=/thfs3/software/programming_env/mt3000_programming_env/third-party-lib:$LD_LIBRARY_PATH
CFLAGS=-c -O2 -g -gdwarf-2 -fenable-m3000 -ffunction-sections -flax-vector-conversions -I./ -I$(ENV_ROOT)/include -I$(GCC_ROOT)/include/
LDFLAGS= -L$(ENV_ROOT)/lib --gc-sections -Tdsp.lds

DEV_SRC=simple_mpi.dev.c
DEV_OBJ=simple_mpi.dev.o
DEV_EXE=simple_mpi.dev.out
DEV_DAT=simple_mpi.dev.dat
HOST_SRC=simple_mpi.host.cpp
HOST_EXE=simple_mpi.host

# SPDLOG=/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/spdlog-1.14.1-sdjeo5cbhuoishhtvc7y2ad74ykmeh4t/lib/
# FMT=/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/fmt-10.2.1-3geqfbtb55ivrmkex5gtmr4gn54rx2ge/lib/
# MPI=/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/openmpi-5.0.3-q3r56nciefv25xdqgkqce3irx3od5ye7/lib/

ALL: $(DEV_EXE) $(HOST_EXE)
	$(GCC_ROOT)/bin/MT-3000-makedat -J $(DEV_EXE)

$(HOST_EXE): $(HOST_SRC)
	mpic++ -O2 -std=c++17 -DMATRIX -o ${HOST_EXE} -I./ -I$(ENV_ROOT)/include ${HOST_SRC} -L${ENV_ROOT}/lib -lpthread -lhthread_host -g -lmpi

$(DEV_OBJ): $(DEV_SRC)
	$(GCC_ROOT)/bin/$(CC) $(CFLAGS) $(DEV_SRC) -o $(DEV_OBJ)

$(DEV_EXE): $(DEV_OBJ)
	$(GCC_ROOT)/bin/$(LD) $(LDFLAGS) $(DEV_OBJ) $(GCC_ROOT)/lib/vlib3000.a $(GCC_ROOT)/lib/slib3000.a ${GCC_ROOT}/lib/libc.a $(ENV_ROOT)/lib/libhthread_device.a ${GCC_ROOT}/lib/libc.a -o $(DEV_EXE)


clean:
	rm ${DEV_EXE} ${HOST_EXE} ${DEV_OBJ} ${DEV_DAT}


