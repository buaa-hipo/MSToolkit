# Collection and analysis

[Back to the project overview](../README.md)

Complete the [installation](installation.md) and source `env.sh` before running these examples. Replace application names, rank counts, and paths with those for your workload. Use a new measurement directory for each run.

## Collect MPI traces

```bash
mpirun -np 4 jsirun -o "$PWD/measurement" --backtrace -- ./your_mpi_application
jsiread -i measurement -o readable --backtrace
```

`--backtrace` records calling context. `jsiread` writes readable trace and metadata files into the output directory. On a cluster, use a shared measurement path and ensure that every rank can load the same toolkit and dependency libraries.

## Collect hardware counters

Use `papi_avail` to inspect the counters supported by the target system. Event availability depends on the processor and its counter configuration.

```bash
mpirun -np 4 jsirun -o "$PWD/measurement-pmu" --backtrace \
  --events PAPI_TOT_INS,PAPI_TOT_CYC -- ./your_mpi_application
```

`--events` enables PMU collection and accepts a comma-separated list of PAPI event names.

For periodic sampling, enable the sampling library and set an interval in seconds:

```bash
mpirun -np 4 jsirun -o "$PWD/measurement-sampled" \
  --enable_sample --samp_interval 1.2 \
  --events PAPI_TOT_INS,PAPI_TOT_CYC -- ./your_mpi_application
```

The sampling library must be present in the installation for `--enable_sample` to work.

## Analyze communication

Communication analysis requires backtraces. Select the section-based data model explicitly for traces from the current collector:

```bash
comm_analysis -i measurement -m 1 -o comm-report
```

The `comm-report/` directory contains reports such as `result.long_wait`, `result.high_prop`, `result.high_vol`, `result.high_freq`, and `result.comm_mtx`. Run `comm_analysis --help` to inspect thresholds and filtering options.

## Analyze performance variance

Collect backtraces and `PAPI_TOT_INS` as in the counter example above. This is the default reference metric used by the variance analyzer.

```bash
variance_analysis -i measurement-pmu -o variance --enable-detail
```

Use `--reference-metric EVENT_NAME` if you collected a different reference event. Analysis writes CSV files into `variance/`. The `--enable-detail` option produces the `comm.csv` and `calc.csv` files consumed by the heatmap script.

The heatmap script requires Python 3 with NumPy, SciPy, and Matplotlib. From the repository root:

```bash
python3 scripts/analysis/variance/var_heatmap.py \
  --input variance --output heatmaps --comm --calc
```

Use `--time` to limit the plotted duration in milliseconds, or `--node_level` to aggregate by host. See the script's `--help` for smoothing, resolution, and accelerator plotting options.

## Additional analysis tools

The default build also includes `timeslice_analysis_all`, `memory_analysis`, `kernel_hotspot`, `scalability_analysis`, and `dwarf_line_info_dump`. Each tool requires the corresponding event data; use its `--help` for inputs and options.

- [Time-slice analysis](../tool/analysis/graph_time_slice_all/README.md) combines event information and communication dependencies across host and device traces.
- [Memory analysis](../tool/analysis/memory_analysis/README.md) processes memory events collected with the appropriate instrumentation.
- [MATRIX instrumentation](../matrix.md) documents the platform-specific collection workflow.

## Legacy and optional tools

Some tools described in earlier versions of the README remain in the source tree but are **excluded from the default build**:

| Tool | Purpose | Source |
| --- | --- | --- |
| `graph_analysis` | Dependency-annotated Chrome trace JSON | [Graph analysis](../tool/analysis/graph/) |
| `jsiextract` | Export trace events to OTF2 | [Conversion tools](../tool/jsiconvert/) |
| `jsiwrap` | Dyninst-based binary instrumentation | [Wrapper guide](../tool/jsiwrap/README.md) |

Their build entries are disabled in [`tool/CMakeLists.txt`](../tool/CMakeLists.txt) or [`tool/analysis/CMakeLists.txt`](../tool/analysis/CMakeLists.txt). Check their dependencies and trace-format compatibility before enabling them; they are not required for the quickstart.
