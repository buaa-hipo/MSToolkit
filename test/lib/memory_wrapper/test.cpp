#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <memory> // 使用智能指针

// 模拟数据处理
void processData() {
    std::vector<int> data(100); // 使用 vector 自动管理内存
    for (int i = 0; i < 100; ++i) {
        data[i] = i * 2;
    }
    std::cout << "Processed data." << std::endl;
}

// 模拟文件读取
void readFile(const std::string& filename) {
    std::ifstream file(filename); // 使用局部变量管理文件资源
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::cout << "Read line: " << line << std::endl;
        }
    } else {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
}

// 模拟配置加载
void loadConfig(std::map<std::string, std::string>& config) {
    config.insert({"host", "localhost"});
    config.insert({"port", "8080"});
    std::cout << "Loaded config." << std::endl;
}

int main() {
    // // 调用模拟的处理函数
    // processData();

    // // 调用文件读取函数
    // readFile("test.txt");

    // // 加载配置
    // std::map<std::string, std::string> config; // 使用局部变量管理配置
    // loadConfig(config);

    // std::cout << "Program finished execution." << std::endl;

    int *list = new int[10];

    int *list2 = (int *)malloc(10 * sizeof(int));

    free(list2);
    return 0;
}


// #include <iostream>
// #include <thread>
// #include <vector>
// #include <memory>  // std::unique_ptr for memory management

// void allocate_and_use_memory(int thread_id, size_t size) {
//     // 动态分配内存
//     int* ptr = new int[size];  // 或者可以使用 std::unique_ptr<int[]> 来自动管理内存

//     // 初始化内存
//     for (size_t i = 0; i < size; ++i) {
//         ptr[i] = i + thread_id;  // 每个线程给内存中的数据赋不同的值
//     }

//     // 打印该线程操作的内存内容的一部分
//     std::cout << "Thread " << thread_id << " allocated memory, first value: " << ptr[0] << ", last value: " << ptr[size-1] << std::endl;

//     // 完成工作后释放内存
//     delete[] ptr;
// }

// int main() {
//     // 创建多个线程
//     const int num_threads = 4;
//     const size_t memory_size = 1000;  // 每个线程分配 1000 个整数

//     std::vector<std::thread> threads;

//     // 创建并启动线程
//     for (int i = 0; i < num_threads; ++i) {
//         threads.push_back(std::thread(allocate_and_use_memory, i, memory_size));
//     }

//     // 等待所有线程完成
//     for (auto& t : threads) {
//         t.join();
//     }

//     std::cout << "All threads have finished their tasks." << std::endl;
//     return 0;
// }