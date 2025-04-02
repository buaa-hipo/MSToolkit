

#ifndef INSTRUMENTED_FUN_DEV_H
#define INSTRUMENTED_FUN_DEV_H

#include <stdint.h>
#include <stdbool.h>

#define DDR_BASE ((void*)0x880000000)
#define AM_BASE ((void*)0x400000000)
#define SM_BASE ((void*)0x400100400)
#define HBM_BASE ((void*)0x890000000)

extern unsigned long correlation_id[24];

// instrumented dev sync api
void *instrumented_vector_malloc(unsigned int bytes);
int instrumented_vector_free(void *ptr);
int instrumented_vector_load(void *mem, void *buf, unsigned int bytes);
int instrumented_vector_store(void *buf, void *mem, unsigned int bytes);
void *instrumented_scalar_malloc(unsigned int bytes);
int instrumented_scalar_free(void *ptr);
int instrumented_scalar_load(void *mem, void *buf, unsigned int bytes);
int instrumented_scalar_store(void *buf, void *mem, unsigned int bytes);
void *instrumented_hbm_malloc(unsigned long bytes);
void instrumented_hbm_free(void *ptr);

// instrumented dev async api
void instrumented_kernel(const char* name, uint32_t host_pid, uint64_t kernel_cid, uint32_t kid, uint32_t phase);
void instrumented_func(void *func, const char *func_name, unsigned long cid, uint32_t phase);

int instrumented_vector_load_async(void *mem, void *buf, unsigned int bytes);
int instrumented_vector_store_async(void *buf, void *mem, unsigned int bytes);
int instrumented_scalar_load_async(void *mem, void *buf, unsigned int bytes);
int instrumented_scalar_store_async(void *buf, void *mem, unsigned int bytes);

unsigned int instrumented_dma_p2p(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
                     void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
                     bool row_syn, unsigned int synmask);


unsigned int instrumented_dma_broadcast(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
                           void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
                           unsigned core_id, unsigned int barrier_id);

unsigned int instrumented_dma_segment(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
                         void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
                         unsigned int c_start, unsigned int c_num, unsigned int c_step, unsigned int barrier_id);

unsigned int instrumented_dma_sg(void *src_base, void *src_index, unsigned long src_row_num, unsigned int src_row_size,
                        int src_row_step, void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step);

void instrumented_dma_wait(unsigned int ch);
void instrumented_dma_wait_p2p(unsigned int ch_no);
void instrumented_dma_wait_sg(unsigned int ch_no);

void instrumented_group_barrier(unsigned int b_id);
void instrumented_core_barrier(unsigned int b_id, unsigned int num);
void instrumented_core_barrier_wait(unsigned int b_id, unsigned int num, unsigned long wait_clk);
int instrumented_rwlock_try_rdlock(unsigned int lock_id);
int instrumented_rwlock_try_wrlock(unsigned int lock_id);
void instrumented_rwlock_rdlock(unsigned int lock_id);
void instrumented_rwlock_wrlock(unsigned int lock_id);
void instrumented_rwlock_unlock(unsigned int lock_id);

unsigned long instrumented_intr_handler_register(void(*func)(int no));
void instrumented_cpu_interrupt(unsigned long val);

#endif