#!/bin/bash
#SBATCH --job-name=xzh_qcd_simple_jsi
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=4
#SBATCH --output=xzh_qcd_simple_jsi.out
#SBATCH --partition=thmt1
#SBATCH --reservation=yanghailong
##SBATCH --exclude=cn11718,cn11720

# 加载MPI模块（根据集群情况可能需要调整）
module purge
module load mpich/4.0.2-mpi-x-gcc10.2.0

/thfs3/software/programming_env/mt3000_programming_env/tools/reset_cluster

/thfs3/software/programming_env/mt3000_programming_env/tools/show_dsp_info

export LIBRARY_PATH=/thfs3/home/yanghailong/mt3000_programming_env/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=/thfs3/home/yanghailong/mt3000_programming_env/lib:$LD_LIBRARY_PATH
source /thfs3/home/yanghailong/xzh/JSI-Toolkit-unified/install_env.sh
source /thfs3/home/yanghailong/xzh/JSI-Toolkit-unified/run_env.sh

# 运行MPI程序
# JSI_COLLECT_PMU_EVENT= srun jsirun --pmu --backtrace simple_mpi.host
# JSI_COLLECT_PMU_EVENT=PAPI_TOT_CYC,PAPI_L1_DCM JSI_COLLECT_DEV_PMU_EVENT=hthread:::CYCLE srun jsirun --backtrace --pmu --accl -- ./main
export DSPGCCROOT=/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/
# export JSI_COLLECT_DEV_PMU_EVENT=hthread:::CYCLE,hthread:::BRTK,hthread:::IACK
time srun jsirun --backtrace -o measurement -- ./main
# JSI_COLLECT_DEV_PMU_EVENT=hthread:::CYCLE srun jsirun --backtrace --accl -o measurement -- ./main
# time srun jsirun --accl --backtrace -o measurement_no_pmu -- ./main

