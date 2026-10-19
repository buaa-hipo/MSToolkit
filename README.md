<p align="center">
  <img src="doc/assets/mstoolkit-logo.png" alt="MSToolkit — Multi-Scale Toolkit" width="560">
</p>

<h3 align="center">Trace, measure, and understand parallel performance.</h3>

<p align="center">
  <a href="#about">About</a> ·
  <a href="#getting-started">Getting Started</a> ·
  <a href="#documentation">Documentation</a> ·
  <a href="#citation">Citation</a> ·
  <a href="https://github.com/buaa-hipo/MSToolkit/issues">Issues</a>
</p>

<p align="center">
  <a href="LICENSE.txt"><img src="https://img.shields.io/badge/License-BSD_3--Clause-blue.svg" alt="License: BSD 3-Clause"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-20-00599C.svg" alt="C++20">
  <img src="https://img.shields.io/badge/Platform-Linux-555555.svg" alt="Platform: Linux">
</p>

## About

**MSToolkit (Multi-Scale Toolkit)** is a performance measurement and analysis toolkit for parallel applications on high-performance computing systems. It brings together runtime tracing, hardware performance counters, sampling, and offline analysis to help researchers and developers investigate communication, computation, and performance variation.

Developed by [HiPO](https://github.com/buaa-hipo), MSToolkit provides:

- **MPI tracing and calling context.** Capture communication events and backtraces to connect runtime behavior with application code.
- **Hardware counter collection and sampling.** Collect PAPI events and configure periodic or selective measurement through `jsirun`.
- **Heterogeneous execution tracing.** Extend collection with optional OpenMP/OMPT, ROCm, and MATRIX backends, subject to the target platform and runtime.
- **Performance analysis.** Inspect communication behavior, performance variance, scalability, accelerator kernel hotspots, and time-sliced dependency graphs.
- **Trace storage and inspection.** Store measurements through a section-based storage engine and inspect them with command-line readers and analysis tools.

The project retains the `jsi` command names and `JSI_*` environment variables from JSI-Toolkit. The examples below use those existing interfaces.

## Getting Started

### 1. Build and install

Use a **Linux** environment with GCC 12.3 or newer, a CMake 3.x release (at least 3.13), Python 3, and an MPI installation compatible with your application. Install and expose the required libraries first; see the [installation guide](doc/installation.md) for dependencies and platform options.

```bash
git clone https://github.com/buaa-hipo/MSToolkit.git
cd MSToolkit

# Configure a CPU/MPI build and install into this checkout.
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX="$PWD/install" \
  -DUSE_ROCM=OFF \
  -DUSE_MATRIX=OFF \
  -DARCH_HYGON_C86=OFF
cmake --build build --target install --parallel

source env.sh
```

For accelerator collection, follow the [platform-specific configuration](doc/installation.md#optional-platform-support). The repository's `build.sh` currently selects a ROCm/Hygon configuration.

### 2. Collect a trace

Run `jsirun` inside the MPI launcher so each application process is instrumented:

```bash
# Replace ./your_mpi_application with your executable and append its arguments.
mpirun -np 4 jsirun -o "$PWD/measurement" --backtrace -- ./your_mpi_application
```

Use your cluster's launcher and allocation settings as appropriate. All ranks must be able to access the installation, its dependencies, and the measurement directory. Building the application with debug information helps resolve source locations.

### 3. Inspect the result

```bash
jsiread -i measurement -o readable --backtrace
```

Human-readable trace and metadata files are written to `readable/`. Continue with the [usage guide](doc/usage.md) for counter sampling, communication reports, and variance analysis.

## Documentation

| Resource | What you will find |
| --- | --- |
| [Installation](doc/installation.md) | Dependencies, build configuration, MPI setup, and optional backends |
| [Collection and analysis](doc/usage.md) | Tracing, PAPI sampling, analysis commands, and legacy tools |
| [Time-slice analysis](tool/analysis/graph_time_slice_all/README.md) | Host/device events and communication dependency graphs |
| [Memory analysis](tool/analysis/memory_analysis/README.md) | Reports for collected memory events |
| [MATRIX platform](matrix.md) | Platform-specific instrumentation and performance counters (中文) |
| [Storage API](storage.md) | Backends, sections, and trace storage interfaces (中文) |
| [Contributing](contribute.md) | Development workflow, reviews, and testing (中文) |

## Contributing

Contributions to collection backends, analysis tools, documentation, and reproducible examples are welcome. Please read the [contribution guide](contribute.md) before opening a pull request and follow its review and testing requirements.

Report bugs or propose features through [GitHub Issues](https://github.com/buaa-hipo/MSToolkit/issues). For build or runtime issues, include the revision, platform, compiler and MPI versions, configuration, command, and relevant logs.

## Citation

If you use MSToolkit for your research, please cite the relevant papers below:

[SimTrace](https://doi.org/10.1145/3720544) (ACM TACO 2025):

```bibtex
@article{xuan2025simtrace,
  title={{SimTrace}: Exploiting Spatial and Temporal Sampling for Large-Scale Performance Analysis},
  author={Zhibo Xuan and Xin You and Tianyu Feng and Hailong Yang and Zhongzhi Luan and Yi Liu and Depei Qian},
  journal={ACM Transactions on Architecture and Code Optimization},
  year={2025}
}
```

[STAD](https://doi.org/10.1109/TPDS.2025.3566735) (IEEE TPDS 2025):

```bibtex
@article{xuan2025stad,
  title={Identifying Performance Inefficiencies of Parallel Program With Spatial and Temporal Trace Analysis},
  author={Zhibo Xuan and Xin Sun and Xin You and Hailong Yang and Zhongzhi Luan and Yi Liu and Depei Qian},
  journal={IEEE Transactions on Parallel and Distributed Systems},
  year={2025}
}
```

[HeSTEAD](https://doi.org/10.1109/TPDS.2026.3711798) (IEEE TPDS 2026):

```bibtex
@article{xuan2026hestead,
  title={Spatio-Temporal Evolving Anomaly Detection Tool for Large-Scale Heterogeneous Programs Analysis},
  author={Zhibo Xuan and Da Huo and Hailong Yang and Xin You and Genshen Chu and Zhongzhi Luan and Yi Liu and Depei Qian},
  journal={IEEE Transactions on Parallel and Distributed Systems},
  year={2026}
}
```

[GVARP](https://doi.org/10.1109/SC41406.2024.00063) (SC 2024):

```bibtex
@inproceedings{you2024gvarp,
  title={{GVARP}: Detecting Performance Variance on Large-Scale Heterogeneous Systems},
  author={Xin You and Zhibo Xuan and Hailong Yang and Zhongzhi Luan and Yi Liu and Depei Qian},
  booktitle={SC24: International Conference for High Performance Computing, Networking, Storage and Analysis},
  year={2024}
}
```

[ASPA](https://thomas-yang.github.io/) (EuroSys 2027, accepted):

```bibtex
@inproceedings{you2027aspa,
  title={{ASPA}: Auto-parallelizing Performance Trace Analysis with Trait-driven Compilation},
  author={Xin You and Zhibo Xuan and Hailong Yang and Depei Qian},
  booktitle={Proceedings of the European Conference on Computer Systems},
  year={2027},
  note={Accepted for publication}
}
```

Complete bibliographic records are also available in [CITATION.bib](CITATION.bib).

## License

MSToolkit is released under the [BSD 3-Clause License](LICENSE.txt).
