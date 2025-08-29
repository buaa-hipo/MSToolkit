export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/gcc-runtime-12.3.0-vh75fc3delr6nfiunk3vtlxf4zdha7wt/lib/:/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-12.3.0/llvm-18.1.3-ceg5xkdbtl7fomzbshor3xnwx6qaoiyx/lib/
spack load llvm
sed -i '1i #include \"instrument/instrumented_func_host.h\"\n#include \"record/mt_callback_defs.h\"' ./daxpy.host.cpp
JSI_HOST_INCLUDE_PATH=/root/MT3000_env/mt3000_programming_env/hthreads/include/:/root/xzh/JSI-Toolkit/include /root/xzh/JSI-Toolkit/install/tools/mt_host_instrumenter ./daxpy.host.cpp
mv ./daxpy.host_expand.cpp.out ./daxpy.host_expand.cpp
sed -i '1,2d' ./daxpy.host.cpp
