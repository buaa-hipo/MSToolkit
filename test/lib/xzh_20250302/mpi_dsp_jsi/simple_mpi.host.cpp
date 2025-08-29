#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "hthread_host.h"

#define N             320
#define NUM_PROCESSES 4
#define BLOCK_SIZE    (N / NUM_PROCESSES)

int main(int argc, char **argv) {
    int     rank, size;
    int     local_sum  = 0;
    int     global_sum = 0;
    int     start, end;
    double *whole_array = NULL;

    // 初始化MPI环境
    MPI_Init(&argc, &argv);
    // 获取当前进程的编号
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // 获取总的进程数
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    system("hostname");
    printf("size = %d\n", size);

    // 检查进程数是否符合要求
    if (size != NUM_PROCESSES) {
        if (rank == 0) {
            printf("需要 %d 个进程来运行此程序。\n", NUM_PROCESSES);
        }
        MPI_Finalize();
        return 1;
    }

    const int cluster_id = rank % 4;

    printf("Before dev open\n");
    hthread_dev_open(cluster_id);
    printf("Before dat load\n");
    fflush(stdout);
    hthread_dat_load(cluster_id, "simple_mpi.dev.dat");
    printf("Before hthread malloc\n");
    fflush(stdout);
    double *sub_array  = (double *)hthread_malloc(cluster_id, sizeof(double) * BLOCK_SIZE, HT_MEM_RW);
    double *sub_result = (double *)hthread_malloc(cluster_id, sizeof(double) * BLOCK_SIZE, HT_MEM_RW);

    // 分割问题，确定每个进程负责的范围
    start = rank * BLOCK_SIZE;
    end   = start + BLOCK_SIZE - 1;

    for (int i = start; i <= end; ++i) {
        sub_array[i - start] = i / 10.0;
    }

    unsigned long args[4];
    args[0] = BLOCK_SIZE;
    args[1] = (unsigned long)2.0;
    args[2] = (unsigned long)sub_array;
    args[3] = (unsigned long)sub_result;

    printf("Before group create\n");
    fflush(stdout);
    int thread_group_id = hthread_group_create(cluster_id, 10, "daxpy_kernel", 2, 2, args);
    if (thread_group_id == -1) {
        printf("hthread_group_create of rank-%d, cluster_id-%d failed!\n", rank, cluster_id);
        fflush(stdout);
        exit(1);
    }
    printf("Before group wait\n");
    fflush(stdout);
    hthread_group_wait(thread_group_id);
    int local_result = *sub_result;
    printf("Before group destroy\n");
    fflush(stdout);
    hthread_group_destroy(thread_group_id);

    whole_array = (double *)malloc(BLOCK_SIZE * size * sizeof(double));
    if (whole_array == NULL) {
        fprintf(stderr, "Memory allocation failed for total_array on rank %d\n", rank);
        MPI_Finalize();
        return 1;
    }

    // 使用MPI_Gather将所有进程的sub_array收集到0号进程的total_array中
    MPI_Gather(sub_array, BLOCK_SIZE, MPI_DOUBLE, whole_array, BLOCK_SIZE, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        bool flag = true;
        for (int i = 0; i < N; ++i) {
            if (whole_array[i] != i / 10.0) {
                flag = false;
            }
        }
        free(whole_array);
        if (flag)
            printf("[daxpy] Test passed!\n");
    }

    printf("Before free\n");
    fflush(stdout);
    hthread_free(sub_array);
    hthread_free(sub_result);
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