# Test report for storage support for backtrace (20241024 by LKL)

## Test scripts

This test will check whether storage support for backtrace can work.

## Expected outputs

The test script should output with the following outputs. Any `FAILED` outputs indicates the specified test is failed. The test logic is the same as the test `lib/common/backtrace_test` where the last backtrace block of the section above `====================================================` is equal to the section below it.

```
$ ./build/test/storage/lkl_20241024/backtrace_storage_test 
[2024-10-24 20:55:08.004] [info] mmap 7eb7fd508000 with length 0 to 7eb7fd508000, total length 8000
./test/storage/lkl_20241024/backtrace_storage_test(+a755) (0x40a755):(null) [??:??:0]
./test/storage/lkl_20241024/backtrace_storage_test(+88a0) (0x4088a0):(null) [??:??:0]
/lib/x86_64-linux-gnu/libc.so.6(+21c87) (0x7fb7ffe29c87):__libc_start_main [??:??:0]
./test/storage/lkl_20241024/backtrace_storage_test(+934a) (0x40934a):(null) [??:??:0]

n == 5
./test/storage/lkl_20241024/backtrace_storage_test(+a777) (0x40a777):(null) [??:??:0]
./test/storage/lkl_20241024/backtrace_storage_test(+88a0) (0x4088a0):(null) [??:??:0]
/lib/x86_64-linux-gnu/libc.so.6(+21c87) (0x7fb7ffe29c87):__libc_start_main [??:??:0]
./test/storage/lkl_20241024/backtrace_storage_test(+934a) (0x40934a):(null) [??:??:0]

**./test/storage/lkl_20241024/backtrace_storage_test(+a8ff) (0x40a8ff):(null) [??:??:0]
./test/storage/lkl_20241024/backtrace_storage_test(+88a0) (0x4088a0):(null) [??:??:0]
/lib/x86_64-linux-gnu/libc.so.6(+21c87) (0x7fb7ffe29c87):__libc_start_main [??:??:0]
./test/storage/lkl_20241024/backtrace_storage_test(+934a) (0x40934a):(null) [??:??:0]**

====================================================
[2024-10-24 20:55:08.008] [info] driver exit
[2024-10-24 20:55:08.008] [info] mmap 7eb7fd508000 with length 0 to 7eb7fd508000, total length 8000
**./test/storage/lkl_20241024/backtrace_storage_test(+a8ff) (0x40a8ff):(null) [??:??:0]
./test/storage/lkl_20241024/backtrace_storage_test(+88a0) (0x4088a0):(null) [??:??:0]
/lib/x86_64-linux-gnu/libc.so.6(+21c87) (0x7fb7ffe29c87):__libc_start_main [??:??:0]
./test/storage/lkl_20241024/backtrace_storage_test(+934a) (0x40934a):(null) [??:??:0]**

**./test/storage/lkl_20241024/backtrace_storage_test(+a8ff) (0x40a8ff):(null) [??:??:0]
./test/storage/lkl_20241024/backtrace_storage_test(+88a0) (0x4088a0):(null) [??:??:0]
/lib/x86_64-linux-gnu/libc.so.6(+21c87) (0x7fb7ffe29c87):__libc_start_main [??:??:0]
./test/storage/lkl_20241024/backtrace_storage_test(+934a) (0x40934a):(null) [??:??:0]**

g_ctx == 6
[2024-10-24 20:55:08.036] [info] driver exit
[2024-10-24 20:55:08.036] [info] pass
```