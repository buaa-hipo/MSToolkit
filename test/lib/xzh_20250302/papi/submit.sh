#!/bin/bash
#SBATCH --job-name=xzh_papi_test
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --output=xzh_papi.out

export LD_LIBRARY_PATH=/thfs3/home/yanghailong/papi/lib:$LD_LIBRARY_PATH
export PATH=/thfs3/home/yanghailong/papi/bin:$PATH

/thfs3/home/yanghailong/xzh/JSI-Toolkit/test/lib/xzh_20250302/papi/papi_test
papi_avail