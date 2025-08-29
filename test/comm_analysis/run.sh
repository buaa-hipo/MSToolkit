make
mpirun -np 4 jsirun -o ./bad_comm_measurement/ --backtrace -- ./bad_comm
comm_analysis -i ./bad_comm_measurement/ -o bad_comm_report -f -k 50 -m 1
python3 ../../tool/analysis/comm_analysis/heatmap_script.py bad_comm_report/result.comm_mtx