// //
// // Created by xiaox on 2025/1/12.
// //

// #ifndef MT_HOST_TYPES_H
// #define MT_HOST_TYPES_H

// #include <cstdint>
// #include "record/record_type.h"

// // MATRIX api record types
// typedef struct {
//     record_t record;
//     uint64_t correlation_id;
// } accl_record_common_t;

// typedef struct {
//     record_t record;
//     uint64_t correlation_id;
//     int32_t cluster_id;
//     uint32_t thread_num;
//     uint32_t thread_mask;
//     uint32_t scalar_args_num;
//     uint32_t ptr_args_num;
//     int32_t group_id;
// } mt_record_kernel_launch_t;

// typedef struct {
//     record_t record;
//     uint64_t correlation_id;
//     int32_t group_id;
// } mt_record_group_wait_t;

// typedef struct {
//     record_t record;
//     uint64_t correlation_id;
//     int32_t cluster_id;
//     uint32_t bytes;
//     uint32_t mode;
//     uint32_t kind;
//     void *address;
// } mt_record_malloc_t;

// typedef struct {
//   record_t record;
//   uint64_t correlation_id;
//   void *address;
// } mt_record_free_t;

// #endif //MT_HOST_TYPES_H
