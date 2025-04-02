#!/bin/bash

module load cmake llvm openmpi

. /thfs3/home/yanghailong/xzh/spack-0.23.1/share/spack/setup-env.sh

export PAPI_PATH=/thfs3/home/yanghailong/papi
export PKG_CONFIG_PATH=/thfs3/home/yanghailong/libdwarf/lib/pkgconfig:$PKG_CONFIG_PATH 
export CMAKE_PREFIX_PATH=/thfs3/home/yanghailong/libdwarf:$CMAKE_PREFIX_PATH 
export CPATH=/thfs3/home/yanghailong/libdwarf/include/libdwarf-0:$CPATH
export LIBRARY_PATH=/thfs3/home/yanghailong/libdwarf/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=/thfs3/home/yanghailong/libdwarf/lib:$LD_LIBRARY_PATH

spack load gcc@12.3.0 dyninst libunwind otf2 boost sqlite range-v3 fmt spdlog magic-enum
