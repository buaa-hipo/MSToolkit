#include <stdio.h>
#include "hthread_host.h"
#include <mpi.h>


int main(int argc, char **argv) {
    const int cluster_id = 0;

    hthread_dev_open(cluster_id);
    hthread_dat_load(cluster_id, "simple_mpi.dev.dat");

    printf("Before kernel\n");
    fflush(stdout);
    int thread_group_id = hthread_group_create(cluster_id, 24, "reduce_kernel", 0, 0, nullptr);
    hthread_group_wait(thread_group_id);
    printf("Kernel returned!\n");
    fflush(stdout);
    hthread_group_destroy(thread_group_id);

    hthread_dat_unload(cluster_id);
    hthread_dev_close(cluster_id);

    return 0;
}