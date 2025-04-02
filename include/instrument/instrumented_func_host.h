
#ifndef INSTRUMENTED_FUNC_HOST
#define INSTRUMENTED_FUNC_HOST

#include <cstdint>
// instrumented host apis
void* instrumented_hthread_malloc(int, int, int);
void instrumented_hthread_free(void*);
int instrumented_hthread_group_create6(int, int, const char*, int, int, uint64_t*);
int instrumented_hthread_group_create2(int, int);
int instrumented_hthread_group_masked_create(int, uint32_t, const char*, int, int, uint64_t*);
int instrumented_hthread_group_exec(int, const char*, int, int, uint64_t*);
int instrumented_hthread_group_wait(int);
int instrumented_hthread_group_destroy(int);
int instrumented_hthread_dat_load(int, const char *);
int instrumented_hthread_dat_unload(int);
int instrumented_hthread_dev_open(int);
int instrumented_hthread_dev_close(int);

int instrumented_hthread_dev_owner(int);
int instrumented_hthread_group_get_status(int);
int instrumented_hthread_barrier_create(int);
void instrumented_hthread_barrier_destroy(int);
int instrumented_hthread_rwlock_create(int);
void instrumented_hthread_rwlock_destroy(int);
void instrumented_hthread_intr_send(int g_id, int t_id, unsigned long intr_id);
unsigned long instrumented_hthread_handler_register(int thread_id, void (*func)(int id, unsigned long val));

#endif