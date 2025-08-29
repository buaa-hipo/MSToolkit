# 1 "dsp_dslash_spinor.h"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "dsp_dslash_spinor.h"
# 1 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/instrument/instrumented_func_dev.h" 1





# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/stdint.h" 1
# 26 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/features.h" 1
# 187 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/features.h"
# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/uClibc_config.h" 1
# 188 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/features.h" 2
# 416 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/features.h"
# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/cdefs.h" 1
# 417 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/features.h" 2
# 27 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/stdint.h" 2

# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/wchar.h" 1
# 29 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/stdint.h" 2

# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/wordsize.h" 1
# 31 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/stdint.h" 2
# 39 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef signed char int8_t;
typedef short int int16_t;
typedef int int32_t;

typedef long int int64_t;







typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;

typedef unsigned int uint32_t;



typedef unsigned long int uint64_t;
# 68 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef signed char int_least8_t;
typedef short int int_least16_t;
typedef int int_least32_t;

typedef long int int_least64_t;






typedef unsigned char uint_least8_t;
typedef unsigned short int uint_least16_t;
typedef unsigned int uint_least32_t;

typedef unsigned long int uint_least64_t;
# 93 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef signed char int_fast8_t;

typedef long int int_fast16_t;
typedef long int int_fast32_t;
typedef long int int_fast64_t;
# 106 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef unsigned char uint_fast8_t;

typedef unsigned long int uint_fast16_t;
typedef unsigned long int uint_fast32_t;
typedef unsigned long int uint_fast64_t;
# 122 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef long int intptr_t;


typedef unsigned long int uintptr_t;
# 137 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/stdint.h"
typedef long int intmax_t;
typedef unsigned long int uintmax_t;
# 7 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/instrument/instrumented_func_dev.h" 2
# 1 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stdbool.h" 1 3 4
# 8 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/instrument/instrumented_func_dev.h" 2






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
                     
# 39 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/instrument/instrumented_func_dev.h" 3 4
                    _Bool 
# 39 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/instrument/instrumented_func_dev.h"
                         row_syn, unsigned int synmask);


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
# 2 "dsp_dslash_spinor.h" 2
# 1 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/record/mt_buffer.h" 1




# 1 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stddef.h" 1 3 4
# 149 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stddef.h" 3 4

# 149 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stddef.h" 3 4
typedef long int ptrdiff_t;
# 216 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 328 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stddef.h" 3 4
typedef int wchar_t;
# 426 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stddef.h" 3 4
typedef struct {
  long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
  long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
# 437 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stddef.h" 3 4
} max_align_t;
# 6 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/record/mt_buffer.h" 2


# 1 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h" 1
# 10 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h"
# 1 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stdarg.h" 1 3 4
# 40 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 99 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stdarg.h" 3 4
typedef __gnuc_va_list va_list;
# 11 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h" 2
# 1 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stddef.h" 1 3 4
# 12 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h" 2
# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/time.h" 1
# 25 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/time.h"
# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/types.h" 1
# 28 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/types.h"
# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/wordsize.h" 1
# 29 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/types.h" 2


# 1 "/thfs3/software/programming_env/mt3000_programming_env_202312/dsp_compiler/lib/gcc/tic6x-elf/8.3.0/include/stddef.h" 1 3 4
# 32 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/types.h" 2



# 34 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/types.h"
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
# 134 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/types.h"
# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/typesizes.h" 1
# 135 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/types.h" 2


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
# 26 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/time.h" 2

# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/time.h" 1
# 75 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/time.h"


typedef __time_t time_t;



# 28 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/time.h" 2

# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/time.h" 1
# 73 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/time.h"
struct timeval
  {
    __time_t tv_sec;
    __suseconds_t tv_usec;
  };
# 30 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/time.h" 2

# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/select.h" 1
# 31 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/select.h"
# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/select.h" 1
# 32 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/select.h" 2


# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/sigset.h" 1
# 23 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/sigset.h"
typedef int __sig_atomic_t;
# 40 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/sigset.h"
typedef struct {
 unsigned long __val[(64 / (8 * sizeof (unsigned long)))];
} __sigset_t;
# 35 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/select.h" 2



typedef __sigset_t sigset_t;





# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/time.h" 1
# 121 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/time.h"
struct timespec
  {
    __time_t tv_sec;
    long int tv_nsec;
  };
# 45 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/select.h" 2

# 1 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/bits/time.h" 1
# 47 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/select.h" 2


typedef __suseconds_t suseconds_t;





typedef long int __fd_mask;
# 67 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/select.h"
typedef struct
  {






    __fd_mask __fds_bits[1024 / (8 * sizeof (__fd_mask))];


  } fd_set;






typedef __fd_mask fd_mask;
# 99 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/select.h"

# 109 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/select.h"
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 121 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/select.h"
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);



# 32 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/time.h" 2








# 57 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/time.h"
struct timezone
  {
    int tz_minuteswest;
    int tz_dsttime;
  };

typedef struct timezone *__restrict __timezone_ptr_t;
# 73 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/time.h"
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
# 193 "/thfs3/software/programming_env/mt3000_programming_env/dsp_compiler/include/sys/time.h"

# 13 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h" 2
# 27 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h"
__asm__(".section ._gsm,\"aw\",%nobits");
# 44 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h"
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
      
# 90 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h" 3 4
     _Bool 
# 90 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h"
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
      
# 111 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h" 3 4
     _Bool 
# 111 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h"
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
             
# 128 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h" 3 4
            _Bool 
# 128 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h"
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
# 182 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h"
void prof_start(int event_id);
unsigned long prof_end(int event_id);
unsigned long prof_read(int event_id);

unsigned long get_clk();




void set_sata_mode (int val);
# 206 "/thfs3/home/yanghailong/mt3000_programming_env/include/hthread_device.h"
void set_ecr (int EID);
void clear_ecr (int EID);



unsigned long intr_handler_register(void(*func)(int no));

void cpu_interrupt(unsigned long val);
# 9 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/record/mt_buffer.h" 2





extern uint32_t *pmu_events;
extern uint32_t pmu_num;
extern 
# 16 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/record/mt_buffer.h" 3 4
      _Bool 
# 16 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/record/mt_buffer.h"
            enable_pmu;


extern void *callback_buffer[24];
extern void *activity_buffer[24];
extern uint32_t *callback_used_dev[24];

extern uint32_t *activity_used_dev[24];


extern uint32_t local_cluster;

void *cb_buffer_alloc(size_t bytes);
void *ac_buffer_alloc(size_t bytes);
void flush_cb_buffer();
void flush_ac_buffer();
void flush_wait();


__attribute__ ((section (".global"))) void buffer_init(uint32_t accl_dev_id, void *cb_buf, void *ac_buf, uint32_t *cb_used,
                                   uint32_t *ac_used);
__attribute__ ((section (".global"))) void buffer_fin();
__attribute__ ((section (".global"))) void dev_pmu_init(uint32_t enable_dev_pmu, uint32_t event_num, uint32_t *event_ids);
# 3 "dsp_dslash_spinor.h" 2
# 1 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/record/mt_callback_defs.h" 1
# 10 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/record/mt_callback_defs.h"
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
    ACCL_API_dma_wait_p2p,
    ACCL_API_dma_wait_sg,

    ACCL_USER_FUNC,
    ACCL_API_dat_load,
    ACCL_API_dat_unload,
    ACCL_API_dev_close,
    ACCL_API_dev_open,
    ACCL_API_dev_owner,
    ACCL_API_group_get_status,
    ACCL_API_barrier_create,
    ACCL_API_barrier_destroy,
    ACCL_API_rwlock_create,
    ACCL_API_rwlock_destroy,
    ACCL_API_intr_send,
    ACCL_API_intr_reg,
    ACCL_API_group_barrier,
    ACCL_API_core_barrier,
    ACCL_API_core_barrier_wait,
    ACCL_API_rwlock_try_rdlock,
    ACCL_API_rwlock_try_wrlock,
    ACCL_API_rwlock_rdlock,
    ACCL_API_rwlock_wrlock,
    ACCL_API_rwlock_unlock,
    ACCL_API_intr_handler_register,
    ACCL_API_cpu_interrupt,


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
    struct {
        uint64_t timestamp;
        uint64_t pmu[26];
    } BufferNode;
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
            uint32_t cluster_id;
            uint32_t kind;
        } dev;
        struct {
            uint32_t cluster_id;
            int32_t barrier_id;
            uint32_t kind;
        } barrier;
        struct {
            int32_t barrier_id;
            uint32_t core_num;
            uint64_t timeout;
        } dev_barrier;
        struct {
            uint32_t cluster_id;
            int32_t lock_id;
            uint32_t kind;
        } rw_lock;
        struct {
            uint32_t lock_id;
            uint32_t op_kind;
        } dev_rw_lock;
        struct {
            uint32_t group_id;
            uint32_t thread_id;
            uint64_t intr_id;
            const void* func;
        } intr;

        struct {
            uint16_t domain;
            uint16_t op;
            uint32_t kind;
            uint32_t process_id;
            uint32_t thread_id;
        } kernel;

    };
} matrix_api_data_t;
# 283 "/thfs3/home/yanghailong/xzh/JSI-Toolkit-mpich-single-buffer/include/record/mt_callback_defs.h"
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
# 4 "dsp_dslash_spinor.h" 2
# 111 "dsp_dslash_spinor.h"
void spinor_acc_gammaFLAG_GAMMA_dag_mul(spinor_vec dst, su3_vec link, spinor_vec src){
    lvector FLOAT *psrc = (lvector FLOAT*)src;
    lvector FLOAT *plink = (lvector FLOAT*)link;
    register lvector FLOAT src_re,src_im;
    register lvector FLOAT dst0_re = 0, dst1_re = 0,dst2_re = 0,dst3_re = 0,
      dst4_re = 0,dst5_re = 0,dst6_re = 0,dst7_re = 0,dst8_re = 0,dst9_re = 0,
      dst10_re = 0,dst11_re = 0;
    register lvector FLOAT dst0_im = 0, dst1_im = 0,dst2_im = 0,dst3_im = 0,
      dst4_im = 0,dst5_im = 0,dst6_im = 0,dst7_im = 0,dst8_im = 0,dst9_im = 0,
      dst10_im = 0,dst11_im = 0;
    register lvector FLOAT link0_re,link1_re,link2_re,link3_re,link4_re,link5_re,link6_re,link7_re,link8_re;
    register lvector FLOAT link0_im,link1_im,link2_im,link3_im,link4_im,link5_im,link6_im,link7_im,link8_im;

    link0_re = vec_ld(0,plink);
    link0_im = vec_ld(16,plink);
    link1_re = vec_ld(32,plink);
    link1_im = vec_ld(48,plink);
    link2_re = vec_ld(64,plink);
    link2_im = vec_ld(80,plink);
    link3_re = vec_ld(96,plink);
    link3_im = vec_ld(112,plink);
    link4_re = vec_ld(128,plink);
    link4_im = vec_ld(144,plink);
    link5_re = vec_ld(160,plink);
    link5_im = vec_ld(176,plink);
    link6_re = 0;
    link6_im = 0;
    link7_re = 0;
    link7_im = 0;
    link8_re = 0;
    link8_im = 0;

    { link6_re = vec_mula(link1_im,link5_im,link6_re); link6_re = vec_mula(link2_re,link4_re,link6_re); link6_re = vec_mulb(link1_re,link5_re,link6_re); link6_re = vec_mula(link2_im,link4_im,link6_re); link6_im = vec_mula(link1_im,link5_re,link6_im); link6_im = vec_mula(link1_re,link5_im,link6_im); link6_im = vec_mulb(link2_im,link4_re,link6_im); link6_im = vec_mula(link2_re,link4_im,link6_im);};
    { link7_re = vec_mula(link2_im,link3_im,link7_re); link7_re = vec_mula(link0_re,link5_re,link7_re); link7_re = vec_mulb(link2_re,link3_re,link7_re); link7_re = vec_mula(link0_im,link5_im,link7_re); link7_im = vec_mula(link2_im,link3_re,link7_im); link7_im = vec_mula(link2_re,link3_im,link7_im); link7_im = vec_mulb(link0_im,link5_re,link7_im); link7_im = vec_mula(link0_re,link5_im,link7_im);};
    { link8_re = vec_mula(link0_im,link4_im,link8_re); link8_re = vec_mula(link1_re,link3_re,link8_re); link8_re = vec_mulb(link0_re,link4_re,link8_re); link8_re = vec_mula(link1_im,link3_im,link8_re); link8_im = vec_mula(link0_im,link4_re,link8_im); link8_im = vec_mula(link0_re,link4_im,link8_im); link8_im = vec_mulb(link1_im,link3_re,link8_im); link8_im = vec_mula(link1_re,link3_im,link8_im);};






    { { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst0_re = vec_mula(link0_re,src_re,dst0_re); dst0_re = vec_mula(link0_im,src_im,dst0_re); dst0_im = vec_mulb(link0_im,src_re,dst0_im); dst0_im = vec_mulb(link0_re,src_im,dst0_im); }; { dst4_re = vec_mula(link1_re,src_re,dst4_re); dst4_re = vec_mula(link1_im,src_im,dst4_re); dst4_im = vec_mulb(link1_im,src_re,dst4_im); dst4_im = vec_mulb(link1_re,src_im,dst4_im); }; { dst8_re = vec_mula(link2_re,src_re,dst8_re); dst8_re = vec_mula(link2_im,src_im,dst8_re); dst8_im = vec_mulb(link2_im,src_re,dst8_im); dst8_im = vec_mulb(link2_re,src_im,dst8_im); };}; { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst1_re = vec_mula(link0_re,src_re,dst1_re); dst1_re = vec_mula(link0_im,src_im,dst1_re); dst1_im = vec_mulb(link0_im,src_re,dst1_im); dst1_im = vec_mulb(link0_re,src_im,dst1_im); }; { dst5_re = vec_mula(link1_re,src_re,dst5_re); dst5_re = vec_mula(link1_im,src_im,dst5_re); dst5_im = vec_mulb(link1_im,src_re,dst5_im); dst5_im = vec_mulb(link1_re,src_im,dst5_im); }; { dst9_re = vec_mula(link2_re,src_re,dst9_re); dst9_re = vec_mula(link2_im,src_im,dst9_re); dst9_im = vec_mulb(link2_im,src_re,dst9_im); dst9_im = vec_mulb(link2_re,src_im,dst9_im); };}; { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst2_re = vec_mula(link0_re,src_re,dst2_re); dst2_re = vec_mula(link0_im,src_im,dst2_re); dst2_im = vec_mulb(link0_im,src_re,dst2_im); dst2_im = vec_mulb(link0_re,src_im,dst2_im); }; { dst6_re = vec_mula(link1_re,src_re,dst6_re); dst6_re = vec_mula(link1_im,src_im,dst6_re); dst6_im = vec_mulb(link1_im,src_re,dst6_im); dst6_im = vec_mulb(link1_re,src_im,dst6_im); }; { dst10_re = vec_mula(link2_re,src_re,dst10_re); dst10_re = vec_mula(link2_im,src_im,dst10_re); dst10_im = vec_mulb(link2_im,src_re,dst10_im); dst10_im = vec_mulb(link2_re,src_im,dst10_im); };}; { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst3_re = vec_mula(link0_re,src_re,dst3_re); dst3_re = vec_mula(link0_im,src_im,dst3_re); dst3_im = vec_mulb(link0_im,src_re,dst3_im); dst3_im = vec_mulb(link0_re,src_im,dst3_im); }; { dst7_re = vec_mula(link1_re,src_re,dst7_re); dst7_re = vec_mula(link1_im,src_im,dst7_re); dst7_im = vec_mulb(link1_im,src_re,dst7_im); dst7_im = vec_mulb(link1_re,src_im,dst7_im); }; { dst11_re = vec_mula(link2_re,src_re,dst11_re); dst11_re = vec_mula(link2_im,src_im,dst11_re); dst11_im = vec_mulb(link2_im,src_re,dst11_im); dst11_im = vec_mulb(link2_re,src_im,dst11_im); };};}
    { { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst0_re = vec_mula(link3_re,src_re,dst0_re); dst0_re = vec_mula(link3_im,src_im,dst0_re); dst0_im = vec_mulb(link3_im,src_re,dst0_im); dst0_im = vec_mulb(link3_re,src_im,dst0_im); }; { dst4_re = vec_mula(link4_re,src_re,dst4_re); dst4_re = vec_mula(link4_im,src_im,dst4_re); dst4_im = vec_mulb(link4_im,src_re,dst4_im); dst4_im = vec_mulb(link4_re,src_im,dst4_im); }; { dst8_re = vec_mula(link5_re,src_re,dst8_re); dst8_re = vec_mula(link5_im,src_im,dst8_re); dst8_im = vec_mulb(link5_im,src_re,dst8_im); dst8_im = vec_mulb(link5_re,src_im,dst8_im); };}; { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst1_re = vec_mula(link3_re,src_re,dst1_re); dst1_re = vec_mula(link3_im,src_im,dst1_re); dst1_im = vec_mulb(link3_im,src_re,dst1_im); dst1_im = vec_mulb(link3_re,src_im,dst1_im); }; { dst5_re = vec_mula(link4_re,src_re,dst5_re); dst5_re = vec_mula(link4_im,src_im,dst5_re); dst5_im = vec_mulb(link4_im,src_re,dst5_im); dst5_im = vec_mulb(link4_re,src_im,dst5_im); }; { dst9_re = vec_mula(link5_re,src_re,dst9_re); dst9_re = vec_mula(link5_im,src_im,dst9_re); dst9_im = vec_mulb(link5_im,src_re,dst9_im); dst9_im = vec_mulb(link5_re,src_im,dst9_im); };}; { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst2_re = vec_mula(link3_re,src_re,dst2_re); dst2_re = vec_mula(link3_im,src_im,dst2_re); dst2_im = vec_mulb(link3_im,src_re,dst2_im); dst2_im = vec_mulb(link3_re,src_im,dst2_im); }; { dst6_re = vec_mula(link4_re,src_re,dst6_re); dst6_re = vec_mula(link4_im,src_im,dst6_re); dst6_im = vec_mulb(link4_im,src_re,dst6_im); dst6_im = vec_mulb(link4_re,src_im,dst6_im); }; { dst10_re = vec_mula(link5_re,src_re,dst10_re); dst10_re = vec_mula(link5_im,src_im,dst10_re); dst10_im = vec_mulb(link5_im,src_re,dst10_im); dst10_im = vec_mulb(link5_re,src_im,dst10_im); };}; { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst3_re = vec_mula(link3_re,src_re,dst3_re); dst3_re = vec_mula(link3_im,src_im,dst3_re); dst3_im = vec_mulb(link3_im,src_re,dst3_im); dst3_im = vec_mulb(link3_re,src_im,dst3_im); }; { dst7_re = vec_mula(link4_re,src_re,dst7_re); dst7_re = vec_mula(link4_im,src_im,dst7_re); dst7_im = vec_mulb(link4_im,src_re,dst7_im); dst7_im = vec_mulb(link4_re,src_im,dst7_im); }; { dst11_re = vec_mula(link5_re,src_re,dst11_re); dst11_re = vec_mula(link5_im,src_im,dst11_re); dst11_im = vec_mulb(link5_im,src_re,dst11_im); dst11_im = vec_mulb(link5_re,src_im,dst11_im); };};}
    { { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst0_re = vec_mula(link6_re,src_re,dst0_re); dst0_re = vec_mula(link6_im,src_im,dst0_re); dst0_im = vec_mulb(link6_im,src_re,dst0_im); dst0_im = vec_mulb(link6_re,src_im,dst0_im); }; { dst4_re = vec_mula(link7_re,src_re,dst4_re); dst4_re = vec_mula(link7_im,src_im,dst4_re); dst4_im = vec_mulb(link7_im,src_re,dst4_im); dst4_im = vec_mulb(link7_re,src_im,dst4_im); }; { dst8_re = vec_mula(link8_re,src_re,dst8_re); dst8_re = vec_mula(link8_im,src_im,dst8_re); dst8_im = vec_mulb(link8_im,src_re,dst8_im); dst8_im = vec_mulb(link8_re,src_im,dst8_im); };}; { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst1_re = vec_mula(link6_re,src_re,dst1_re); dst1_re = vec_mula(link6_im,src_im,dst1_re); dst1_im = vec_mulb(link6_im,src_re,dst1_im); dst1_im = vec_mulb(link6_re,src_im,dst1_im); }; { dst5_re = vec_mula(link7_re,src_re,dst5_re); dst5_re = vec_mula(link7_im,src_im,dst5_re); dst5_im = vec_mulb(link7_im,src_re,dst5_im); dst5_im = vec_mulb(link7_re,src_im,dst5_im); }; { dst9_re = vec_mula(link8_re,src_re,dst9_re); dst9_re = vec_mula(link8_im,src_im,dst9_re); dst9_im = vec_mulb(link8_im,src_re,dst9_im); dst9_im = vec_mulb(link8_re,src_im,dst9_im); };}; { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst2_re = vec_mula(link6_re,src_re,dst2_re); dst2_re = vec_mula(link6_im,src_im,dst2_re); dst2_im = vec_mulb(link6_im,src_re,dst2_im); dst2_im = vec_mulb(link6_re,src_im,dst2_im); }; { dst6_re = vec_mula(link7_re,src_re,dst6_re); dst6_re = vec_mula(link7_im,src_im,dst6_re); dst6_im = vec_mulb(link7_im,src_re,dst6_im); dst6_im = vec_mulb(link7_re,src_im,dst6_im); }; { dst10_re = vec_mula(link8_re,src_re,dst10_re); dst10_re = vec_mula(link8_im,src_im,dst10_re); dst10_im = vec_mulb(link8_im,src_re,dst10_im); dst10_im = vec_mulb(link8_re,src_im,dst10_im); };}; { src_re = vec_ld(0,psrc++); src_im = vec_ld(0,psrc++); { dst3_re = vec_mula(link6_re,src_re,dst3_re); dst3_re = vec_mula(link6_im,src_im,dst3_re); dst3_im = vec_mulb(link6_im,src_re,dst3_im); dst3_im = vec_mulb(link6_re,src_im,dst3_im); }; { dst7_re = vec_mula(link7_re,src_re,dst7_re); dst7_re = vec_mula(link7_im,src_im,dst7_re); dst7_im = vec_mulb(link7_im,src_re,dst7_im); dst7_im = vec_mulb(link7_re,src_im,dst7_im); }; { dst11_re = vec_mula(link8_re,src_re,dst11_re); dst11_re = vec_mula(link8_im,src_im,dst11_re); dst11_im = vec_mulb(link8_im,src_re,dst11_im); dst11_im = vec_mulb(link8_re,src_im,dst11_im); };};}

    { MACRO_GAMMAFLAG_GAMMA (0,1,2,3) MACRO_GAMMAFLAG_GAMMA (4,5,6,7) MACRO_GAMMAFLAG_GAMMA (8,9,10,11)};
    psrc = (lvector FLOAT*) dst;
    vec_st(dst0_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst0_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst1_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst1_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst2_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst2_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst3_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst3_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst4_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst4_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst5_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst5_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst6_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst6_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst7_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst7_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst8_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst8_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst9_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst9_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst10_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst10_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst11_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst11_im+vec_ld(0,psrc),0,psrc);
}
