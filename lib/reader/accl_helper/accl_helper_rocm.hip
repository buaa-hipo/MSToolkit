#include "record/accl_helper.h"
#include <roctracer.h>
#include <roctracer_hip.h>

const char* accl_tracer_get_name(int16_t id) {
    return hip_api_name(id);
}

bool accl_is_launch(int16_t id) {
    return id==HIP_API_ID_hipLaunchKernel;
}

bool accl_is_memcpy(int16_t id) {
    return id==HIP_API_ID_hipMemcpy;
}

bool accl_is_memcpy_async(int16_t id) {
    return id==HIP_API_ID_hipMemcpyAsync;
}