#include <iostream>
#include <mpi.h>
#include <hip/hip_runtime.h>

// HIP内核函数，将数组中的每个元素加1
__global__ void addOneKernel(int *data, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        data[idx] = data[idx] + 1;
    }
}

int main(int argc, char** argv) {
    int rank, num_procs;
    const int ARRAY_SIZE = 1024;
    const int BLOCK_SIZE = 256;
    const int GRID_SIZE = (ARRAY_SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // 初始化MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // 检查进程数是否不超过可用GPU数
    int deviceCount;
    hipGetDeviceCount(&deviceCount);
    if (num_procs > deviceCount) {
        if (rank == 0) {
            std::cerr << "Error: Number of MPI processes exceeds the number of available HIP devices." << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    // 根据进程排名设置对应的GPU设备
    hipSetDevice(rank);

    // 分配主机内存
    int *h_data = new int[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        h_data[i] = i;
    }

    // 分配设备内存
    int *d_data;
    hipMalloc((void**)&d_data, ARRAY_SIZE * sizeof(int));

    // 将数据从主机复制到设备
    hipMemcpy(d_data, h_data, ARRAY_SIZE * sizeof(int), hipMemcpyHostToDevice);

    // 启动HIP内核
    addOneKernel<<<GRID_SIZE, BLOCK_SIZE>>>(d_data, ARRAY_SIZE);

    // 同步设备
    hipDeviceSynchronize();

    // 将数据从设备复制回主机
    hipMemcpy(h_data, d_data, ARRAY_SIZE * sizeof(int), hipMemcpyDeviceToHost);

    // 检查结果
    bool all_correct = true;
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        if (h_data[i] != i + 1) {
            all_correct = false;
            break;
        }
    }
    
    // 打印结果
    if (all_correct) {
        std::cout << "Process " << rank << " on GPU " << rank << ": HIP kernel executed correctly." << std::endl;
    } else {
        std::cout << "Process " << rank << " on GPU " << rank << ": HIP kernel execution failed." << std::endl;
    }

    // 释放设备内存
    hipFree(d_data);
    // 释放主机内存
    delete[] h_data;

    // 结束MPI
    MPI_Finalize();

    return 0;
}