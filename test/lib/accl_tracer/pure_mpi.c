#include <stdio.h>
#include <mpi.h>

int main(int argc, char** argv) {
    int rank, num_procs;

    // 初始化 MPI 环境
    MPI_Init(&argc, &argv);

    // 获取当前进程的编号
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // 获取总的进程数量
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // 输出当前进程的编号和总进程数
    printf("Hello from process %d out of %d processes.\n", rank, num_procs);

    // 结束 MPI 环境
    MPI_Finalize();

    return 0;
}