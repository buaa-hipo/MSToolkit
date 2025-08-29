#!/bin/bash
#SBATCH -o /thfs3/home/yanghailong/workdir/NCFX-TEST/misa-md/logs/plot/plot_1.log
#SBATCH -e /thfs3/home/yanghailong/workdir/NCFX-TEST/misa-md/logs/plot/plot_1.err
#SBATCH -J misa-md-plot-job
#SBATCH -p thmt1
#SBATCH --nodes=100
#SBATCH --ntasks-per-node=1
#SBATCH --ntasks=100
#SBATCH --cpus-per-task=1


echo "作业启动时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "作业ID: $SLURM_JOB_ID"
echo "节点信息: $SLURM_NODELIST"
echo "分配核心数: $SLURM_CPUS_PER_TASK"
module load  mpi4py/3.1.3-py3.8-gcc9.3.0-mpi-x

source /thfs3/home/yanghailong/workdir/NCFX-TEST/env/bin/activate

# 检查依赖是否安装
# pip list | grep -E "matplotlib|numpy|pandas|mpi4py" || {
#     echo "错误 未安装必要的Python库"
#     exit 1
# }

CSV_DIR="/thfs3/home/yanghailong/workdir/NCFX-TEST/misa-md/logs/analysis"  # CSV输入目录
IMAGE_DIR="/thfs3/home/yanghailong/workdir/NCFX-TEST/misa-md/logs/image"      # 图片输出目录

# 创建输出目录
mkdir -p "$IMAGE_DIR"

# 运行Python脚本
srun -n $SLURM_NTASKS python /thfs3/home/yanghailong/workdir/NCFX-TEST/draw_mpi.py "$CSV_DIR" "$IMAGE_DIR"

echo "作业完成时间: $(date '+%Y-%m-%d %H:%M:%S')"    