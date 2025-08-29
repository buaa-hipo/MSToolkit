export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/gcc-runtime-12.3.0-vh75fc3delr6nfiunk3vtlxf4zdha7wt/lib/:/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/llvm-18.1.3-ceg5xkdbtl7fomzbshor3xnwx6qaoiyx/lib/
spack load llvm
sed -i '1i #include \"instrument/instrumented_func_host.h\"\n#include \"record/mt_callback_defs.h\"' ./simple_mpi.host.cpp
export C_INCLUDE_PATH=$C_INCLUDE_PATH:/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/openmpi-5.0.3-q3r56nciefv25xdqgkqce3irx3od5ye7/include
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/openmpi-5.0.3-q3r56nciefv25xdqgkqce3irx3od5ye7/include
JSI_HOST_INCLUDE_PATH=/root/MT3000_env/mt3000_programming_env/hthreads/include/:/root/xzh/JSI-Toolkit/include /root/xzh/JSI-Toolkit/install/tools/mt_host_instrumenter ./simple_mpi.host.cpp
mv ./simple_mpi.host_expand.cpp.out ./simple_mpi.host_expand.cpp
sed -i '1,2d' ./simple_mpi.host.cpp
