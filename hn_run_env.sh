#!/bin/bash

# . /vol8/home/njust_ztc/workdir/environment/spack-0.23.1/share/spack/setup-env.sh

source /vol8/home/njust_ztc/workdir/environment/papi/env.sh
export CPATH=/vol8/home/njust_ztc/workdir/environment/libdwarf/include/libdwarf-0:$CPATH
export LIBRARY_PATH=/vol8/home/njust_ztc/workdir/environment/libdwarf/lib:/vol8/home/njust_ztc/workdir/environment/papi/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=/vol8/home/njust_ztc/workdir/environment/libdwarf/lib:/vol8/home/njust_ztc/workdir/environment/papi/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/vol8/home/njust_ztc/workdir/environment/backup_spack/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-12.3.0/fmt-10.2.1-3rsp7oz2evm2h5epmxrwqazwzyptgrxr/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/vol8/home/njust_ztc/workdir/environment/backup_spack/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-12.3.0/spdlog-1.14.1-uimdwzxtxiis2uq7xfy7k4g5ry4bdtad/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/vol8/home/njust_ztc/workdir/environment/backup_spack/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-12.3.0/gcc-runtime-12.3.0-ndnfj35mzk2uepl2xexfd4gte26fr33z/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/vol8/home/njust_ztc/workdir/environment/backup_spack/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-12.3.0/libunwind-1.7.2-6brbmkcazwvonhh3ilhx2whzgjrwjslg/lib:$LD_LIBRARY_PATH

source /vol8/home/njust_ztc/workdir/mstoolkit-new/env.sh

# spack load gcc@12.3.0 dyninst libunwind otf2 boost sqlite range-v3 fmt spdlog magic-enum

# 配置spack load行为的方法
# LD_LIBRARY_PATH is no longer set by default by spack load or module loads. Setting LD_LIBRARY_PATH in Spack environments/modules can cause binaries from outside of Spack to crash, and Spack's own builds use RPATH and do not need LD_LIBRARY_PATH set in order to run. If you still want the old behavior, you can run these commands to configure Spack to set LD_LIBRARY_PATH: spack config add modules:prefix_inspections:lib64:[LD_LIBRARY_PATH] spack config add modules:prefix_inspections:lib:[LD_LIBRARY_PATH]

# 也许spack导出module文件用module来管理？