#include "record/record_defines.h"
#include "record/record_meta.h"
#include "record/record_reader.h"
#include "record/record_type.h"
#include "record/record_writer.h"
#include "instrument/pmu_collector.h"
#include "fsl/raw_backend.h"
#include "ral/backend.h"
#include "ral/section.h"
#include "record/record_type_info.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <list>
#include "utils/configuration.h"

#ifdef ENABLE_BACKTRACE
#include "instrument/backtrace.h"
#define BACKTRACE_MAX_SIZE_DEFAULT 21
int backtrace_max_size = BACKTRACE_MAX_SIZE_DEFAULT;
#endif
#include <fsl/raw_backend.h>
#include <filesystem>

#define DECLARE_GLOBAL_VARIABLE(T, val) \
T * val;\
__attribute__((constructor (RECORD_INIT_PRIORITY-1)))\
void construct_##val() { \
    val = new T(); \
} \
__attribute__((destructor (RECORD_FINI_PRIORITY-1))) \
void destory_##val() { \
    delete val; \
}

using namespace pse;

bool _jsi_record_inited = false;
FILE *_jsi_record_file;
RecordMeta *meta;

DECLARE_GLOBAL_VARIABLE(std::unique_ptr<pse::ral::BackendInterface>, _backend);
DECLARE_GLOBAL_VARIABLE(std::unique_ptr<pse::ral::DirSectionInterface>, _root);
DECLARE_GLOBAL_VARIABLE(std::unique_ptr<pse::ral::DirSectionInterface>, _process_trace_dir);
DECLARE_GLOBAL_VARIABLE(std::unique_ptr<pse::ral::DirSectionInterface>, _process_backtrace_dir);
DECLARE_GLOBAL_VARIABLE(std::unique_ptr<pse::ral::DirSectionInterface>, _thread_trace_dir);

// pse::ral::BackendInterface* backend_ptr;
// pse::ral::DirSectionInterface* process_backtrace_dir_ptr;

// thread_local pse::ral::DirSectionInterface* thread_trace_dir_ptr;


bool jsi_pmu_enabled = false;
bool jsi_backtrace_enabled = false;
int jsi_pmu_num = 0;
std::atomic<int> jsi_record_writer_init_completed(0);
std::atomic<int> jsi_record_writer_fini_completed(0);
std::atomic<int> jsi_record_writer_thread_local_wrapper_completed(0);

inline size_t jsi_pmu_bytes()
{
    return sizeof(uint64_t) * 2 * jsi_pmu_num;
}

// static FILE* _bt_fp = 0;

class _ConfigHelper {
  public:
    static const std::string& get_identifier() {
        static std::string identifier = _ConfigHelper::get_hostname() + "_" + std::to_string(_ConfigHelper::get_pid());
        return identifier;
    }
    static const std::string& get_hostname() {
        static std::string hostname = _ConfigHelper::_get_hostname();
        return hostname;
    }
    static uint32_t get_pid() {
        static auto pid = getpid();
        return pid;
    }
    static uint32_t get_tid() {
        thread_local static auto tid = syscall(SYS_gettid);
        return tid;
    }
  private:
    static std::string _get_hostname() {
        char _hostname[1024];
        gethostname(_hostname, 1024);
        _hostname[1023] = '\0';
        return std::string(_hostname);
    }
};

__attribute__((constructor (RECORD_INIT_PRIORITY)))
void recordWriterCacheInit() {
    JSI_LOG(JSILOG_INFO, "Initialize Record Writer Library.\n");
    RecordWriter::init();
    // record the process start event
    uint64_t t = get_tsc_raw();
    record_t* rec = (record_t*) ALLOCATE(
        sizeof(record_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        // TODO: Now this is just a workaround to make sure the values are valid
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    rec[0].MsgType = JSI_PROCESS_START;
    rec[0].timestamps.enter = t;
    rec[0].timestamps.exit = t;

    RecordWriter::traceStore(rec);
    DEALLOCATE(rec, sizeof(record_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);

    jsi_record_writer_init_completed.store(1);
}

__attribute__((destructor (RECORD_FINI_PRIORITY)))
void recordWriterCacheFinalize() {
    jsi_record_writer_fini_completed.store(1);
    JSI_LOG(JSILOG_INFO, "Finalize Record Writer Library.\n");
    uint64_t t = get_tsc_raw();
    record_t* rec = (record_t*) ALLOCATE(
        sizeof(record_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        // TODO: Now this is just a workaround to make sure the values are valid
        pmu_collector_get_all(counters);
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    rec[0].MsgType = JSI_PROCESS_EXIT;
    rec[0].timestamps.enter = t;
    rec[0].timestamps.exit = t;
    RecordWriter::traceStore(rec);
    DEALLOCATE(rec, sizeof(record_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    // Finalize dependant utilities
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        if (!pmu_collector_fini()) {
            JSI_WARN("PMU collector finalization failed.\n");
        }
    }
#endif
    auto& process_backtrace_dir = *(_process_backtrace_dir);
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        backtrace_db_dump(process_backtrace_dir);
        //fclose(_bt_fp);
        backtrace_finalize();
    }
#endif
    RecordWriter::finalize();
    delete meta;
}

// // TODO: Implement Async IO
// void RecordWriter::writeAndClear() {
//     uint64_t enter = get_tsc_raw();
//     area->AreaFlush();
//     uint64_t exit = get_tsc_raw();
//     record_t *rec = (record_t *) RecordWriter::allocate(sizeof(record_t));
//     rec[0].MsgType = JSI_TOOL_IO_MSGTYPE;
//     rec[0].timestamps.enter = enter;
//     rec[0].timestamps.exit = exit;
//     return;
// }

inline void save_hostinfo() {
    RecordWriter::metaSectionStart("HOST INFO");
    RecordWriter::metaStore("HOSTNAME", _ConfigHelper::get_hostname());
    RecordWriter::metaStore("PID", (int32_t)_ConfigHelper::get_pid());
    RecordWriter::metaSectionEnd("HOST INFO");
}

void RecordWriter::init() {
    RecordWriter::init(_ConfigHelper::get_hostname());
}

void RecordWriter::init(const std::string& identifier) {
    if (_jsi_record_inited) {
        JSI_WARN("[RecordWriter::init(id=%s)] Already initialized!\n", identifier.c_str());
        return;
    }
    _jsi_record_inited = true;

    jsi_backtrace_enabled = EnvConfigHelper::get_enabled("JSI_ENABLE_BACKTRACE", false);
    jsi_pmu_enabled = EnvConfigHelper::get_enabled("JSI_ENABLE_PMU", false);

    std::string file_path;
    std::string file_sym_path;
    char* measurement_path = getenv("JSI_MEASUREMENT_DIR");
    if (measurement_path) {
        file_path += std::string(measurement_path) + "/";
    }

    file_path += DEFAULT_PROFILE_BASE + identifier + JSI_SECTION_FILE_EXT;
    auto raw_backend = pse::fsl::RawSectionBackend(file_path, pse::ral::RWMode::WRITE);
    auto& backend = *(_backend);
    auto& root = *(_root);
    auto& process_trace_dir = *(_process_trace_dir);
    auto &process_backtrace_dir = *(_process_backtrace_dir);
    backend = std::make_unique<pse::ral::BackendWrapper<pse::fsl::RawSectionBackend>>(std::move(raw_backend));
    root = std::move(backend->openRootSection());
    auto trace_dir = root->openDirSection(StaticSectionDesc::TRACE_SEC_ID, true);
    process_trace_dir = trace_dir->openDirSection(_ConfigHelper::get_pid(), true);
    if (jsi_backtrace_enabled) {
        auto backtrace_dir = root->openDirSection(StaticSectionDesc::BACKTRACE_SEC_ID, true);
        process_backtrace_dir = backtrace_dir->openDirSection(_ConfigHelper::get_pid(), true);
    }
    if (backend->isLeader())
    {
        meta = new RecordMeta(root, false);
        RecordWriter::metaSectionStart("HOST INFO");
        RecordWriter::metaStore("HOSTNAME", _ConfigHelper::get_hostname());
        RecordWriter::metaStore("VERSION", STORAGE_VERSION);
        RecordWriter::metaSectionEnd("HOST INFO");
        delete meta;
    }
    meta = new RecordMeta(process_trace_dir, false);

    // save host information
    save_hostinfo();

#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        JSI_INFO("PMU collecting enabled inside program\n");
        if (!pmu_collector_init()) {
            JSI_ERROR("PMU collecting initialization failed. Aborting...\n");
        }
        jsi_pmu_num = pmu_collector_get_num_events();
    }
#endif

#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        JSI_INFO("Backtrace enabled inside program\n");
        const char* bt_max_depth = getenv("JSI_BACKTRACE_MAX_DEPTH");
        if (bt_max_depth) {
            char* end_ptr;
            backtrace_max_size = (int) std::strtol(bt_max_depth, &end_ptr, 10);
            if (*end_ptr != '\0') {
                JSI_WARN(
                        "Invalid JSI_BACKTRACE_MAX_DEPTH (expect an integer, but get \"%s\") "
                        "encountered and will be ignored, the max depth is set to `%d` by default",
                        bt_max_depth, BACKTRACE_MAX_SIZE_DEFAULT);
                backtrace_max_size = BACKTRACE_MAX_SIZE_DEFAULT;
            }
        }
        backtrace_init_recording(backtrace_max_size);
        // TODO (fty): Use `RecordWriter::writeMeta` when it is implemented
        //             in the future to dump the backtrace db as metadata,
        //             instead of writing to manually created files.
        // std::string file_path;
        // char* measurement_path = getenv("JSI_MEASUREMENT_DIR");
        // if (measurement_path) {
        //     file_path += std::string(measurement_path);
        // }
        // file_path += "/backtrace_db." + _ConfigHelper::get_identifier() + ".bin";
        // JSI_INFO("Backtrace will dumped to %s\n", file_path.c_str());
        // _bt_fp = fopen(file_path.c_str(), "w");
    }
#endif

}
auto& get_thread_trace_dir()
{
    // if (thread_trace_dir_ptr) {
    //     return thread_trace_dir_ptr;
    // }
    auto &process_trace_dir = *(_process_trace_dir);
    auto &thread_trace_dir = *(_thread_trace_dir);
    if (thread_trace_dir)
    {
        return thread_trace_dir;
    }
    thread_trace_dir = (process_trace_dir)->openDirSection(_ConfigHelper::get_tid(), true);
    return thread_trace_dir;
}
bool thread_destroyed = false;
template<typename T>
struct ThreadLocalWrapper {
    T init_val;
    T& exit_val;
    ThreadLocalWrapper(T& exit_val) : exit_val(exit_val) {}
    ~ThreadLocalWrapper()
    {
        jsi_record_writer_thread_local_wrapper_completed.store(1);
        if (_ConfigHelper::get_tid() == _ConfigHelper::get_pid())
        {
            thread_destroyed = true;
            exit_val = std::move(init_val);
        }
    }
};

using T1 = std::unordered_map<size_t, std::unique_ptr<pse::ral::DataSectionInterface>>;
// std::unordered_map<size_t, std::unique_ptr<pse::ral::DataSectionInterface>> global_sec_map;
DECLARE_GLOBAL_VARIABLE(T1, global_sec_map);
thread_local ThreadLocalWrapper<std::unordered_map<size_t, std::unique_ptr<pse::ral::DataSectionInterface>>> _sec_map(*global_sec_map);

void RecordWriter::traceStore(const record_t* record)
{
    size_t id = record[0].MsgType;
    const char* t= (const char*)&record->timestamps.enter;
    auto& sec_map = thread_destroyed ? *global_sec_map : _sec_map.init_val;
    auto iter = sec_map.find(id);
    if (iter == sec_map.end()) {
        auto& dir = get_thread_trace_dir();
        auto generic = dir->openDirSection(StaticSectionDesc::GENERIC_TRACE_SEC_ID, true);
        auto sec = generic->openDataSection(id + StaticSectionDesc::RECORD_SEC_OFFSET, true, record_info[id].add_size(jsi_pmu_bytes()), t-(const char*)record);
        sec->write(record);
        sec_map[id] = std::move(sec);
    }
    else
    {
        iter->second->write(record);
    }
}
void RecordWriter::samplingStore(size_t id, const record_t* record)
{
    const char* t= (const char*)&record->timestamps.enter;
    auto& thread_trace_dir = get_thread_trace_dir();
    auto dir = thread_trace_dir->openDirSection(StaticSectionDesc::SAMPLING_TRACE_SEC_ID, true);
    auto sec = dir->openDataSection(id + StaticSectionDesc::RECORD_SEC_OFFSET, true, record_info[id].add_size(jsi_pmu_bytes()), t-(const char*)record);
    sec->write(record);
}
void RecordWriter::extStore(const void* record, size_t size)
{
    auto& thread_trace_dir = get_thread_trace_dir();
    auto stream_sec = thread_trace_dir->openStreamSection(StaticSectionDesc::EXT_TRACE_SEC_ID, true);
    stream_sec->write(record, size);
}


void *RecordWriter::allocate(size_t size) {
    JSI_ERROR("RecordWriter::allocate is diabled. DO NOT USE.\n");
    // if (!_jsi_record_inited) { printf("Warning: allocate before initialization!\n"); }
    // //JSI_WARN("RecordWriter::allocate should not be called!");
    // char *ptr = (char *) (area->AreaAllocate(size));
    // return ptr;
}

void RecordWriter::finalize() {
    if (!_jsi_record_inited) {
        return;
    }
    // area->AreaFinalize();
    _jsi_record_inited = false;
}


void RecordWriter::writeMetaData() {
    JSI_ERROR("RecordWriter::writeMetaData Not implemented!\n");
}

void RecordWriter::metaSectionStart(const char *name) {
    meta->sectionStart(name);
}

void RecordWriter::metaSectionEnd(const char *name) {
    meta->sectionEnd(name);
}

void RecordWriter::metaRawStore(const char *name, void *p, size_t size) {
    meta->metaRawStore(name,p,size);
}

RecordWriterExt::RecordWriterExt() {
    file = NULL;
    file_sym = NULL;
    _buff_size = 0;
    _buff_cur = 0;
    _buff = NULL;
}

RecordWriterExt::~RecordWriterExt() {
    if (_buff_size) {
        flushCache();
        delete[] _buff;
    }
    close();
}

void RecordWriterExt::flushCache() {
    if(_buff_cur) {
        JSI_INFO("RecordWriterExt::flushCache Mannual flush: cur=%lu\n", _buff_cur);
        writeSection(reinterpret_cast<const char*>(_buff), _buff_cur);
        _buff_cur = 0;
    }
}

void RecordWriterExt::init(JSI_TRACE_HEADER header) {
    std::string file_path;
    std::string file_sym_path;
    char* measurement_path = getenv("JSI_MEASUREMENT_DIR");
    if (measurement_path) {
        file_path += std::string(measurement_path) + "/";
    }
    file_path += get_trace_base(header) + _ConfigHelper::get_identifier() + JSI_EXT_TRACE_FILE_EXT;
    file = fopen(file_path.c_str(), "w");
    if (file==NULL) {
        JSI_ERROR("file could not open: %s\n", file_path.c_str());
    }
    int32_t header_id = (int8_t)header;
    fwrite(&header_id, 1, sizeof(int8_t), file);

    file_sym_path = file_path + "-sym";
    file_sym = fopen(file_sym_path.c_str(), "w");
    if (file_sym==NULL) {
        JSI_ERROR("file could not open: %s\n", file_sym_path.c_str());
    }
}

void RecordWriterExt::setCacheSize(size_t size) {
    if (_buff_size) {
        JSI_ERROR("RecordWriterExt::setCacheSize size is already configured: %lu!\n", size);
    }
    _buff_size = size;
    _buff_cur = 0;
    _buff = new int8_t[size];
}

int8_t* RecordWriterExt::allocate(size_t size) {
    if (size > _buff_size) {
        JSI_ERROR("RecordWriterExt::allocate require too large memory: %lu. Please set larger cache size.", size);
    }
    size_t next_cur = _buff_cur + size;
    if (next_cur >= _buff_size) {
        writeSection(reinterpret_cast<const char*>(_buff), _buff_cur);
        _buff_cur = size;
        return _buff;
    }
    int8_t* _ret = &_buff[_buff_cur];
    _buff_cur = next_cur;
    return _ret;
}

void RecordWriterExt::close() {
    if(file) {
        fclose(file);
    }
    if(file_sym) {
        fclose(file_sym);
    }
}

void RecordWriterExt::writeSection(const char* data, uint64_t size) {
    size_t sz;
    sz = fwrite(&size, 1, sizeof(uint64_t), file);
    sz = fwrite(data, 1, size, file);
}

void RecordWriterExt::writeStringBuffer(uint64_t key, std::string str) {
    auto it = _stringbuf.find(key);
    if(it!=_stringbuf.end()) {
        return ;
    }
    _stringbuf[key] = 1;
    // include the final '\0'
    fwrite(&key, 1, sizeof(uint64_t), file_sym);
    fwrite(str.c_str(), str.size()+1, sizeof(char), file_sym);
}