#pragma once
#include "record/wrap_defines.h"
#include "record/record_type_traits.h"
#include "utils/compile_time.h"
#include "ral/time_detector.h"
#include <string_view>
extern pse::utils::EncodedStruct record_info[];

extern long record_time_offset[];
extern size_t record_info_len;