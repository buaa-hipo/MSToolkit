#!/bin/bash
#SBATCH --job-name=xzh_qcd  # 作业名称
#SBATCH --nodes=1                # 使用的节点数
#SBATCH --ntasks=4              # 总的MPI进程数
#SBATCH --ntasks-per-node=4
#SBATCH --output=xzh_qcd.out     # 输出文件

# 加载MPI模块（根据集群情况可能需要调整）
module purge
# module load openmpi

srun ./main