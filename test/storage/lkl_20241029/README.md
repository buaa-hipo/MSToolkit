# Test report for storage support for metadata (20241029 by LKL)

## Test scripts

This test will check whether storage support for metadata can work.

## Expected outputs

The test script should output with the following outputs. Any `FAILED` outputs indicates the specified test is failed. The test logic is to write two meta sections and later read the same meta sections with the same user specific content.

```
$ ./build/test/storage/lkl_20241029/meta_storage_test 
[2024-10-29 20:47:29.004] [info] mmap 7e0519cb2000 with length 0 to 7e0519cb2000, total length 8000
====================================================
[2024-10-29 20:47:29.004] [info] mmap 7d0519cb2000 with length 0 to 7d0519cb2000, total length 8000
[2024-10-29 20:47:29.004] [info] driver exit
[2024-10-29 20:47:29.004] [info] pass
```