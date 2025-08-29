# 迈创环境说明

- 迈创的Device程序只能使用MT-3000-gcc编译，不能使用C++（目前），链接也需要用特殊链接器
- Host端有动态库和静态库两种，Device端只有静态库，用到hthread接口的程序需要链接这两个库
- 迈创的Device端程序被编译链接（？）成dat文件，由Host端程序在运行时动态加载
- `hthread_malloc`得到的内存空间是由Host和Device端共享的，目前没有发现由内存序导致的问题
- 迈创Device端的GSM地址似乎会涉及`0x0`这个看似非法的地址，打印出来也是(nil)，但就是和NULL不等，注意
- 迈创Device端不能进行文件IO，stdio也只能用`hthread_printf`操作，否则卡死
- **迈创Device端对DDR和HBM（HBSM）的内存分配会向CPU发中断，如果恰逢CPU也在分配DDR内存，则会死锁，所以在当前版本的hthread环境下Device端尽量不要进行一切有关DDR和HBM（HBSM）内存分配的操作（malloc、hbm_malloc、hthread_printf等）**，其它stdlib.h中的函数未做实验验证
- Device端似乎没有直接获取自己所在DSP簇号的方法

# 迈创插桩器说明

- 具体流程为先将输入源代码文件进行预处理（由于`hthread_group_create`的特殊处理），再进行AST节点匹配，最后替换函数调用的函数名或插入代码
- 目前的插桩标准为将`hthread_malloc`替换为`instrumented_hthread_malloc`的形式
- 支持输入参数的黑名单功能
- 依赖`libclang.so`和`libclang-cpp.so`
- 没有将插桩后代码直接写回原文件

# 迈创采集功能说明

- 设计上分为源代码插桩实现的hthread库api两端回调功能（callback api）以及异步工作较精确采集（activity api）两类
- 为了在DSP程序中读取PMU，封装了hthread device库的三个prof接口
- 由于DSP目前不支持DMA的callback，全部功能暂时由callback api实现，相当于device端的两段双缓冲区只用了一段，host端实现的缓冲池也没有用到
- 对应kernel函数的mt_record_kernel_t中op为存储接口的string section在write的时候所返回的offset
- 在accl_tracer.cpp中用一个采集线程不断循环查询每个cluster的每个core对应的缓冲有没有满，满则读出
- 利用两个32位int的低24位标识一个cluster的对应core的双缓冲可读可写情况

