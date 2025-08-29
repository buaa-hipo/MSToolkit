#!/bin/bash

# 显示帮助信息
print_help() {
    echo "用法: $0 <库路径> <源文件路径>"
    echo "此脚本用于对设备端源代码进行预处理和插桩操作"
    echo "参数:"
    echo "  <库路径>     clang库目录的路径"
    echo "  <源文件路径> 要处理的源文件路径"
    echo "选项:"
    echo "  -h, --help   显示此帮助信息"
    echo "clang库路径示例：/thfs3/home/yanghailong/xzh/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-12.3.0/llvm-19.1.3-yoleutnuscc7z6zbui3cgsvrorpx3lxx/lib"
    echo "需要添加JSI_HOST_INCLUDE_PATH环境变量，示例：/thfs3/home/yanghailong/mt3000_programming_env/include:/thfs3/home/yanghailong/xzh/JSI-Toolkit/include:/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include"
}

# 检查是否提供了足够的参数
if [[ $# -eq 0 ]]; then
    echo "错误: 未提供任何参数" >&2
    print_help
    exit 1
fi

# 处理-h选项
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    print_help
    exit 0
fi

# 检查是否提供了两个参数
if [[ $# -ne 2 ]]; then
    echo "错误: 需要两个参数" >&2
    print_help
    exit 1
fi

# 检查文件/目录是否存在
if [[ ! -d "$1" ]]; then
    echo "错误: 库路径 '$1' 不存在或不是目录" >&2
    exit 1
fi

if [[ ! -f "$2" ]]; then
    echo "错误: 源文件 '$2' 不存在或不是文件" >&2
    exit 1
fi

# 保存原始IFS
OLDIFS=$IFS
IFS=$'\n'

# 执行主要操作
echo "正在处理文件: $2"

# 设置库路径
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$1
export LIBRARY_PATH=$LIBRARY_PATH:$1

echo "已设置LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
echo "已设置LIBRARY_PATH: $LIBRARY_PATH"

# 添加头文件
echo "添加头文件..."
sed -i '1i #include \"instrument/instrumented_func_dev.h\"\n#include \"record/mt_buffer.h\"\n#include \"record/mt_callback_defs.h\"' "$2"

# 获取脚本所在目录
ROOT=$(dirname "$0")
echo "脚本根目录: $ROOT"

# 执行插桩工具
echo "执行插桩工具..."
"$ROOT/mt_dev_instrumenter" "$2"
if [[ $? -ne 0 ]]; then
    echo "错误: 插桩工具执行失败" >&2
    exit 1
fi

# 移除添加的头文件
echo "移除临时头文件..."
sed -i '1,3d' "$2"

# 恢复原始IFS
IFS=$OLDIFS

echo "处理完成!"