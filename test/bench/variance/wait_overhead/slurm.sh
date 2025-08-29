NTASKS=2
#!/bin/bash
JOBNAME="waitbench"

echo "TASK $JOBNAME TEST START NTASK=$NTASKS "
nowdate=$(date +%Y_%m_%d_%H_%M_%S)
echo $nowdate
sbatch << END
#!/bin/bash
#SBATCH -J $JOBNAME
#SBATCH -o waitbench-$NTASKS-%j-$nowdate.log
#SBATCH -e waitbench-$NTASKS-%j-$nowdate.err
#SBATCH -p debug 
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-node=4
#SBATCH -n $NTASKS
export JSI_BACKTRACE_MAX_DEPTH=5
export JSI_COLLECT_PMU_EVENT=PAPI_TOT_INS,PAPI_L1_DCA
/usr/bin/time -v mpirun -n $NTASKS ./mpi_wait_bench

END