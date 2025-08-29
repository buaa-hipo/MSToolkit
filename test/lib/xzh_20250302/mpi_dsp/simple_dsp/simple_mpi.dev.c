#include "hthread_device.h"
#include <compiler/m3000.h>
#include <stdio.h>

__global__ void reduce_kernel() {
    hthread_printf("Begin of kernel!\n");
    fflush(stdout);
}