## 采样模块说明文档

采样模块使用PAPI对目标程序在处理器上执行过程中的性能计数器数值（如时钟周期、指令执行次数、缓存缺失次数等）进行定时采集。

使用采样模块进行采样时需要给定目标程序位置，需要采集的性能计数器事件名（可以通过papi_avail查看）和采样时间间隔。

#### 通过`jsirun`使用采样模块

需要设置如下参数：

- `--enable_sample`：选择开启后，在`LD_PRELOAD`环境变量中添加采样模块动态库
- `--events <(string)PMU events>`：选择需要采集的`PMU`事件
- `--samp_interval <(double)interval while sampling>`：采样时间间隔，单位是秒，用浮点数表示
- `-- <target program> <args>`：目标程序

示例启动命令行如下：

```
$JSI-Toolkit/install/bin/jsirun --enable_sample --samp_interval 1.2 --events PAPI_TOT_INS,PAPI_TOT_CYC -- <target program> <args> 
```

#### 测试采样模块

在测试文件夹下(`$JSI-Toolkit/test/lib/sampling`)给出了一个目标测试程序示例，是一个倒计时计数程序

在测试采样模块前，在项目CMakeLists.txt中加入编译相应的测试文件夹

使用`jsirun`来测试采样模块，目标程序即为`$build/test/lib/sampling/code2measure`