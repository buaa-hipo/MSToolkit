#!/bin/bash

module load cmake mpich/mpi-x

. /vol8/home/njust_ztc/workdir/environment/backup_spack/spack-0.23.1/share/spack/setup-env.sh

export PATH=/vol8/home/njust_ztc/workdir/environment/binutils-2.40/bin:$PATH

export PAPI_PATH=/vol8/home/njust_ztc/workdir/environment/papi
export PKG_CONFIG_PATH=/vol8/home/njust_ztc/workdir/environment/libdwarf/lib/pkgconfig:$PKG_CONFIG_PATH 

export CMAKE_PREFIX_PATH=/vol8/home/njust_ztc/workdir/environment/libdwarf:$CMAKE_PREFIX_PATH
export CMAKE_PREFIX_PATH=/vol8/appsoftware/mpi-x:$CMAKE_PREFIX_PATH

export MPI_CXX_COMPILER=/vol8/appsoftware/mpi-x/bin/mpicxx

export CPATH=/vol8/home/njust_ztc/workdir/environment/libdwarf/include/libdwarf-0:$CPATH

export LIBRARY_PATH=/vol8/home/njust_ztc/workdir/environment/libdwarf/lib:$LIBRARY_PATH
# export LIBRARY_PATH=/vol8/home/njust_ztc/workdir/environment/libdwarf/lib:/thfs3/home/yanghailong/papi/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=/vol8/home/njust_ztc/workdir/environment/libdwarf/lib:$LD_LIBRARY_PATH

export CLANG_INCLUDE_DIR=/vol8/home/njust_ztc/workdir/environment/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-10.3.0/llvm-19.1.3-qdtfwk6mclgsh77tz4qm5vz6kt3ymrkz/include
export CLANG_LIB_DIR=/vol8/home/njust_ztc/workdir/environment/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-10.3.0/llvm-19.1.3-qdtfwk6mclgsh77tz4qm5vz6kt3ymrkz/lib

spack load gcc@12.3.0 dyninst libunwind otf2 boost sqlite range-v3 fmt spdlog magic-enum
