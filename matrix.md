# 迈创采集功能和插桩器使用说明


## 迈创架构PMU采集说明

**（板卡需要对dtb文件进行修改后重启计算机再进行后续步骤）**

迈创架构下CPU性能事件采集需要对PAPI工具进行修改后重新编译，具体流程为：
进入PAPI的源码目录，和src目录同级，执行命令： 
```
patch -p 1 <ft.patch
```
其中，ft.patch为`JSI-Toolkit/mt-depencies`目录下的同名文件。该命令会修改PAPI源代码中的多个文件。之后重新编译PAPI工具，并将PAPI安装目录下的lib目录路径加入`LD_LIBRARY_PATH`环境变量中：
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<path-to-papi>/lib
```
如果以下命令：
```
<path-to-papi>/bin/papi_avail
```
的输出显示可以支持一定量的PAPI预置事件（板卡为18个基础3个衍生共21个事件），则代表迈创架构下CPU的性能事件采集功能配置完成
***
如果环境中hthread库的版本小于20241223,则需要更新hthread库以支持迈创架构下DSP端的多核性能事件采集功能。

## 迈创架构插桩器使用说明

迈创架构插桩器默认对几乎所有用户源代码中影响性能的hthread库函数进行插桩，但可以通过参数选项进行以黑名单形式进行配置：
```
mt_dev_instrumenter <your-source-code> [-no-malloc] [-no-memcpy] [-no-func]
```
三个黑名单选项在设置后即代表不进行对应类别hthread函数的插桩

在使用插桩器前需要将`gcc-runtime`和`llvm`工具的lib目录加入`LD_LIBRARY_PATH`环境变量，将clang，并改动源代码
若插桩对象为Device端代码，向源代码头部添加：
```
#include "instrument/instrumented_func_dev.h"
#include "record/mt_double_buffer.h"
#include "record/mt_callback_defs.h"
```
若为Host端代码，则添加：
```
#include "instrument/instrumented_func_host.h"
#include "record/mt_callback_defs.h"
```
插桩器会将代码在AST层面进行转换，为`your-code.cpp`生成`your-code_expand.cpp`，在编译前在需要源代码的编译脚本中改变被编译对象
对于Host端代码，添加
```
-ldefault_mt_host_callbacks -lmt_host_tracer -lbacktrace -lFSL -ljsi_record -lpapi -ldwarf -lfmt -lspdlog
```
的链接参数，并将PAPI LibDwarf fmt spdlog和MPI的lib目录以-L的形式加入链接参数
对于Device端代码，将`JSI-Toolkit/install/lib`下的`libmtpmu.a`和`libmt_dev.a`两个静态库文件与Device端代码一起链接（可能需要加入MT-3000 DSP库中的glibc.a并调整库与源代码之间的顺序）
编译链接成的Host端可执行文件和Device端dat文件便可按照原本方式用jsirun运行和采集，用户也可以为Host端代码实现自定义`buffer callback`和`api callback`函数，编译成库文件并替代`default_mt_host_callbacks`库进行自定义采集

目前迈创架构下不支持DMA数据传输工作的采集