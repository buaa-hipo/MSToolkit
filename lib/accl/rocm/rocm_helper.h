#include <stddef.h>
#include <unistd.h>

extern bool jsi_pmu_enabled;
extern bool jsi_backtrace_enabled;
extern int jsi_pmu_num;

size_t  writeStringSection      (const char *str);
void    helperExtStore          (size_t id, const void* record, size_t size);
void    helperTraceStore        (void* record);
void    rocmMetaSet             ();

// For alloca.
#ifdef USE_ALLOCA

#include <malloc.h>
#define ALLOCATE(size) alloca(size)
#define DEALLOCATE(ptr,size) 

#else

#define ALLOCATE(size) malloc(size)
#define DEALLOCATE(ptr,size) free(ptr)

#endif


// For backtrace related fucntions.
#include <stdint.h>
typedef int32_t backtrace_context_t;
extern backtrace_context_t backtrace_context_get(int);