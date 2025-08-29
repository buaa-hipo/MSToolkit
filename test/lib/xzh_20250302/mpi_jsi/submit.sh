#!/bin/bash
#SBATCH --job-name=xzh_mpi_test  # 作业名称
#SBATCH --nodes=5                # 使用的节点数
#SBATCH --ntasks=80              # 总的MPI进程数
#SBATCH --output=xzh_mpi.out     # 输出文件

module purge
module load openmpi

which mpicc
which mpirun
mpirun --version

source /thfs3/home/yanghailong/xzh/JSI-Toolkit/run_env.sh
source /thfs3/home/yanghailong/xzh/JSI-Toolkit/env.sh

JSI_COLLECT_PMU_EVENT=PAPI_TOT_CYC,PAPI_L1_DCM mpirun jsirun -o ./result --backtrace --pmu -- ./mpi_example