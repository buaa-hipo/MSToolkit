#!/bin/bash

die() {
    local message=$1
    echo -e "$message \e[31m [FAILED] \e[0m" >&2
    exit 1
}

ok() {
    local message=$1
    echo -e "$message \e[32m PASSED \e[0m"
}

JSI_ROOT=../../../../

echo "# Running tests for variance_analysis"
# Step 1
echo "=== Preparing test programs ==="
make clean && make \
    && ok MAKE || die MAKE
# Step 2
echo "=== Collecting test data for variance_analysis"
nowdate=`date +%Y%m%d%H%M%S`
RUN_DIR=run-$nowdate
mkdir -p $RUN_DIR && cd $RUN_DIR
MEASUREMENT_DIR=measurement-$nowdate
export JSI_COLLECT_PMU_EVENT="PAPI_TOT_INS"
mpirun -n 8 jsirun -o $MEASUREMENT_DIR --backtrace --pmu -- ../test > test_run.log 2> test_run.err \
    && ok JSIRUN || die JSIRUN 
echo "=== Validating test data with jsiread ==="
jsiread -i $MEASUREMENT_DIR -o jsiread -f > test_read.log 2> test_read.err \
    && ok jsiread || die jsiread
# Step 3
echo "=== Running test for variance_analysis ==="
export OMP_NUM_THREADS=2
variance_analysis -i $MEASUREMENT_DIR -o variance -f > test_var.log 2> test_var.err \
    && ok variance_analysis || die variance_analysis
python3 $JSI_ROOT/scripts/analysis/variance/heatmap.py --input variance/ --output figure > test_fig.log 2> test_fig.err \
    && ok heatmap || die heatmap
# Step 4
echo "=== Running test for variance_analysis_mpi ==="
mpirun -n 2 variance_analysis_mpi -i $MEASUREMENT_DIR -o variance-mpi -f > test_var_mpi.log 2> test_var.err \
    && ok variance_analysis_mpi || die variance_analysis_mpi
python3 $JSI_ROOT/scripts/analysis/variance/heatmap_mpi.py --input variance-mpi/ --output figure-mpi > test_fig_mpi.log 2> test_fig_mpi.err \
    && ok heatmap_mpi || die heatmap_mpi