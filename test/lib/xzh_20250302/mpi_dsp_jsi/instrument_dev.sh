. ../../../../run_env.sh
sed -i '1i #include \"instrument/instrumented_func_dev.h\"\n#include \"record/mt_double_buffer.h\"\n#include \"record/mt_callback_defs.h\"' ./simple_mpi.dev.c

JSI_DEV_INCLUDE_PATH=/thfs3/home/yanghailong/mt3000_programming_env/include:/thfs3/home/yanghailong/xzh/JSI-Toolkit/include:/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include /thfs3/home/yanghailong/xzh/JSI-Toolkit/install/tools/mt_dev_instrumenter ./simple_mpi.dev.c
mv ./simple_mpi.dev_expand.c.out ./simple_mpi.dev_expand.c
sed -i '1,3d' ./simple_mpi.dev.c
