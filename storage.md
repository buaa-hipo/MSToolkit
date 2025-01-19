# 存储API使用说明

存储框架使用backend作为管理存储的接口，使用section来组织实际存储。在使用时，首先需要创建backend，再使用backend打开对应的section，再在section中写入数据

目前存储API提供了三种不同的存储section， 包括DataSection、StringSection、StreamSection。除此之外，还有一种DirSection，起到目录的作用

lib/pse/test/new_backend_test.cpp中有API使用示例

## Backend
backend计划支持多种不同的后端，目前已经完成的后端只有pse::fsl::RawSectionBackend，该后端实现了文件聚合，支持一个节点内的所有进程共享同一文件写入，使用方式如下：

```c++
#include <ral/backend.h>
#include <fsl/raw_backend.h>

auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::WRITE);
auto raw_backend2 = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::READ);
auto _backend = ral::BackendWrapper(std::move(raw_backend));
ral::BackendInterface &backend = static_cast<pse::ral::BackendInterface &>(_backend);
```

其中raw_backend为实际后端，而backend为其封装。_backend的类型与raw_backend相关，直接使用会导致难以迁移到其它后端，而backend为类型擦除的结果，建议在实际使用时使用backend。
对于写入的情况，目前为析构时自动保存的设计。

## DirSection
每个后端都有一个RootSection，类似于文件系统的根目录，其它的Section都直接挂载在RootSection中。
```c++
auto root = backend.openRootSection();
auto dir = root->openDirSection(desc, create);
auto dir2 = dir1->openDirSection(desc, create);
```
DirSection支持嵌套，通过一个32位int索引desc，所有的DirSection构成了一棵树。建议按照Root-进程-线程的方式组织DirSection。openSection的第二个参数表示Section不存在时是否创建。

## StringSection
StringSection为用于存储字符串的Section，其接口如下所示
```c++
auto string_section = dir->openStringSection(desc, create);
auto offset = string_sectioin->write(str);
auto res2 = string_sectioin->read(str, offset, buf_len);
```
offset为字符串的唯一索引，写入字符串时write会返回该值，调用方应保存该值用于读取。读取时，三个参数分别为字符串缓冲区地址、索引与缓冲区长度，返回值为字符串长度。若缓冲区长度小于字符串长度则不会读取。

## DataSection
DataSection为用于存储定长类型结构体的Section。
```c++
auto data_sec = dir->openDataSection<Record_t>(desc, create, time_offset, variable_length);
data->write(&r2);
data->read(&r2, i);
```
其中time_offset用于描述时间戳字段偏移量，可设置为(uint64_t)(&r.time) - (uint64_t)(&r)
variable_length用于可变PMU等追加在结构体后的定长字段，每个section只能取一个值，传入值为可变字段的字节数
read接口表示读取第i个record。
读取时不需要设置time_offset与variable_length，传入值会被忽略

## StreamSection
StreamSection支持类文件接口，可以写入任意数据，但写入目前只支持顺序读写，不支持seek。不建议大量使用该类型的Section，对后续的压缩等会有较大障碍。可以为meta data等数据量较小的数据使用该类型的Section存储。
```c++
auto stream_sec = dir->openStreamSection(desc, create);
auto real_len = stream_sec->read(buf, len);
stream_sec->write(buf, len);
```
