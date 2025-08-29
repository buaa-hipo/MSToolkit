#!/bin/bash
#SBATCH --job-name=xzh_dsp_jsi_test  # 作业名称
#SBATCH --nodes=1                # 使用的节点数
#SBATCH --output=xzh_dsp_jsi.out     # 输出文件

# 加载MPI模块（根据集群情况可能需要调整）
module purge

export LIBRARY_PATH=/thfs3/home/yanghailong/mt3000_programming_env/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=/thfs3/home/yanghailong/mt3000_programming_env/lib:$LD_LIBRARY_PATH
source /thfs3/home/yanghailong/xzh/JSI-Toolkit/install_env.sh
source /thfs3/home/yanghailong/xzh/JSI-Toolkit/run_env.sh

# 运行MPI程序
# JSI_COLLECT_PMU_EVENT= srun jsirun --pmu --backtrace simple_mpi.host
JSI_COLLECT_PMU_EVENT=PAPI_TOT_CYC,PAPI_L1_DCM JSI_COLLECT_DEV_PMU_EVENT=hthread:::CYCLE jsirun --backtrace --pmu --accl -- ./simple_mpi.host