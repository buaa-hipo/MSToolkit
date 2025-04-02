#include <stdio.h>
#include <mpi.h>
#include <cstdlib>

int main(int argc, char** argv) {
    int rank, size;

    // 初始化MPI环境
    MPI_Init(&argc, &argv);

    // 获取当前进程的编号
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // 获取总的进程数
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 打印每个进程的编号和总进程数
    printf("Hello from process %d of %d\n", rank, size);
    system("hostname");

    // 结束MPI环境
    MPI_Finalize();

    return 0;
}