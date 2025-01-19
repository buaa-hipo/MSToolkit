### 1. BUILDING

jsiwrap is a standard CMake project. By default, it builds with GNU make.

```
$ cmake -DDyninst_DIR=path/to/dyninst/lib/cmake/Dyninst
$ make
```

For dependencies, we recommend to use `spack install dyninst` method.

### 2. SETUP

Before jsiwrap is run, certain environment variables need to be set.

For dyninstAPI, the environment variables DYNINSTAPI_RT_LIB and LD_LIBRARY_PATH need to be set appropriately. DYNINSTAPI_RT_LIB needs to be set to the path of the dyninst runtime library file (libdyninstAPI_RT.so e.g. `export DYNINSTAPI_RT_LIB=/path/to/libdyninstAPI_RT.so`) and the environment variable LD_LIBRARY_PATH needs to be updated so that it contains the directory containing the dyninst library (libdyninstAPI.so).

Additionly, LD_LIBRARY_PATH needs to contain the directory containing the papi library.

For the jsiwrap utility, the environment variable TRACETOOL_LIB needs to be set to the path of the jsiwrap tracing library. JSI_MEASUREMENT_DIR needs to be set to the path of the directory for tracing results.

### 3. RUNNING

jsiwrap -i <EXE> -l <LIB> -f <FUNC>

After running, `EXE-rewritten` in X86 or `EXE.dyn` in AARCH64 will be generated in the directory the same as `EXE`.
