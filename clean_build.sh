rm -rf build; rm -rf install; cd include/record; rm -rf wrap_defines.h; cd ../..;
cd lib/accl/matrix/device/
rm -f libmtpmu.a MT_PMU_collector.o libmt_dev.a mt_double_buffer.o instrumented_func_dev.o
cd ../../../..
