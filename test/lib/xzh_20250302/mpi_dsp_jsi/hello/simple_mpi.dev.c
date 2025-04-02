#include <stdint.h>
#include <stdio.h>
#include <compiler/m3000.h>
#include "hthread_device.h"

__global__
void daxpy_kernel(int rank){
    int *ptr = (int*)scalar_malloc(4);
    scalar_free(ptr);
    hthread_printf("rank-%d inside daxpy_kernel!\n", rank);
    fflush(stdout);
}