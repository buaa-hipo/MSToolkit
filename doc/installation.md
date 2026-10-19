# Installation

[Back to the project overview](../README.md)

MSToolkit targets Linux HPC environments. Use the same MPI implementation and compatible runtime libraries as the application you intend to measure.

## Dependencies

| Component | Requirement or version guidance |
| --- | --- |
| Compiler | GCC 12.3 or newer, with C++20 and OpenMP support |
| Build tools | CMake 3.x, at least 3.13; Make or another CMake generator; pkg-config |
| Scripting | Python 3 and Bash |
| MPI | Headers and libraries matching the target application |
| Measurement and symbols | PAPI, libunwind, libelf, and libdwarf (the existing setup uses 0.11.1) |
| C++ libraries | Boost 1.86.0, range-v3 0.12.0, fmt 10.2.1, spdlog 1.14.1, magic-enum 0.9.6 |
| Storage | SQLite 3.40.1 |

The library versions above retain the project's existing dependency pins; they are not a claim that other versions are incompatible. Use a CMake 3.x release because some bundled subprojects still declare older CMake policy versions.

OTF2 (previously documented as 3.0.3) is used by the optional conversion tools. Dyninst is used by the optional binary instrumentation wrapper. These tools are currently excluded from the default build; neither is needed for the basic workflow in the README.

### Install dependencies with Spack

If your cluster provides an environment module or a Spack installation, use that installation. With Spack initialized, the following commands cover the core third-party libraries:

```bash
spack install papi libunwind libdwarf@0.11.1 elfutils
spack install boost@1.86.0 sqlite@3.40.1 range-v3@0.12.0 \
  fmt@10.2.1 spdlog@1.14.1 magic-enum@0.9.6

spack load papi libunwind libdwarf@0.11.1 elfutils
spack load boost@1.86.0 sqlite@3.40.1 range-v3@0.12.0 \
  fmt@10.2.1 spdlog@1.14.1 magic-enum@0.9.6
```

Load your compiler and MPI environment separately. If multiple Spack installations of a package exist, select the intended concretized spec or hash when loading it.

For a custom PAPI installation, set `PAPI_PATH` to its prefix. For dependencies installed outside system paths, add their prefixes to `CMAKE_PREFIX_PATH` and expose their shared libraries through the site's module environment or `LD_LIBRARY_PATH`.

### Configure MPI and dependency discovery

The current build locates `mpi.h` through `C_INCLUDE_PATH` and the MPI library through `LIBRARY_PATH`. If your module environment does not expose them, set these paths before configuring:

```bash
# Replace this path with the MPI installation used by your application.
export MSTOOLKIT_MPI_PREFIX=/path/to/mpi
export C_INCLUDE_PATH="$MSTOOLKIT_MPI_PREFIX/include${C_INCLUDE_PATH:+:$C_INCLUDE_PATH}"
export LIBRARY_PATH="$MSTOOLKIT_MPI_PREFIX/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"
export LD_LIBRARY_PATH="$MSTOOLKIT_MPI_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
```

Use `lib64` instead of `lib` where appropriate. You can also pass `-DMPI_INCLUDE_PATH=/path/to/mpi/include` and `-DMPI_LIBRARIES=/path/to/libmpi.so` directly to CMake.

For libdwarf installations that place `libdwarf.h` in a versioned subdirectory, pass `-DLIBDWARF_INCLUDE_DIRS=/path/to/headers` and, if needed, `-DLIBDWARF_LIBRARIES=/path/to/libdwarf.so`.

SQLite and magic-enum also need to be discoverable through pkg-config. Prefer exposing dependency prefixes through `CMAKE_PREFIX_PATH`; the current CMake files overwrite `PKG_CONFIG_PATH`, so setting only that environment variable may not be sufficient.

## Build from source

From the repository root, after loading the dependencies:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX="$PWD/install" \
  -DUSE_ROCM=OFF \
  -DUSE_MATRIX=OFF \
  -DARCH_HYGON_C86=OFF
cmake --build build --target install --parallel
source env.sh
```

`env.sh` adds the checkout's `install/bin` and `install/lib` directories to the environment. For a custom installation prefix, add that prefix's `bin` and `lib` directories yourself. Keep the dependency environment available when running and analyzing applications.

## Optional platform support

| Feature | Build configuration | Runtime configuration |
| --- | --- | --- |
| ROCm accelerator tracing | `-DUSE_ROCM=ON`; CMake 3.21+ in the 3.x series and a compatible HIP/ROCm SDK; set `ROCM_PATH` before configuring | Run `jsirun --accl` |
| Hygon C86 | `-DARCH_HYGON_C86=ON` on the matching processor | Use the platform's compiler and counter environment |
| MATRIX | `-DUSE_MATRIX=ON` and the platform SDK | Follow the [MATRIX guide](../matrix.md) |
| OpenMP tracing | An LLVM OpenMP runtime with OMPT support; set `LLVMOpenMP_DIR` to its prefix and `LLVMOpenMP_INCLUDE_DIR` to the directory containing `ompt.h` | Keep `LLVMOpenMP_DIR` set; run `jsirun --collect_omp` |

The ROCm backend contains platform-specific SDK settings. Check [its build configuration](../lib/accl/rocm/CMakeLists.txt) against your installation before enabling it. Use a separate build directory when switching platform configurations.

The convenience script [`build.sh`](../build.sh) currently enables ROCm and Hygon C86. The explicit CPU/MPI configuration above is the starting point for other Linux systems.
