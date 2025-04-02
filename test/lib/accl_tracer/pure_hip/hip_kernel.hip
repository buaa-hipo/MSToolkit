#include <hip/hip_runtime.h>
#include <iostream>
#include <any>

// HIP内核函数，将数组中的每个元素加1
__global__ void addOneKernel(int *data, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        data[idx] = data[idx] + 1;
    }
}

#define CHECK_HIP_ERROR(err) \
    if (err != hipSuccess) { \
        std::cerr << "HIP error: " << hipGetErrorString(err) << " at line " << __LINE__ << std::endl; \
        exit(-1); \
    }

int main(){
    int rank = 0;
    const int ARRAY_SIZE = 1024;
    const int BLOCK_SIZE = 256;
    const int GRID_SIZE = (ARRAY_SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE;

    int *h_data = new int[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        h_data[i] = i;
    }
    

    CHECK_HIP_ERROR(hipSetDevice(1));
    // 分配设备内存
    int *d_data;
    CHECK_HIP_ERROR(hipMalloc((void**)&d_data, ARRAY_SIZE * sizeof(int)));

    // 将数据从主机复制到设备
    CHECK_HIP_ERROR(hipMemcpy(d_data, h_data, ARRAY_SIZE * sizeof(int), hipMemcpyHostToDevice));

    // 启动HIP内核
    addOneKernel<<<GRID_SIZE, BLOCK_SIZE>>>(d_data, ARRAY_SIZE);

    // 同步设备
    CHECK_HIP_ERROR(hipDeviceSynchronize());

    // 将数据从设备复制回主机
    CHECK_HIP_ERROR(hipMemcpy(h_data, d_data, ARRAY_SIZE * sizeof(int), hipMemcpyDeviceToHost));

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
    CHECK_HIP_ERROR(hipFree(d_data));
    // 释放主机内存
    delete[] h_data;

    return 0;
}