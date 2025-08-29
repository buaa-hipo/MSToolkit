#include "hthread_device.h"
#include <compiler/m3000.h>
#include <stdio.h>

typedef struct {
    int a, b, c;
    unsigned long d;
} __attribute__((aligned(1)))type_s;

void test_func() {
    hthread_printf("?\n");
}

__global__ void reduce_kernel(long size, long *array, long *result) {
    test_func();
    hthread_printf("sizeof(type_s) = %lu\n", sizeof(type_s));
    long *cache = (long*)scalar_malloc(size * sizeof(long));
    hthread_printf("Got size: %ld\n", size);
    fflush(stdout);
    scalar_load(array, cache, sizeof(long) * size);
    
    long *res = (long*)scalar_malloc(sizeof(long));
    *res = 0;
    for (int i = 0; i < size; ++i) {
        res[0] += cache[i];
    }
    
    scalar_store(res, result, sizeof(long));
    hthread_printf("result = %ld\n", *result);
    scalar_free(cache);
    scalar_free(res);
}