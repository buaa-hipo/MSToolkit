#include <stdint.h>
const char* accl_tracer_get_name(int16_t id);
bool accl_is_launch(int16_t id);
bool accl_is_memcpy(int16_t id);
bool accl_is_memcpy_async(int16_t id);