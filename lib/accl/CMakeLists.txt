if (USE_CUDA)
    if (USE_ROCM)
      message(FATAL "CUDA and ROCM should not be configured at the same time!")
    endif()
endif()

if (USE_CUDA)
    add_subdirectory(cuda)
endif()

if (USE_ROCM)
    add_subdirectory(rocm)
endif()