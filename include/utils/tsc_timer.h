#pragma once

#include <stdint.h>
#include <sys/time.h>

#ifdef __aarch64__
    static inline uint64_t get_tsc_freq_mhz() {
        uint64_t freq;
        // Read cntfrq_el0 reg to get the tsc freq in Hz
        asm volatile("mrs %0, cntfrq_el0"
                    : "=r"(freq));
        return freq / 1000000; // `freq` is in Hz and need to be converted to MHz
    }

    static inline uint64_t get_tsc_raw() {
        uint64_t tsc;
        asm volatile("isb; mrs %0, cntvct_el0"
                    : "=r"(tsc));
        return tsc;
    }
#elif __x86_64__
    #include "tsc_freq_x86_cpuinfo.h"

    static inline uint64_t get_tsc_freq_mhz() {
        return get_tsc_freq_x86_cpuinfo_mhz();
    }

    static inline uint64_t get_tsc_raw() {
        uint32_t hi, lo, aux;
        __asm__ volatile("rdtscp"
                        : "=a"(lo), "=d"(hi), "=c"(aux));
        return ((uint64_t) hi << 32) | lo;
    }
#endif

static inline double tsc_duration_seconds(uint64_t tsc_delta, uint64_t tsc_freq_mhz) {
    return (double) (tsc_delta) / (tsc_freq_mhz * 1e6);
}

static inline double tsc_duration_ms(uint64_t tsc_delta, uint64_t tsc_freq_mhz) {
    return (double) (tsc_delta) / (tsc_freq_mhz * 1e3);
}

static inline double tsc_duration_us(uint64_t tsc_delta, uint64_t tsc_freq_mhz) {
    return (double) (tsc_delta) / tsc_freq_mhz;
}

static inline double tsc_duration_ns(uint64_t tsc_delta, uint64_t tsc_freq_mhz) {
    return (double) (tsc_delta) / (tsc_freq_mhz * 1e-3);
}

static inline uint64_t us_to_tsc(uint64_t us, uint64_t tsc_freq_mhz) {
    return us * tsc_freq_mhz;
}

static inline uint64_t ms_to_tsc(uint64_t ms, uint64_t tsc_freq_mhz) {
    return ms * tsc_freq_mhz * 1000;
}

static inline uint64_t ns_to_tsc(uint64_t ns, uint64_t tsc_freq_mhz) {
    return ns * tsc_freq_mhz / 1000;
}

static inline uint64_t system_clock_duration_us() {
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

// only for inner profiling
class _Timer {
    public:
        void reset() {
            acc_time = 0;
        }
        void start() {
            start_ts = system_clock_duration_us();
        }
        void stop() {
            acc_time += system_clock_duration_us() - start_ts;
        }
        void print(const char* name) {
            printf("### %s elasped time: %lf sec\n", name, (double)acc_time/1000000.0);
        }
    private:
        uint64_t start_ts;
        uint64_t acc_time;
};
static _Timer _timer;
#define TICK(name) do { _timer.reset(); _timer.start(); } while(0)
#define TOCK(name) do { _timer.stop(); _timer.print(name); } while(0)