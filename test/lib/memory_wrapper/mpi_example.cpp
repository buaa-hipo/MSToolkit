#include <iostream>
#include <mpi.h>
#include <unistd.h>

int main(int argc, char** argv) {
    // 初始化 MPI 环境
    MPI_Init(&argc, &argv);

    // 获取进程总数和当前进程的编号
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size); // 获取进程数
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank); // 获取当前进程的编号

    // 获取每个进程的主机名
    char hostname[256];
    gethostname(hostname, 256);

    // 输出进程的信息
    std::cout << "Hello from process " << world_rank << " out of " << world_size
              << " on host " << hostname << std::endl;

    // 如果是进程 0, 发送消息给进程 1
    if (world_rank == 0) {
        const char* message = "Hello from process 0";
        // MPI_Send(message, strlen(message) + 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        std::cout << "Process 0 sent message: " << message << std::endl;
    }
    // 如果是进程 1, 接收进程 0 发送的消息
    // else if (world_rank == 1) {
    //     char received_message[256];
    //     MPI_Recv(received_message, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //     std::cout << "Process 1 received message: " << received_message << std::endl;
    // }

    // 结束 MPI 环境
    MPI_Finalize();
    return 0;
}