#!/bin/bash
#SBATCH --job-name=xzh_mpi_test  # 作业名称
#SBATCH --nodes=5                # 使用的节点数
#SBATCH --ntasks=80              # 总的MPI进程数
#SBATCH --output=xzh_mpi.out     # 输出文件

# 加载MPI模块（根据集群情况可能需要调整）
module load openmpi

# 运行MPI程序
mpirun ./mpi_example