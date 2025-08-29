#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "hthread_host.h"

int main(int argc, char **argv) {
    int     rank, size;

    // 初始化MPI环境
    MPI_Init(&argc, &argv);
    // 获取当前进程的编号
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // 获取总的进程数
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int cluster_id = 3 - rank % 4;

    printf("Before dev open\n");
    hthread_dev_open(cluster_id);
    printf("Before dat load\n");
    fflush(stdout);
    hthread_dat_load(cluster_id, "simple_mpi.dev.dat");
    printf("Before hthread malloc\n");
    fflush(stdout);

    unsigned long args[1];
    args[0] = rank;
    printf("Before group create\n");
    fflush(stdout);
    int thread_group_id = hthread_group_create(cluster_id, 1, "daxpy_kernel", 1, 0, args);
    if (thread_group_id == -1) {
        printf("hthread_group_create of rank-%d, cluster_id-%d failed!\n", rank, cluster_id);
        fflush(stdout);
        exit(1);
    }
    printf("Before group wait\n");
    fflush(stdout);
    hthread_group_wait(thread_group_id);
    printf("Before group destroy\n");
    fflush(stdout);
    hthread_group_destroy(thread_group_id);

    printf("Before free\n");
    fflush(stdout);
    printf("Before dat unload\n");
    fflush(stdout);
    hthread_dat_unload(cluster_id);
    printf("Before dev close\n");
    fflush(stdout);
    hthread_dev_close(cluster_id);
    // 结束MPI环境
    MPI_Finalize();
    return 0;
}