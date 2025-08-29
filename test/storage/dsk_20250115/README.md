# Test report for storage library(20241219 by DSK)

## Test scripts

This test will check whether storage library interface can work.

## Expected outputs

The test script should output with the following outputs. Any `FAILED` or `error` outputs indicates the specified test is failed.

First, run write_trace to generate the trace file.
```
$ ./write_trace 
[JSILOG] Initialize Record Writer Library.
[2025-01-15 18:09:17.958] [info] mmap 7e1f264ef000 with length 0 to 7e1f264ef000, total length 8000
[JSILOG] Finalize Record Writer Library.
[2025-01-15 18:09:17.959] [info] mmap 7e1f264f7000 with length 0 to 7e1f264f7000, total length 10000
[2025-01-15 18:09:17.959] [info] driver exit
```
A file data.<node-name>.sec-tr will be created.
Then, run record_reader_test to validate the trace file.
```
$ ./record_reader_test .
[JSILOG] RecordReader::load>> Ignore file ./Makefile with unknown extension.
[2025-01-15 18:08:28.178] [info] mmap 7ed7e9d85000 with length 0 to 7ed7e9d85000, total length 8000
[JSILOG] RecordReader::load>> Ignore file ./record_reader_test with unknown extension.
[JSILOG] RecordReader::load>> Ignore file ./data.gpu2.sec-tr.lock with unknown extension.
[JSILOG] RecordReader::load>> Ignore file ./write_trace with unknown extension.
[JSILOG] RecordReader::load>> Ignore file ./cmake_install.cmake with unknown extension.
[2025-01-15 18:08:28.178] [info] process_dir_iter desc: 83790
[2025-01-15 18:08:28.178] [info] thread_dir_iter desc: 83790
[2025-01-15 18:08:28.178] [info] load meta success
[2025-01-15 18:08:28.178] [info] process_dir_iter desc: 83790
[2025-01-15 18:08:28.179] [info] pass read test
[2025-01-15 18:08:28.180] [info] driver exit
[2025-01-15 18:08:28.180] [info] pass
```