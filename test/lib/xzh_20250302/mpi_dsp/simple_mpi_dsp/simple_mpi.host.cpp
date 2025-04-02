#include <stdio.h>
#include <unistd.h>
#include "hthread_host.h"
#include <mpi.h>


int main(int argc, char **argv) {
    int rank, size;

    printf("Before MPI init\n");
    fflush(stdout);

    // 初始化MPI环境
    MPI_Init(&argc, &argv);
    // 获取当前进程的编号
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // 获取总的进程数
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int cluster_id = rank % 4;
    char hostname[256];
    gethostname(hostname, 256);
    printf("rank-%d trying to open cluster-%d on node-%s\n", rank, cluster_id, hostname);
    fflush(stdout);
    hthread_dev_open(cluster_id);
    hthread_dat_load(cluster_id, "simple_mpi.dev.dat");

    printf("Before kernel\n");
    fflush(stdout);
    unsigned long args[1];
    args[0] = rank;
    int thread_group_id = hthread_group_create(cluster_id, 24, "reduce_kernel", 1, 0, args);
    hthread_group_wait(thread_group_id);
    printf("rank-%d on node-%s KERNEL returned!\n", rank, hostname);
    fflush(stdout);
    hthread_group_destroy(thread_group_id);

    hthread_dat_unload(cluster_id);
    hthread_dev_close(cluster_id);

    // 结束MPI环境
    MPI_Finalize();
    return 0;
}