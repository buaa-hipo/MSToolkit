#!/bin/bash

export OMP_NUM_THREADS=$(nproc)
# 默认值设定
program="test_program"
name="graph"
start=0
end=0
backtrace_input_dir=""
lineinfo_dir=""  # 新增lineinfo文件参数
mode="run"  # 新增模式，默认是run
use_omp=true

# 参数解析
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -p|--program) program="$2"; shift ;;
        -i|--trace_dir) trace_dir="$2"; shift ;;
        -o|--output_dir) output_dir="$2"; shift ;;
        -n|--name) name="$2"; shift ;;
        -d|--duration) duration="$2"; shift ;;
        -u|--interval) interval="$2"; shift ;;  # 修改了interval参数的标志符
        --start) start="$2"; shift ;;
        --end) end="$2"; shift ;;
        -b|--backtrace) backtrace_input_dir="$2"; shift ;;  # 使用lineinfo_file代替dump_dir
        -m|--mode) mode="$2"; shift ;;  # 新增对-m参数的支持
        -l|--lineinfo_dir) lineinfo_dir="$2"; shift ;;  # 新增lineinfo文件参数
        *) echo "未知参数: $1"; exit 1 ;;
    esac
    shift
done

echo $backtrace_input_dir
echo $lineinfo_dir

# 检查必需的输入目录是否存在
if [ ! -d "$trace_dir" ]; then
    echo "错误: 输入目录 '$trace_dir' 不存在."
    exit 1
fi

if [ -n "$backtrace_input_dir" ] && [ -n "$lineinfo_dir" ]; then
    if [ ! -d "$lineinfo_dir" ]; then
        echo "错误: LINEINFO文件 '$lineinfo_dir' 不存在."
        exit 1
    fi
    if [ ! -f "$backtrace_input_dir" ]; then
        echo "错误: BACKTRACE输入文件 '$backtrace_input_dir' 不存在."
        exit 1
    fi
fi

# 计算输出路径
output_subdir="${output_dir}/${program}/$((duration))_$((interval))"

# 创建输出目录（如果它还不存在）
mkdir -p "$output_subdir"

# 根据模式构建命令
cmd_prefix=""
if [ "$mode" == "run" ]; then
    cmd_prefix="time"
elif [ "$mode" == "gdb" ]; then
    cmd_prefix="gdb -args"
else
    echo "错误: 未知的模式 '$mode'. 使用 'run' 或 'gdb'."
    exit 1
fi

# 构建完整的命令字符串
command="$cmd_prefix timeslice_analysis_all -i \"$trace_dir\" -f -o \"${output_subdir}/${name}\" -d \"$lineinfo_dir\" -l \"$((interval * 2500000 ))\" -u \"$((duration * 2500000 ))\" ${start:+--start \"$start\"} ${end:+--end \"$end\"} ${backtrace_input_dir:+-b} ${backtrace_input_dir:+-s \"$backtrace_input_dir\"}" 


# 打印最终将执行的命令
echo "即将执行的命令为: $command"

# 执行timeslice_analysis_all命令
eval "$command"