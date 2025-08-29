M3000_ENV=/thfs3/software/programming_env/mt3000_programming_env/
export LD_LIBRARY_PATH=${M3000_ENV}/third-party-lib:$LD_LIBRARY_PATH
DSP_CC_HOME=${M3000_ENV}/dsp_compiler

JSI_LIBS=/thfs3/home/yanghailong/xzh/JSI-Toolkit-unified/install/lib

DEV_TRACER=${JSI_LIBS}/libmt_dev.a
DEV_PMU=${JSI_LIBS}/libmtpmu.a

HTHREAD_HOME=/thfs3/home/yanghailong/mt3000_programming_env

MT_LIB=${HTHREAD_HOME}/lib
PAPI_LIB=/thfs3/home/yanghailong/papi/lib
DWARF_LIB=/thfs3/home/yanghailong/libdwarf/lib
FMT_LIB=/thfs3/home/yanghailong/xzh/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-12.3.0/fmt-10.2.1-3rsp7oz2evm2h5epmxrwqazwzyptgrxr/lib
SPDLOG_LIB=/thfs3/home/yanghailong/xzh/spack-0.23.1/opt/spack/linux-ubuntu20.04-aarch64/gcc-12.3.0/spdlog-1.14.1-uimdwzxtxiis2uq7xfy7k4g5ry4bdtad/lib

DSP_OBJ=$(patsubst %.c,%.o,$(wildcard dsp_*.c))

all: main dsp_kernel.dat

main: master_*.cpp 
	mpicxx -o $@ -g -O3 -fopenmp -I/thfs3/home/yanghailong/xzh/JSI-Toolkit-unified/include -I ${HTHREAD_HOME}/include -L${JSI_LIBS} -L${MT_LIB} -L${DWARF_LIB} -L${PAPI_LIB} -L${SPDLOG_LIB} -L${FMT_LIB}  $^ -Wl,--start-group -lhthread_host -g -lpthread -ldefault_mt_host_callbacks -lmt_host_tracer -lbacktrace -lFSL -ljsi_record -ljsi_record_info -lpapi -ldwarf -Wl,--end-group
	echo 'host done'

main: master.h


dsp_kernel.out: $(DSP_OBJ)
	${DSP_CC_HOME}/bin/MT-3000-ld -o $@ \
	 -L${HTHREAD_HOME}/lib --gc-sections -Tdsp_sm.lds \
	 ${DEV_TRACER} ${DEV_PMU} ${DSP_CC_HOME}/lib/libc.a ${MT_LIB}/libhthread_device.a \
	 $^ \
	  ${DSP_CC_HOME}/lib/vlib3000.a ${DSP_CC_HOME}/lib/slib3000.a ${DSP_CC_HOME}/lib/vlibvsip.a ${DEV_TRACER} ${DEV_PMU} ${DSP_CC_HOME}/lib/libc.a ${MT_LIB}/libhthread_device.a 

# dsp_kernel.out: $(DSP_OBJ)
# 	${DSP_CC_HOME}/bin/MT-3000-ld -o $@ \
# 	 -L${HTHREAD_HOME}/lib --gc-sections -Tdsp_sm.lds \
# 	 $^ \
# 	 -lhthread_device ${DSP_CC_HOME}/lib/vlib3000.a ${DSP_CC_HOME}/lib/slib3000.a ${DSP_CC_HOME}/lib/vlibvsip.a ${DEV_PMU} ${DEV_TRACER} ${DSP_CC_HOME}/lib/libc.a ${MT_LIB}/libhthread_device.a ${DSP_CC_HOME}/lib/libc.a ${DEV_PMU} ${DEV_TRACER}

%.o: %.c
	${DSP_CC_HOME}/bin/MT-3000-gcc -std=c99 -c -o $@ -O3 -fenable-m3000 -ffunction-sections -flax-vector-conversions \
	 -I${HTHREAD_HOME}/include -I${DSP_CC_HOME}/include $^

%.o: dsp.h dsp_dslash_spinor.h
dsp_kernel.dat: dsp_kernel.out
	${DSP_CC_HOME}/bin/MT-3000-makedat -J $^

clean:
	rm -f main *.out *.dat *.o
