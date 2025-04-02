#!/bin/bash
#SBATCH --job-name=xzh_mpi_dsp_jsi_test  # 作业名称
#SBATCH --nodes=1                # 使用的节点数
#SBATCH --ntasks=4              # 总的MPI进程数
#SBATCH --ntasks-per-node=4
#SBATCH --output=xzh_mpi_dsp_jsi.out     # 输出文件

# 加载MPI模块（根据集群情况可能需要调整）
module purge
module load openmpi

export LIBRARY_PATH=/thfs3/home/yanghailong/mt3000_programming_env/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=/thfs3/home/yanghailong/mt3000_programming_env/lib:$LD_LIBRARY_PATH
source /thfs3/home/yanghailong/xzh/JSI-Toolkit/install_env.sh
source /thfs3/home/yanghailong/xzh/JSI-Toolkit/run_env.sh

# 运行MPI程序
# JSI_COLLECT_PMU_EVENT= srun jsirun --pmu --backtrace simple_mpi.host
JSI_COLLECT_PMU_EVENT=PAPI_TOT_CYC,PAPI_L1_DCM JSI_COLLECT_DEV_PMU_EVENT=hthread:::CYCLE srun jsirun --backtrace --pmu --accl -- ./simple_mpi.host
# JSI_COLLECT_DEV_PMU_EVENT=hthread:::CYCLE srun jsirun --backtrace --accl -- ./simple_mpi.host
