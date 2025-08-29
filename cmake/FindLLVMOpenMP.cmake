# FindLLVMOpenMP.cmake
#
# This module finds if LLVM OpenMP is installed and sets the necessary variables
#
# It defines the following variables:
#  LLVMOpenMP_FOUND - Set to true if LLVM OpenMP is found
#  LLVMOpenMP_INCLUDE_DIRS - Include directories for LLVM OpenMP
#  LLVMOpenMP_LIBRARIES - Libraries for LLVM OpenMP
message(STATUS "LLVMOpenMP_DIR: $ENV{LLVMOpenMP_DIR}")
message(STATUS "Searching for ompt.h in ${LLVMOpenMP_DIR}/include")
message(STATUS "Searching for libomp.so in ${LLVMOpenMP_DIR}/lib")

find_path(LLVMOpenMP_INCLUDE_DIR
  NAMES ompt.h
  HINTS $ENV{LLVMOpenMP_INCLUDE_DIR}
  PATH_SUFFIXES include
  DOC "Path to the LLVM OpenMP include directory"
)

find_library(LLVMOpenMP_LIBRARY
  NAMES omp
  HINTS $ENV{LLVMOpenMP_DIR}
  PATH_SUFFIXES lib
  DOC "Path to the LLVM OpenMP library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LLVMOpenMP DEFAULT_MSG
  LLVMOpenMP_INCLUDE_DIR LLVMOpenMP_LIBRARY)

if(LLVMOpenMP_FOUND)
  set(LLVMOpenMP_INCLUDE_DIRS ${LLVMOpenMP_INCLUDE_DIR})
  set(LLVMOpenMP_LIBRARIES ${LLVMOpenMP_LIBRARY})
  message(STATUS "Found ompt.h in ${LLVMOpenMP_INCLUDE_DIRS}")
  message(STATUS "Found libomp.so in ${LLVMOpenMP_LIBRARIES}")
else()
  set(LLVMOpenMP_INCLUDE_DIRS)
  set(LLVMOpenMP_LIBRARIES)
endif()

mark_as_advanced(LLVMOpenMP_INCLUDE_DIR LLVMOpenMP_LIBRARY)