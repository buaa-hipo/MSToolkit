# 合并到writer
set exec-wrapper bash -c 'export LD_PRELOAD="../../..//install/bin/libmpi_wrapper.so ../../../install/lib/libjsi_record.so"; export JSI_ENABLE_MEMORY_COLLECT=ON ;export JSI_ENABLE_BACKTRACE=ON; export JSI_MEASUREMENT_DIR="collect"; export JSI_SAMPLING_MODE=1; export JSI_SAMPLING_RATIO=0; export JSI_SAMPLING_IFRANDOM=0; export JSI_SAMPLING_ONTIME=0; export JSI_ENABLE_PMU=ON; export JSI_COLLECT_PMU_EVENT="PAPI_TOT_INS"; exec "$@"' bash
set solib-search-path '/home/luohaonan/JSI-Toolkit/install/lib/libjsi_record.so'

set follow-fork-mode child
