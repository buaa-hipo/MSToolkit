## Memory Analysis

### usage

The function of `memory_analysis` is to analyze the collected memory-related events. Its functions include memory usage, memory occupancy, and memory leak detection.

```
memory_analysis need data collected by memory_wrapper
Usage:
  memory_analysis [OPTION...]

  -f, --force               Force to overwrite the output file if already 
                            exists.
  -p, --pretty_print        Print readable report in stdout.
  -h, --help                Print help
  -i, --input arg           data collected by memory_wrapper
  -d, --line_info_dump arg  Input line info directory dumped by 
                            dwarf_line_info_dump
  -o, --output arg          Output dir to store the chrome trace outputs 
                            after analysis
```

### example

For the collected data, you can use `memory_analysis` to process

```
memory_analysis -f -p --input trace/collect --line_info_dump trace/line-info --output trace/analysis
```

You can then get the output memory leak information and a csv file of memory usage.