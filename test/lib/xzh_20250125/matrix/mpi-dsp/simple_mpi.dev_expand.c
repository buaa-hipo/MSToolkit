# 0 "./simple_mpi.dev.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "./simple_mpi.dev.c"
# 1 "/root/xzh/JSI-Toolkit/include/instrument/instrumented_func_dev.h" 1





# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdint.h" 1
# 26 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/features.h" 1
# 187 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/features.h"
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_config.h" 1
# 188 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/features.h" 2
# 416 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/features.h"
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/cdefs.h" 1
# 417 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/features.h" 2
# 27 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdint.h" 2

# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/wchar.h" 1
# 29 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdint.h" 2

# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/wordsize.h" 1
# 31 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdint.h" 2
# 39 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef signed char int8_t;
typedef short int int16_t;
typedef int int32_t;

typedef long int int64_t;







typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;

typedef unsigned int uint32_t;



typedef unsigned long int uint64_t;
# 68 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef signed char int_least8_t;
typedef short int int_least16_t;
typedef int int_least32_t;

typedef long int int_least64_t;






typedef unsigned char uint_least8_t;
typedef unsigned short int uint_least16_t;
typedef unsigned int uint_least32_t;

typedef unsigned long int uint_least64_t;
# 93 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef signed char int_fast8_t;

typedef long int int_fast16_t;
typedef long int int_fast32_t;
typedef long int int_fast64_t;
# 106 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef unsigned char uint_fast8_t;

typedef unsigned long int uint_fast16_t;
typedef unsigned long int uint_fast32_t;
typedef unsigned long int uint_fast64_t;
# 122 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef long int intptr_t;


typedef unsigned long int uintptr_t;
# 137 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef long int intmax_t;
typedef unsigned long int uintmax_t;
# 7 "/root/xzh/JSI-Toolkit/include/instrument/instrumented_func_dev.h" 2
# 1 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stdbool.h" 1 3 4
# 8 "/root/xzh/JSI-Toolkit/include/instrument/instrumented_func_dev.h" 2






extern unsigned long correlation_id[24];


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


void instrumented_kernel(const char* name, uint32_t host_pid, uint64_t kernel_cid, uint32_t kid, uint32_t phase);
void instrumented_func(void *func, const char *func_name, unsigned long cid, uint32_t phase);

int instrumented_vector_load_async(void *mem, void *buf, unsigned int bytes);
int instrumented_vector_store_async(void *buf, void *mem, unsigned int bytes);
int instrumented_scalar_load_async(void *mem, void *buf, unsigned int bytes);
int instrumented_scalar_store_async(void *buf, void *mem, unsigned int bytes);

unsigned int instrumented_dma_p2p(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
                     void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
                     
# 39 "/root/xzh/JSI-Toolkit/include/instrument/instrumented_func_dev.h" 3 4
                    _Bool 
# 39 "/root/xzh/JSI-Toolkit/include/instrument/instrumented_func_dev.h"
                         row_syn, unsigned int synmask);


unsigned int instrumented_dma_broadcast(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
                           void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
                           unsigned core_id, unsigned int barrire_id);

unsigned int instrumented_dma_segment(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
                         void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
                         unsigned int c_start, unsigned int c_num, unsigned int c_step, unsigned int barrire_id);

unsigned int instrumented_dma_sg(void *src_base, void *src_index, unsigned long src_row_num, unsigned int src_row_size,
                        int src_row_step, void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step);

void instrumented_dma_wait(unsigned int ch);
# 2 "./simple_mpi.dev.c" 2
# 1 "/root/xzh/JSI-Toolkit/include/record/mt_double_buffer.h" 1





# 1 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 1 3 4
# 145 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 3 4

# 145 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 3 4
typedef long int ptrdiff_t;
# 214 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 329 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 3 4
typedef unsigned int wchar_t;
# 424 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 3 4
typedef struct {
  long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
  long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
# 435 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 3 4
} max_align_t;
# 7 "/root/xzh/JSI-Toolkit/include/record/mt_double_buffer.h" 2
# 1 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h" 1
# 10 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h"
# 1 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stdarg.h" 1 3 4
# 40 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 99 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stdarg.h" 3 4
typedef __gnuc_va_list va_list;
# 11 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h" 2
# 1 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 1 3 4
# 12 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h" 2
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/time.h" 1
# 25 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/time.h"
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/types.h" 1
# 28 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/types.h"
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/wordsize.h" 1
# 29 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/types.h" 2


# 1 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 1 3 4
# 32 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/types.h" 2



# 34 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/types.h"
typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;







typedef long int __quad_t;
typedef unsigned long int __u_quad_t;
# 134 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/types.h"
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/typesizes.h" 1
# 135 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/types.h" 2


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;

typedef int __daddr_t;
typedef long int __swblk_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;

typedef long int __ssize_t;



typedef __off64_t __loff_t;
typedef __quad_t *__qaddr_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;
# 26 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/time.h" 2

# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/time.h" 1
# 75 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/time.h"


typedef __time_t time_t;



# 28 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/time.h" 2

# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/time.h" 1
# 73 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/time.h"
struct timeval
  {
    __time_t tv_sec;
    __suseconds_t tv_usec;
  };
# 30 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/time.h" 2

# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/select.h" 1
# 31 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/select.h"
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/select.h" 1
# 32 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/select.h" 2


# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/sigset.h" 1
# 23 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/sigset.h"
typedef int __sig_atomic_t;
# 40 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/sigset.h"
typedef struct {
 unsigned long __val[(64 / (8 * sizeof (unsigned long)))];
} __sigset_t;
# 35 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/select.h" 2



typedef __sigset_t sigset_t;





# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/time.h" 1
# 121 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/time.h"
struct timespec
  {
    __time_t tv_sec;
    long int tv_nsec;
  };
# 45 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/select.h" 2

# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/time.h" 1
# 47 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/select.h" 2


typedef __suseconds_t suseconds_t;





typedef long int __fd_mask;
# 67 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/select.h"
typedef struct
  {






    __fd_mask __fds_bits[1024 / (8 * sizeof (__fd_mask))];


  } fd_set;






typedef __fd_mask fd_mask;
# 99 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/select.h"

# 109 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/select.h"
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 121 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/select.h"
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);



# 32 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/time.h" 2








# 57 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/time.h"
struct timezone
  {
    int tz_minuteswest;
    int tz_dsttime;
  };

typedef struct timezone *__restrict __timezone_ptr_t;
# 73 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/time.h"
extern int gettimeofday (struct timeval *__restrict __tv,
    __timezone_ptr_t __tz) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));




extern int settimeofday (__const struct timeval *__tv,
    __const struct timezone *__tz)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));





extern int adjtime (__const struct timeval *__delta,
      struct timeval *__olddelta) __attribute__ ((__nothrow__));




enum __itimer_which
  {

    ITIMER_REAL = 0,


    ITIMER_VIRTUAL = 1,



    ITIMER_PROF = 2

  };



struct itimerval
  {

    struct timeval it_interval;

    struct timeval it_value;
  };






typedef int __itimer_which_t;




extern int getitimer (__itimer_which_t __which,
        struct itimerval *__value) __attribute__ ((__nothrow__));




extern int setitimer (__itimer_which_t __which,
        __const struct itimerval *__restrict __new,
        struct itimerval *__restrict __old) __attribute__ ((__nothrow__));




extern int utimes (__const char *__file, __const struct timeval __tvp[2])
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));



extern int lutimes (__const char *__file, __const struct timeval __tvp[2])
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 193 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/sys/time.h"

# 13 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h" 2
# 27 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h"
__asm__(".section ._gsm,\"aw\",%nobits");
# 44 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h"
int get_group_size();
unsigned int get_group_cores();
int get_thread_id();
int get_core_id();

void group_barrier(unsigned id);




void core_barrier(unsigned int id, unsigned int num);
void core_barrier_wait(unsigned int id, unsigned int num, unsigned long wait_time);


int rwlock_try_rdlock(unsigned int lock_id);
int rwlock_try_wrlock(unsigned int lock_id);
void rwlock_rdlock(unsigned int lock_id);
void rwlock_wrlock(unsigned int lock_id);
void rwlock_unlock(unsigned int lock_id);





void * vector_malloc(unsigned int bytes);
int vector_free(void *ptr);
int vector_load(void *mem, void *buf, unsigned int bytes);
unsigned int vector_load_async(void *mem, void *buf, unsigned int bytes);
int vector_store(void *buf, void *mem, unsigned int bytes);
unsigned int vector_store_async(void *buf, void *mem, unsigned int bytes);
int get_am_free_space ();

void * scalar_malloc(unsigned int bytes);
int scalar_free(void *ptr);
int scalar_load(void *mem, void *buf, unsigned int bytes);
unsigned int scalar_load_async(void *mem, void *buf, unsigned int bytes);
int scalar_store(void *buf, void *mem, unsigned int bytes);
unsigned int scalar_store_async(void *buf, void *mem, unsigned int bytes);
int get_sm_free_space ();





unsigned int dma_p2p(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
      void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
      
# 90 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h" 3 4
     _Bool 
# 90 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h"
          row_syn, unsigned int synmask);

unsigned int dma_broadcast(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
      void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
      unsigned core_id, unsigned int barrire_id);

unsigned int dma_segment(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
      void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
      unsigned int c_start, unsigned int c_num, unsigned int c_step, unsigned int barrire_id);

unsigned int dma_sg(void *src_base, void *src_index, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
      void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step);

void dma_wait(unsigned int ch);


int dma_query(unsigned int ch);
void dma_clear();

unsigned int raw_dma_p2p(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
      void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
      
# 111 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h" 3 4
     _Bool 
# 111 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h"
          row_syn, unsigned int synmask, int prir, int chno);
unsigned int raw_dma_broadcast(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
      void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
      unsigned core_id, unsigned int barrire_id, int total_cores, int prir, int chno);
unsigned int raw_dma_segment(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
      void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
      unsigned int c_start, unsigned int c_num, unsigned int c_step, unsigned int barrire_id, int prir, int chno);
unsigned int raw_dma_sg(void *src_base, void *src_index, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
      void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step, int prir, int chno);
void raw_dma_wait(int tcc);


unsigned int dma_sg_opt(void *src_base, void *src_index, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
             void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step, int ch);

unsigned int dma_p2p_opt(void *src, unsigned long src_row_num, unsigned int src_row_size, int src_row_step,
             void *dst, unsigned long dst_row_num, unsigned int dst_row_size, int dst_row_step,
             
# 128 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h" 3 4
            _Bool 
# 128 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h"
                 row_syn, unsigned int p2pmask, int ch);

void dma_wait_p2p(unsigned int ch_no);
void dma_wait_sg(unsigned int ch_no);
void set_prir(unsigned long val);




void dsp_abort(unsigned int err_no);
void dsp_halt();
void dsp_sleep(unsigned long usec);

void hthread_printf(const char *fmt, ...);
int hthread_sprintf(char* buffer, const char* format, ...);
int hthread_snprintf(char* buffer, size_t count, const char* format, ...);
int hthread_vsnprintf(char* buffer, size_t count, const char* format, va_list va);

void hthread_gettimeofday(struct timeval *tv);

void * hbm_malloc(unsigned long bytes);
void hbm_free(void *ptr);

void trigger_cpu(unsigned long val);
# 182 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h"
void prof_start(int event_id);
unsigned long prof_end(int event_id);
unsigned long prof_read(int event_id);

unsigned long get_clk();




void set_sata_mode (int val);
# 206 "/root/MT3000_env/mt3000_programming_env/hthreads/include/hthread_device.h"
void set_ecr (int EID);
void clear_ecr (int EID);



unsigned long intr_handler_register(void(*func)(int no));

void cpu_interrupt(unsigned long val);
# 8 "/root/xzh/JSI-Toolkit/include/record/mt_double_buffer.h" 2




extern uint32_t *pmu_events;
extern uint32_t pmu_num;


extern volatile void * callback_buffer;
extern volatile void * activity_buffer;
extern volatile uint32_t *callback_used_dev;
extern volatile uint32_t *activity_used_dev;


extern volatile uint32_t *cb_writing_pool_index;
extern volatile uint32_t *cb_ready_pool_index;
extern volatile uint32_t *ac_writing_pool_index;
extern volatile uint32_t *ac_ready_pool_index;

extern uint32_t local_cluster;

void * cb_buffer_alloc(size_t bytes);
void * ac_buffer_alloc(size_t bytes);
void flush_cb_buffer();
void flush_ac_buffer();
void flush_wait();


__attribute__ ((section (".global"))) void double_buffer_init(uint32_t accl_dev_id, void *cb_buf, void *ac_buf, uint32_t *cb_used, uint32_t *ac_used, uint32_t *flags);
__attribute__ ((section (".global"))) void double_buffer_fin();
__attribute__ ((section (".global"))) void dev_pmu_init(uint32_t event_num, uint32_t *event_ids);
# 3 "./simple_mpi.dev.c" 2
# 1 "/root/xzh/JSI-Toolkit/include/record/mt_callback_defs.h" 1
# 10 "/root/xzh/JSI-Toolkit/include/record/mt_callback_defs.h"
typedef enum {
    ACCL_API_ENTER = 0,
    ACCL_API_EXIT = 1
} accl_api_phase_t;





typedef enum {

    ACCL_API_group_create,
    ACCL_API_group_create_masked_launch,
    ACCL_API_group_create_launch,
    ACCL_API_group_exec,
    ACCL_API_group_wait,
    ACCL_API_group_destroy,

    ACCL_API_malloc,
    ACCL_API_free,

    ACCL_API_vector_malloc,
    ACCL_API_vector_free,
    ACCL_API_scalar_malloc,
    ACCL_API_scalar_free,
    ACCL_API_hbm_malloc,
    ACCL_API_hbm_free,

    ACCL_API_vector_load,
    ACCL_API_vector_store,
    ACCL_API_scalar_load,
    ACCL_API_scalar_store,
    ACCL_API_vector_load_async,
    ACCL_API_vector_store_async,
    ACCL_API_scalar_load_async,
    ACCL_API_scalar_store_async,
    ACCL_API_dma_p2p,
    ACCL_API_dma_broadcast,
    ACCL_API_dma_segment,
    ACCL_API_dma_sg,
    ACCL_API_dma_wait,

    ACCL_USER_FUNC,

    ACCL_API_SHMalloc,
} accl_api_op_t;





typedef enum {
    ACCL_ACTIVITY_kernel,
    ACCL_ACTIVITY_vector_load_async,
    ACCL_ACTIVITY_vector_store_async,
    ACCL_ACTIVITY_scalar_load_async,
    ACCL_ACTIVITY_scalar_store_async,
    ACCL_ACIVITY_dma_p2p,
    ACCL_ACIVITY_dma_broadcast,
    ACCL_ACIVITY_dma_segment,
    ACCL_ACIVITY_dma_sg,

    ACCL_HOST_MEM,
    ACCL_HOST_THREAD,
    ACCL_HOST_DRIVER,
    ACCL_DEV_MEM,
    ACCL_DEV_THREAD,
    ACCL_DEV_DRIVER,
    ACCL_DMA,
    ACCL_DEV_USER_FUNC
} accl_api_kind_t;





typedef enum {
    ACCL_DOMAIN_HTHREAD_API,
    ACCL_DOMAIN_HTHREAD_OPS,
    ACCL_DOMAIN_LIBMT,
    ACCL_DOMAIN_COMMON
} accl_api_domain_t;

typedef enum {
 ACCL_MALLOC_FREE,
 ACCL_MALLOC_RO,
 ACCL_MALLOC_WO,
 ACCL_MALLOC_RW,
 ACCL_MALLOC_CACHE
} accl_malloc_mode_t;

typedef enum {
    ACCL_MALLOC_DDR,
    ACCL_MALLOC_HBM,
    ACCL_MALLOC_GSM,
    ACCL_MALLOC_AM,
    ACCL_MALLOC_SM,
    ACCL_FREE
} accl_malloc_kind_t;

typedef enum {
    ACCL_MEMCPY_OFF_AM,
    ACCL_MEMCPY_OFF_SM,
    ACCL_MEMCPY_AM_OFF,
    ACCL_MEMCPY_SM_OFF,
    ACCL_MEMCPY_OFF_OFF
} accl_memcpy_kind_t;

typedef struct {
    accl_api_phase_t phase;
    uint64_t correlation_id;
    union {
        struct {
            int32_t cluster_id;
            uint32_t bytes;
            uint32_t mode;
            uint32_t kind;
            void *address;
        } mem_alloc;
        struct {
            int32_t cluster_id;
            uint32_t thread_num;
            uint32_t kernel_index;
            uint32_t thread_mask;
            uint32_t scalar_args_num;
            uint32_t ptr_args_num;
            int32_t thread_group_id;
        } kernel_info;
        struct {
            const void *dst;
            const void *src;
            uint64_t size_bytes;
            uint32_t kind;
        } mem_sync;
        struct {
            const void *dst;
            const void *src;
            uint64_t size_bytes;
            uint32_t kind;
            uint32_t dma_channel;
        } mem_async;
        struct {
            int32_t cluster_id;
            uint32_t dma_channel;
        } mem_wait;
        struct {
            const void *dst;
            const void *src;
            uint64_t size_bytes;
            uint32_t core_id;
            uint32_t kind;
            uint32_t dma_channel;
        } mem_broadcast;
        struct {
            const void *dst;
            const void *src;
            uint64_t size_bytes;
            uint32_t core_start;
            uint32_t core_num;
            uint32_t step;
            uint32_t kind;
            uint32_t dma_channel;
        } mem_segment;
        struct {
            const void *dst;
            const void *src;
            const void *index;
            uint64_t size_bytes;
            uint32_t kind;
            uint32_t dma_channel;
        } mem_sg;

        struct {
            uint16_t domain;
            uint16_t op;
            uint32_t kind;
            uint32_t process_id;
            uint32_t thread_id;
        } kernel;

    };
} matrix_api_data_t;
# 225 "/root/xzh/JSI-Toolkit/include/record/mt_callback_defs.h"
typedef struct {
    uint16_t domain;
    uint32_t kind;
    uint16_t op;
    struct {
        uint64_t correlation_id;
        uint64_t begin_ns;
        uint64_t end_ns;
    };
    uint32_t process_id;
    uint32_t thread_id;
    uint64_t kernel_index;
} accl_activity_record_t;
# 4 "./simple_mpi.dev.c" 2

# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/compiler/m3000.h" 1
# 6 "./simple_mpi.dev.c" 2
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h" 1
# 30 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"




# 1 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 1 3 4
# 35 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h" 2
# 44 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"


typedef struct __STDIO_FILE_STRUCT FILE;





# 62 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
typedef struct __STDIO_FILE_STRUCT __FILE;
# 72 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h" 1
# 69 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h"
# 1 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 1 3 4
# 70 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h" 2





# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/wchar.h" 1
# 52 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/wchar.h"
# 1 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 1 3 4
# 359 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 3 4

# 359 "/root/spack-0.22.0/opt/spack/linux-ubuntu19.04-aarch64/gcc-8.3.0/gcc-12.3.0-mlrocfzbvbny3ywb3j2oxa7tdqnhtcca/lib/gcc/aarch64-unknown-linux-gnu/12.3.0/include/stddef.h" 3 4
typedef unsigned int wint_t;
# 53 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/wchar.h" 2
# 81 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/wchar.h"

# 81 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/wchar.h"
typedef struct
{
 wchar_t __mask;
 wchar_t __wc;
} __mbstate_t;
# 76 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h" 2
# 107 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h"
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_mutex.h" 1
# 119 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_mutex.h"
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/multi_core.h" 1
# 15 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/multi_core.h"
typedef struct
{
    unsigned int cnt;
    unsigned int owner;
}lock_t;
# 32 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/multi_core.h"
unsigned int getCoreId(void);
void syn_lock(void);
void syn_unlock(void);
# 120 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_mutex.h" 2
# 108 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h" 2
# 165 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h"
typedef struct {
 __off_t __pos;

 __mbstate_t __mbstate;


 int __mblen_pending;

} __STDIO_fpos_t;
# 191 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h"
typedef __off_t __offmax_t;
# 228 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h"
struct __STDIO_FILE_STRUCT {
 unsigned short __modeflags;


 unsigned char __ungot_width[2];






 int __filedes;

 unsigned char *__bufstart;
 unsigned char *__bufend;
 unsigned char *__bufpos;
 unsigned char *__bufread;
# 256 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h"
 struct __STDIO_FILE_STRUCT *__nextopen;






 wchar_t __ungot[2];


 __mbstate_t __state;
# 279 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h"
};
# 356 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_stdio.h"
extern int __fgetc_unlocked(FILE *__stream);
extern int __fputc_unlocked(int __c, FILE *__stream);
# 73 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h" 2








typedef __STDIO_fpos_t fpos_t;




# 131 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
# 1 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/bits/stdio_lim.h" 1
# 132 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h" 2



extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;







extern int remove (__const char *__filename) __attribute__ ((__nothrow__));

extern int rename (__const char *__old, __const char *__new) __attribute__ ((__nothrow__));




extern int renameat (int __oldfd, __const char *__old, int __newfd,
       __const char *__new) __attribute__ ((__nothrow__));








extern FILE *tmpfile (void) ;
# 177 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern char *tmpnam (char *__s) __attribute__ ((__nothrow__)) ;






extern char *tmpnam_r (char *__s) __attribute__ ((__nothrow__)) ;
# 196 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern char *tempnam (__const char *__dir, __const char *__pfx)
     __attribute__ ((__nothrow__)) __attribute__ ((__malloc__)) ;








extern int fclose (FILE *__stream);




extern int fflush (FILE *__stream);

# 221 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern int fflush_unlocked (FILE *__stream);
# 235 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"






extern FILE *fopen (__const char *__restrict __filename,
      __const char *__restrict __modes) ;




extern FILE *freopen (__const char *__restrict __filename,
        __const char *__restrict __modes,
        FILE *__restrict __stream) ;
# 264 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"

# 275 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern FILE *fdopen (int __fd, __const char *__modes) __attribute__ ((__nothrow__)) ;
# 299 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"



extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) __attribute__ ((__nothrow__));



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) __attribute__ ((__nothrow__));





extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
         size_t __size) __attribute__ ((__nothrow__));


extern void setlinebuf (FILE *__stream) __attribute__ ((__nothrow__));








extern int fprintf (FILE *__restrict __stream,
      __const char *__restrict __format, ...);




extern int printf (__const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      __const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3)));





extern int vfprintf (FILE *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg);




extern int vprintf (__const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 0)));





extern int snprintf (char *__restrict __s, size_t __maxlen,
       __const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        __const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 0)));

# 397 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"





extern int fscanf (FILE *__restrict __stream,
     __const char *__restrict __format, ...)
     __attribute__ ((__format__ (__scanf__, 2, 3))) ;




extern int scanf (__const char *__restrict __format, ...)
     __attribute__ ((__format__ (__scanf__, 1, 2))) ;

extern int sscanf (__const char *__restrict __s,
     __const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__scanf__, 2, 3)));








extern int vfscanf (FILE *__restrict __s, __const char *__restrict __format,
      __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 2, 0))) ;





extern int vscanf (__const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 1, 0))) ;


extern int vsscanf (__const char *__restrict __s,
      __const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__scanf__, 2, 0)));









extern int fgetc (FILE *__stream);
extern int getc (FILE *__stream);





extern int getchar (void);

# 466 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern int getc_unlocked (FILE *__stream);
extern int getchar_unlocked (void);
# 480 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern int fgetc_unlocked (FILE *__stream);











extern int fputc (int __c, FILE *__stream);
extern int putc (int __c, FILE *__stream);





extern int putchar (int __c);

# 513 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern int fputc_unlocked (int __c, FILE *__stream);







extern int putc_unlocked (int __c, FILE *__stream);
extern int putchar_unlocked (int __c);
# 532 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern int getw (FILE *__stream);


extern int putw (int __w, FILE *__stream);








extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
     ;






extern char *gets (char *__s) ;

# 599 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"





extern int fputs (__const char *__restrict __s, FILE *__restrict __stream);





extern int puts (__const char *__s);






extern int ungetc (int __c, FILE *__stream);






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream) ;




extern size_t fwrite (__const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s) ;

# 652 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
         size_t __n, FILE *__restrict __stream) ;
extern size_t fwrite_unlocked (__const void *__restrict __ptr, size_t __size,
          size_t __n, FILE *__restrict __stream) ;








extern int fseek (FILE *__stream, long int __off, int __whence);




extern long int ftell (FILE *__stream) ;




extern void rewind (FILE *__stream);

# 688 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern int fseeko (FILE *__stream, __off_t __off, int __whence);




extern __off_t ftello (FILE *__stream) ;
# 707 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"






extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos);




extern int fsetpos (FILE *__stream, __const fpos_t *__pos);
# 730 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"

# 739 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"


extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__)) ;

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__)) ;




extern void clearerr_unlocked (FILE *__stream) __attribute__ ((__nothrow__));
extern int feof_unlocked (FILE *__stream) __attribute__ ((__nothrow__)) ;
extern int ferror_unlocked (FILE *__stream) __attribute__ ((__nothrow__)) ;








extern void perror (__const char *__s);

# 776 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern int fileno (FILE *__stream) __attribute__ ((__nothrow__)) ;




extern int fileno_unlocked (FILE *__stream) __attribute__ ((__nothrow__)) ;
# 791 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern FILE *popen (__const char *__command, __const char *__modes) ;





extern int pclose (FILE *__stream);





extern char *ctermid (char *__s) __attribute__ ((__nothrow__));
# 831 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"
extern void flockfile (FILE *__stream) __attribute__ ((__nothrow__));



extern int ftrylockfile (FILE *__stream) __attribute__ ((__nothrow__)) ;


extern void funlockfile (FILE *__stream) __attribute__ ((__nothrow__));
# 885 "/root/MT3000_env/mt3000_programming_env/dsp_compiler/include/stdio.h"

# 7 "./simple_mpi.dev.c" 2

typedef struct {
    int a, b, c;
    unsigned long d;
} __attribute__((aligned(1)))type_s;

void test_func() {
    hthread_printf("?\n");
}

__attribute__ ((section (".global"))) void reduce_kernel(unsigned int host_pid, unsigned long long kernel_cid, unsigned int kernel_index, long size, long *array, long *result) {
instrumented_kernel("reduce_kernel", host_pid, kernel_cid, kernel_index, ACCL_API_ENTER);
    test_func();
    hthread_printf("sizeof(type_s) = %lu\n", sizeof(type_s));
    long *cache = (long*)instrumented_scalar_malloc(size * sizeof(long));
    hthread_printf("Got size: %ld\n", size);
    fflush(stdout);
    instrumented_scalar_load(array, cache, sizeof(long) * size);

    long *res = (long*)instrumented_scalar_malloc(sizeof(long));
    *res = 0;
    for (int i = 0; i < size; ++i) {
        res[0] += cache[i];
    }

    instrumented_scalar_store(res, result, sizeof(long));
    hthread_printf("result = %ld\n", *result);
    instrumented_scalar_free(cache);
    instrumented_scalar_free(res);
instrumented_kernel("reduce_kernel", host_pid, kernel_cid, kernel_index, ACCL_API_EXIT);
}
