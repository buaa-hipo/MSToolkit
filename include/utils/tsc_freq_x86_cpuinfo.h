#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HYGON_C86
// Work around for Hygon C86 7185 32-core Processor, as it did not provide base freq in cpuinfo.
static inline uint64_t get_cpu_base_freq_x86_cpuinfo_mhz() {
    // from documents: 2.5 GHz
    return 2500;
}
#else
static inline uint64_t get_cpu_base_freq_x86_cpuinfo_mhz() {
    FILE* file = fopen("/proc/cpuinfo", "r");
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char* pos = strstr(line, "model name");
        if (pos == NULL) {
            continue;
        }

        char* pos_freq_begin = strchr(line, '@');
        char* pos_freq_end;
        while (pos_freq_begin != nullptr && (pos_freq_end = strchr(pos_freq_begin + 1, '@')) != NULL) {
            pos_freq_begin = pos_freq_end;
        }

        if (pos_freq_begin == nullptr) {
            fprintf(stderr, "Missing freq info in cpuinfo, using default value 2.2 GHz\n");
            return 2200;
        }

        pos_freq_begin += 2; // skip "@ "

        double freq_ghz = strtod(pos_freq_begin, &pos_freq_end);

        if (pos_freq_end[0] == 'G') {
            return (uint64_t) (freq_ghz * 1000);
        } else {
            // unreachable
            goto error;
        }
        break;
    }

error:
    fprintf(stderr, "Invalid cpuinfo format");// might be replaced with a logger in the future
    exit(-1);

    // unreachable
    return 0;
}
#endif

static inline uint64_t get_tsc_freq_x86_cpuinfo_mhz() {
    return get_cpu_base_freq_x86_cpuinfo_mhz();
}