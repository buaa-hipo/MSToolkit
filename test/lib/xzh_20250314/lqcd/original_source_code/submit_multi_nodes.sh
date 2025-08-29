#!/bin/bash
#SBATCH --job-name=xzh_qcd_no_mem  # 作业名称
#SBATCH --nodes=5                # 使用的节点数
#SBATCH --ntasks=20              # 总的MPI进程数
#SBATCH --ntasks-per-node=4
#SBATCH --output=xzh_qcd_no_mem.out     # 输出文件
#SBATCH --partition=thmt1

# 加载MPI模块（根据集群情况可能需要调整）
module purge
module load mpich/4.0.2-mpi-x-gcc10.2.0

source /thfs3/home/yanghailong/xzh/JSI-Toolkit-unified/install_env.sh
source /thfs3/home/yanghailong/xzh/JSI-Toolkit-unified/run_env.sh

time srun jsirun --backtrace -- ./main
