# Test report for storage library(20241219 by DSK)

## Test scripts

This test will check whether storage library interface can work.

## Expected outputs

The test script should output with the following outputs. Any `FAILED` outputs indicates the specified test is failed.

For a measurement file ${JSI_MEASUREMENT_DIR}/data.<node-name>.sec-tr, run storage_interface_read_test to validate the trace file.
```
$ ./storage_interface_read_test ./output/data.dcu-1.sec-tr
[2024-12-25 16:20:25.281] [info] open file measurement-bt.B.x-4/data.dcu-1.sec-tr
[2024-12-25 16:20:25.281] [info] mmap 7ef4fc820000 with length 0 to 7ef4fc820000, total length 8000
[2024-12-25 16:20:25.281] [info] iter for process isa dir: true
[2024-12-25 16:20:25.281] [info] mmap 7ef4fc830000 with length 0 to 7ef4fc830000, total length 18000
[2024-12-25 16:20:25.281] [info] event_PROCESS_START = 365
[2024-12-25 16:20:25.281] [info] mmap 7ef4fc868000 with length 0 to 7ef4fc868000, total length 50000
[2024-12-25 16:20:25.281] [info] mmap 7ef4fc8f8000 with length 0 to 7ef4fc8f8000, total length e0000
[2024-12-25 16:20:25.281] [info] event_PROCESS_EXIT = 366
[2024-12-25 16:20:25.281] [info] pass
[2024-12-25 16:20:25.281] [info] driver exit
```