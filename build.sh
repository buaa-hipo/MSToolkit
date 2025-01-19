#!/bin/bash
mkdir -p build && cd build
# ARCH_HYGON_C86 only valid for ARCH_HYGON_C86 platform.
# cmake -DCMAKE_INSTALL_PREFIX=`pwd`/../install -DARCH_HYGON_C86=ON -DUSE_ROCM=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# CPU-only
cmake -DCMAKE_INSTALL_PREFIX=`pwd`/../install -DCMAKE_BUILD_TYPE=RelWithDebInfo ..  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

make -j install
