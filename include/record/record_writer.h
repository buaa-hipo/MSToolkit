#ifndef __JSI_RECORD_WRITER_H__
#define __JSI_RECORD_WRITER_H__

#include <string>
#include "utils/tsc_timer.h"
#include "utils/jsi_log.h"
#include "record/record_defines.h"
#include "record/record_area.h"
#include "record/record_meta.h"
#include <atomic>


// 4KB preallocate buffer size
// MOVE TO RECORD_AREA.H
//#define PREALLOCATE_BUFFSIZE 4096

#define RECORD_INIT_PRIORITY 102
#define RECORD_FINI_PRIORITY 102

#ifdef USE_ALLOCA
#include <malloc.h>
#define ALLOCATE(size) alloca(size)
#define DEALLOCATE(ptr,size) 
#else
#warning Performance may degrade as ALLOCA is not enabled (USE_ALLOCA=OFF)
#define ALLOCATE(size) malloc(size)
#define DEALLOCATE(ptr,size) free(ptr)
#endif

extern bool _jsi_record_inited;
extern FILE *_jsi_record_file;
extern RecordArea *area;
extern RecordMeta *meta;
extern bool jsi_pmu_enabled;
extern bool jsi_backtrace_enabled;
extern int jsi_pmu_num;
extern std::atomic<int> jsi_record_writer_init_completed;
extern std::atomic<int> jsi_record_writer_fini_completed;
extern std::atomic<int> jsi_record_writer_thread_local_wrapper_completed;

struct RecordWriterHelper {
    template<typename RecordType>
    static inline __attribute__((always_inline))
    uint64_t* counters(RecordType* r) {
        return reinterpret_cast<uint64_t *>(r + 1);
    }
};

struct RecordWriterExt {
    /* Extended record writer: records are managed as sections as follows:
     * <trace header> <sec.size> <sec.data...> <sec.size> <sec.data...> ...
     */
    public:
        RecordWriterExt();
        ~RecordWriterExt();
        void init(JSI_TRACE_HEADER header);
        void close();
        void writeStringBuffer(uint64_t key, std::string str);
        int8_t* allocate(size_t size);
        void flushCache();
        void setCacheSize(size_t size);
    private:
        FILE* file;
        FILE* file_sym;
        std::unordered_map<uint64_t, int> _stringbuf;
        int8_t* _buff;
        size_t  _buff_size;
        size_t  _buff_cur;
        void writeSection(const char* data, uint64_t size);
};

// TODO: add backtrace init/dump/finalize when required.
struct RecordWriter {
private:
    // static void writeAndClear();

    /* TODO: implement */
    static void writeMetaData();

public:
    static void init(const std::string& identifier);

    static void init();

    static void *allocate(size_t size);

    static void finalize();

    static void metaSectionStart(const char *name);

    static void metaSectionEnd(const char *name);

    template<typename T>
    inline static void metaStore(const char *name, T val) {
        meta->metaRaw(name, val);
    }

    static void metaRawStore(const char *name, void *p, size_t size);

    static void traceStore(const record_t* record);
    static void samplingStore(size_t id, const record_t* record);
    static void extStore(const void* record, size_t size);
};

#endif