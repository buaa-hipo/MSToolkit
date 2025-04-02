. ../../../../run_env.sh

sed -i '1i #include \"instrument/instrumented_func_host.h\"\n#include \"record/mt_callback_defs.h\"' ./simple_mpi.host.cpp

JSI_HOST_INCLUDE_PATH=/thfs3/home/yanghailong/mt3000_programming_env/include:/thfs3/home/yanghailong/xzh/JSI-Toolkit/include:/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include /thfs3/home/yanghailong/xzh/JSI-Toolkit/install/tools/mt_host_instrumenter ./simple_mpi.host.cpp
mv ./simple_mpi.host_expand.cpp.out ./simple_mpi.host_expand.cpp
sed -i '1,2d' ./simple_mpi.host.cpp
