#!/bin/bash
#SBATCH --job-name=xzh_qcd_original  # 作业名称
#SBATCH --nodes=5                # 使用的节点数
#SBATCH --ntasks=20              # 总的MPI进程数
#SBATCH --ntasks-per-node=4
#SBATCH --output=xzh_qcd_orginal.out
#SBATCH --partition=thmt1

module purge
module load mpich/4.0.2-mpi-x-gcc10.2.0


time srun ./main
