M3000_ENV=/thfs3/software/programming_env/mt3000_programming_env/
export LD_LIBRARY_PATH=${M3000_ENV}/third-party-lib:$LD_LIBRARY_PATH
DSP_CC_HOME=${M3000_ENV}/dsp_compiler

HTHREAD_HOME=/thfs3/home/yanghailong/mt3000_programming_env
DSP_OBJ=$(patsubst %.c,%.o,$(wildcard dsp_*.c))

all: main dsp_kernel.dat

main: master_*.cpp 
	mpicxx -o $@ -O3 -fopenmp -I ${HTHREAD_HOME}/include $^ ${HTHREAD_HOME}/lib/libhthread_host.a  -lpthread
	echo 'host done'

main: master.h


dsp_kernel.out: $(DSP_OBJ)
	${DSP_CC_HOME}/bin/MT-3000-ld -o $@ \
	 -L${HTHREAD_HOME}/lib --gc-sections -Tdsp_sm.lds \
	 $^ \
	 -lhthread_device ${DSP_CC_HOME}/lib/vlib3000.a ${DSP_CC_HOME}/lib/slib3000.a ${DSP_CC_HOME}/lib/vlibvsip.a

%.o: %.c
	${DSP_CC_HOME}/bin/MT-3000-gcc -std=c99 -c -o $@ -O3 -fenable-m3000 -ffunction-sections -flax-vector-conversions \
	 -I${HTHREAD_HOME}/include -I${DSP_CC_HOME}/include $^

%.o: dsp.h dsp_dslash_spinor.h
dsp_kernel.dat: dsp_kernel.out
	${DSP_CC_HOME}/bin/MT-3000-makedat -J $^

clean:
	rm -f main *.out *.dat *.o
