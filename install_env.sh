#!/bin/bash

module load cmake mpich/4.0.2-mpi-x-gcc10.2.0 

. /thfs3/home/yanghailong/xzh/spack-0.23.1/share/spack/setup-env.sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/thfs3/software/programming_env/mt3000_programming_env/third-party-lib

export PATH=/thfs3/home/yanghailong/binutils-2.40/bin:$PATH

export PAPI_PATH=/thfs3/home/yanghailong/papi
export PKG_CONFIG_PATH=/thfs3/home/yanghailong/libdwarf/lib/pkgconfig:$PKG_CONFIG_PATH 

export CMAKE_PREFIX_PATH=/thfs3/home/yanghailong/libdwarf:$CMAKE_PREFIX_PATH
export CMAKE_PREFIX_PATH=/thfs3/software/mpich/4.0.2-mpi-x-gcc10.2.0:$CMAKE_PREFIX_PATH

export MPI_CXX_COMPILER=/thfs3/software/mpich/4.0.2-mpi-x-gcc10.2.0/bin/mpicxx

export CPATH=/thfs3/home/yanghailong/libdwarf/include/libdwarf-0:$CPATH

export LIBRARY_PATH=/thfs3/home/yanghailong/libdwarf/lib:$LIBRARY_PATH
# export LIBRARY_PATH=/thfs3/home/yanghailong/libdwarf/lib:/thfs3/home/yanghailong/papi/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=/thfs3/home/yanghailong/libdwarf/lib:$LD_LIBRARY_PATH

export CLANG_INCLUDE_DIR=/thfs3/home/yanghailong/xzh/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-12.3.0/llvm-19.1.3-yoleutnuscc7z6zbui3cgsvrorpx3lxx/include
export CLANG_LIB_DIR=/thfs3/home/yanghailong/xzh/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-12.3.0/llvm-19.1.3-yoleutnuscc7z6zbui3cgsvrorpx3lxx/lib

spack load gcc@12.3.0 dyninst libunwind otf2 boost sqlite range-v3 fmt spdlog magic-enum
