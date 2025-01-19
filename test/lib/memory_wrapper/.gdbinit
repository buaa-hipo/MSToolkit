set exec-wrapper bash -c 'export LD_PRELOAD="/home/luohaonan/JSI-Toolkit/install/bin/libmpi_wrapper.so /home/luohaonan/JSI-Toolkit/install/bin/libmemory_wrapper.so"; export JSI_ENABLE_BACKTRACE=ON; export JSI_MEASUREMENT_DIR="/home/luohaonan/JSI-TEST/collect"; export JSI_SAMPLING_MODE=1; export JSI_SAMPLING_RATIO=0; export JSI_SAMPLING_IFRANDOM=0; export JSI_SAMPLING_ONTIME=0; export JSI_ENABLE_PMU=ON; export JSI_COLLECT_PMU_EVENT="PAPI_TOT_INS"; exec "$@"' bash

# set exec-wrapper bash -c 'export LD_PRELOAD="/home/luohaonan/JSI-Toolkit/install/bin/libmemory_wrapper.so"; export JSI_MEASUREMENT_DIR="/home/luohaonan/JSI-TEST/collect"; export JSI_SAMPLING_MODE=1; export JSI_SAMPLING_RATIO=0; export JSI_SAMPLING_IFRANDOM=0; export JSI_SAMPLING_ONTIME=0; export JSI_ENABLE_PMU=ON; export JSI_COLLECT_PMU_EVENT="PAPI_TOT_INS"; exec "$@"' bash

set solib-search-path '/home/luohaonan/JSI-Toolkit/install/bin/libmemory_wrapper.so'

# set follow-fork-mode child