#include "hthread_device.h"
#include <compiler/m3000.h>
#include <stdio.h>

__global__ void reduce_kernel(int rank) {
    hthread_printf("[CORE-%d][rank-%d] Begin of kernel!\n", get_core_id(), rank);
    fflush(stdout);
}