export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/gcc-runtime-12.3.0-vh75fc3delr6nfiunk3vtlxf4zdha7wt/lib/:/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/llvm-18.1.3-ceg5xkdbtl7fomzbshor3xnwx6qaoiyx/lib/
spack load llvm
sed -i '1i #include \"instrument/instrumented_func_dev.h\"\n#include \"record/mt_double_buffer.h\"\n#include \"record/mt_callback_defs.h\"' ./simple_mpi.dev.c
export C_INCLUDE_PATH=$C_INCLUDE_PATH:/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/openmpi-5.0.3-q3r56nciefv25xdqgkqce3irx3od5ye7/include
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/openmpi-5.0.3-q3r56nciefv25xdqgkqce3irx3od5ye7/include
JSI_DEV_INCLUDE_PATH=/root/MT3000_env/mt3000_programming_env/hthreads/include/:/root/xzh/JSI-Toolkit/include:/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/ /root/xzh/JSI-Toolkit/install/tools/mt_dev_instrumenter ./simple_mpi.dev.c -no-func
mv ./simple_mpi.dev_expand.c.out ./simple_mpi.dev_expand.c
sed -i '1,3d' ./simple_mpi.dev.c
