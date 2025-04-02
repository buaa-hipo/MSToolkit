#!/bin/bash
#SBATCH --job-name=xzh_dsp_test  # 作业名称
#SBATCH --nodes=2                # 使用的节点数
#SBATCH --output=xzh_mpi_dsp.out     # 输出文件
#SBATCH -t 1:00

# 加载MPI模块（根据集群情况可能需要调整）
module purge

export LIBRARY_PATH=/thfs3/home/yanghailong/mt3000_programming_env/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=/thfs3/home/yanghailong/mt3000_programming_env/lib:$LD_LIBRARY_PATH


# 运行MPI程序
srun simple_mpi.host