# Test report for storage library(20241219 by DSK)

## Test scripts

This test will check whether storage library interface can work.

## Expected outputs

The test script should output with the following outputs. Any `FAILED` outputs indicates the specified test is failed.

First, run storage_interface_write_test to generate the trace file.
```
$ JSI_MEASUREMENT_DIR=<path-to-trace-dir> ./storage_interface_write_test
[JSILOG] Initialize Record Writer Library.
[2024-12-19 12:38:18.568] [info] mmap 7ea3b43ee000 with length 0 to 7ea3b43ee000, total length 8000
```
A file ${JSI_MEASUREMENT_DIR}/data.<node-name>_<pid>.node.tr will be created.
Then, run storage_interface_read_test to validate the trace file.
```
$ ./storage_interface_read_test ./output/data.gpu2_29737.node.tr
[2024-12-19 12:40:55.645] [info] open file ./output/data.gpu2_29737.node.tr
[2024-12-19 12:40:55.645] [info] mmap 7e0ca146a000 with length 0 to 7e0ca146a000, total length 8000
[2024-12-19 12:40:55.646] [info] iter for process isa dir: true
[2024-12-19 12:40:55.646] [info] pass
[2024-12-19 12:40:55.646] [info] driver exit
```