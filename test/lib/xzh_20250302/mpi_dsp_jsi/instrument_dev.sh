ROOT=$(dirname "$0")

echo $ROOT

# . $ROOT/../../run_env.sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$1
export LIBRARY_PATH=$LIBRARY_PATH:$1
sed -i '1i #include \"instrument/instrumented_func_dev.h\"\n#include \"record/mt_buffer.h\"\n#include \"record/mt_callback_defs.h\"' $1

# JSI_DEV_INCLUDE_PATH=/thfs3/home/yanghailong/mt3000_programming_env/include:/thfs3/home/yanghailong/mstoolkit-from-hn/include:/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include
$ROOT/mt_dev_instrumenter $1
sed -i '1,3d' $1
