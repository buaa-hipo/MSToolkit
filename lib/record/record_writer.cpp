#include "record/record_defines.h"
#include "record/record_meta.h"
// #include "record/record_reader.h"
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
#include <set>
#include "utils/safe.hpp"
#include "utils/configuration.h"
#include <sys/time.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#ifdef ENABLE_BACKTRACE
#include "instrument/backtrace.h"
#define BACKTRACE_MAX_SIZE_DEFAULT 21
int backtrace_max_size = BACKTRACE_MAX_SIZE_DEFAULT;
#endif
#include <fsl/raw_backend.h>
#include <filesystem>

// #define SIG_THREAD_EXIT SIGUSR1
// #define SIG_THREAD_SAMPLING SIGUSR2

#define SIG_THREAD_EXIT (SIGRTMIN+1)
#define SIG_THREAD_SAMPLING (SIGRTMIN+2)
#define REALTIME_SIGNAL     (SIGRTMIN+3)

#define DECLARE_GLOBAL_VARIABLE(T, val) \
T * val;\
// __attribute__((constructor (RECORD_INIT_PRIORITY-1)))\
// void construct_##val() { \
//     val = new T(); \
// } \
// __attribute__((destructor (RECORD_FINI_PRIORITY-1))) \
// void destory_##val() { \
//     delete val; \
// }

using namespace pse;

bool _jsi_record_inited = false;
FILE *_jsi_record_file;
RecordMeta *meta;

void reset_signal_handler(int signum) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;  // 设置为默认处理
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(signum, &sa, NULL) == -1) {
        perror("sigaction");
    }
}
extern std::set<pthread_t>* threads;

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

volatile bool thread_destroyed = false;
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

thread_local timer_t timer_id;
class PAPISampler {
    private:
        double sampling_interval = 1.0;
    
    public:
        PAPISampler() {
            sampling_interval = EnvConfigHelper::get_double("JSI_SAMPLING_INTERVAL", 0.1/*10Hz by default*/);
            
            startTimers();
            JSI_LOG(JSILOG_INFO, "[TID=%d] Initialize SAMPLER with sampling interval: %lf.\n", _ConfigHelper::get_tid(), sampling_interval);
        }
    
        ~PAPISampler() {
            cleaning();
            JSI_LOG(JSILOG_INFO, "[TID=%d] Finalize SAMPLER.\n", _ConfigHelper::get_tid());
        }
        // Set timer callback Function
        static void timer_handler(int signum) {
            if(!jsi_safe_enter())
                return;
            // JSI_INFO("Timer triggered , num of events %d.\n", pmu_collector_get_num_events());
            record_t * rec = (record_t*) ALLOCATE(
                sizeof(record_t) + sizeof(uint64_t) * pmu_collector_get_num_events()
            );
            rec[0].MsgType = (int16_t) event_SAMPLING_Counters;
            rec[0].timestamps.enter = get_tsc_raw();

            if(jsi_backtrace_enabled) {
                // JSI_LOG(JSILOG_INFO, "[TID=%d] sampler wants to get bt\n", _ConfigHelper::get_tid());
                rec[0].ctxt = backtrace_context_get();
            }
            
    
            if (pmu_collector_get_num_events()>0) {
                uint64_t* counters = RecordWriterHelper::counters(rec);
                int r = pmu_collector_get_all(counters);
            }
            
            // RecordWriter::traceStore((const record_t *) rec);
            RecordWriter::samplingStore(event_SAMPLING_Counters, rec);
            DEALLOCATE(rec, sizeof(record_t) + sizeof(uint64_t) * pmu_collector_get_num_events());
            // jsi_samp_safe_exit();
            jsi_safe_exit();
        }
        static void main_thread_timer_handler() 
        //static void main_thread_timer_handler(int signum)
        { 
            for (auto thread: *threads)
            {
                pthread_kill(thread, SIG_THREAD_SAMPLING);
            }
            timer_handler(-1);
        } 
   private: 
        void startTimers() {
            long sec = static_cast<long>(sampling_interval);
            long usec = static_cast<long>((sampling_interval - sec) * 1000000);

            if (_ConfigHelper::get_pid() == _ConfigHelper::get_tid())
            {
                struct sigaction sa;
                sa.sa_flags = SA_SIGINFO;
                sa.sa_sigaction = [](auto, auto, auto){main_thread_timer_handler();};
                sigemptyset(&sa.sa_mask);
                sigaction(REALTIME_SIGNAL, &sa, NULL);
                struct sigevent sev{};

                sev.sigev_notify = SIGEV_SIGNAL; 
                sev.sigev_signo = REALTIME_SIGNAL ;
                timer_create(CLOCK_REALTIME, &sev, &timer_id);
                struct itimerspec its;

                its.it_value.tv_sec = sec;
                its.it_value.tv_nsec = usec*1000;
                its.it_interval.tv_sec = sec;
                its.it_interval.tv_nsec = usec*1000;
                timer_settime(timer_id, 0, &its, NULL);
            }
            else
            {
                struct sigaction sa_thread;
                sa_thread.sa_flags = 0;
                sa_thread.sa_handler = timer_handler;
                sigemptyset(&sa_thread.sa_mask);
                sigaction(SIG_THREAD_SAMPLING, &sa_thread, NULL);
            }   
        }        
    
        // Cleaning Function
        static void cleaning() {
            //struct itimerval zero_timer;
            reset_signal_handler(REALTIME_SIGNAL);
            timer_delete(timer_id);
            return;
            struct itimerspec zero_timer;
            zero_timer.it_value.tv_sec = 0;
            zero_timer.it_value.tv_nsec = 0;
            zero_timer.it_interval.tv_sec = 0;
            zero_timer.it_interval.tv_nsec = 0;
            //setitimer(ITIMER_REAL, &zero_timer, NULL);
            timer_settime(timer_id, 0, &zero_timer, NULL);
        }
    };
thread_local PAPISampler* globalPAPISampler;


std::unique_ptr<pse::ral::BackendInterface>* _backend;
std::unique_ptr<pse::ral::DirSectionInterface>* _root;
std::unique_ptr<pse::ral::DirSectionInterface>* _process_trace_dir;
std::unique_ptr<pse::ral::DirSectionInterface>* _process_backtrace_dir;
// std::unique_ptr<pse::ral::DirSectionInterface>* _main_thread_trace_dir;

using sec_map_t = std::unordered_map<size_t, std::unique_ptr<pse::ral::DataSectionInterface>>;
using ext_sec_map_t = std::unordered_map<size_t, std::unique_ptr<pse::ral::StreamSectionInterface>>;

//std::unique_ptr<pse::ral::DirSectionInterface> _main_thread_trace_dir;
thread_local std::unique_ptr<pse::ral::DirSectionInterface> *_thread_trace_dir;
thread_local sec_map_t* _thread_sec_map;
thread_local sec_map_t* _thread_sampling_sec_map;
thread_local ext_sec_map_t* _thread_ext_sec_map;

void record_init_thread()
{
    _thread_trace_dir = new std::unique_ptr<pse::ral::DirSectionInterface>();
    _thread_sec_map = new sec_map_t();
    _thread_sampling_sec_map = new sec_map_t();
    _thread_ext_sec_map = new ext_sec_map_t();

    if (jsi_backtrace_enabled) {
        backtrace_init_recording(backtrace_max_size);
    }
    globalPAPISampler = EnvConfigHelper::get_enabled("JSI_ENABLE_SAMPLING", false) ? new PAPISampler() : nullptr;
    struct sigaction sa_thread;
    sa_thread.sa_flags = 0;
    sa_thread.sa_handler = [](auto){jsi_thread_finalize();};
    sigemptyset(&sa_thread.sa_mask);
    sigaction(SIG_THREAD_EXIT, &sa_thread, NULL);
}

thread_local bool record_fini = false;
void record_fini_thread()
{
    if (record_fini) {
        JSI_INFO("****** RECORD FINI CALLED MULTIPLE TIMES!!!!\n");
        return;
    } else {
        record_fini = true;
    }
    jsi_mark_unsafe_enter();
    JSI_INFO("****** delete globalPAPISampler\n");
    delete globalPAPISampler;
    JSI_INFO("****** delete backtrace\n");
    if (jsi_backtrace_enabled) {
        auto& process_backtrace_dir = *(_process_backtrace_dir);
        auto thread_backtrace_dir = process_backtrace_dir->openDirSection(_ConfigHelper::get_tid(), true);
        backtrace_db_dump(thread_backtrace_dir);
        backtrace_finalize();
    }
    // jsi_thread_data_mark_finalized();
    JSI_INFO("****** delete _thread_trace_dir\n");
    delete _thread_trace_dir;
    JSI_INFO("****** delete _thread_sec_map\n");
    delete _thread_sec_map;
    JSI_INFO("****** delete _thread_sampling_sec_map\n");
    delete _thread_sampling_sec_map;
    JSI_INFO("****** delete _thread_ext_sec_map\n");
    delete _thread_ext_sec_map;
    JSI_INFO("****** record_fini_thread exit\n");

    // delete sampler;
    //RecordWriter::metaSectionStart("FINALIZED");
    //RecordWriter::metaStore("SUCCESS", 1);
    //RecordWriter::metaSectionEnd("FINALIZED");
    // JSI_LOG(JSILOG_INFO, "[Record Writer] Child Thread :: finalize for tid=%d\n", _ConfigHelper::get_tid());
}

// sec_map_t& get_sec_map()
// {
//     // if (_ConfigHelper::get_tid() == _ConfigHelper::get_pid())
//     // {
//     //     return *_main_thread_sec_map;
//     // }
//     return *_thread_sec_map;
// }
// sec_map_t& get_sampling_sec_map()
// {
//     // if (_ConfigHelper::get_tid() == _ConfigHelper::get_pid())
//     // {
//     //     return *_main_thread_sampling_sec_map;
//     // }
//     return *_thread_sampling_sec_map;
// }
// ext_sec_map_t& get_ext_sec_map()
// {
//     // if (_ConfigHelper::get_tid() == _ConfigHelper::get_pid())
//     // {
//     //     return *_main_thread_ext_sec_map;
//     // }
//     return *_thread_ext_sec_map;
// }

// module construstor/destructors
__attribute__((constructor (RECORD_INIT_PRIORITY)))
void recordWriterCacheInit() {
    JSI_LOG(JSILOG_INFO, "Initialize Record Writer Library.\n");
    jsi_mark_unsafe_enter();
    // call back registration
    registerThreadSafeGuard(record_init_thread, record_fini_thread);
    // initialize storage system
    _backend = new std::unique_ptr<pse::ral::BackendInterface>();
    _root = new std::unique_ptr<pse::ral::DirSectionInterface>();
    _process_trace_dir = new std::unique_ptr<pse::ral::DirSectionInterface>();
    _process_backtrace_dir = new std::unique_ptr<pse::ral::DirSectionInterface>();
    // jsi_thread_data_mark_initialized();

    threads = new std::set<pthread_t>();

    RecordWriter::init();

    // JSI_LOG(JSILOG_INFO, "[Record Writer] Main Thread :: init for tid=%d\n", _ConfigHelper::get_tid());
    jsi_thread_init();

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
    // globalPAPISampler = EnvConfigHelper::get_enabled("JSI_ENABLE_SAMPLING", false) ? new PAPISampler() : nullptr;
    jsi_record_writer_init_completed.store(1);
    jsi_mark_unsafe_exit();
}

__attribute__((destructor (RECORD_FINI_PRIORITY)))
void recordWriterCacheFinalize() {
    jsi_mark_unsafe_enter();
    for (auto thread: *threads)
    {
        JSI_LOG(JSILOG_INFO, "have THREDID %p\n", thread);
    }
    for (auto thread: *threads)
    {
        JSI_LOG(JSILOG_INFO, "finalize THREDID %p\n", thread);
        pthread_kill(thread, SIG_THREAD_EXIT);
    }
    for (auto thread: *threads)
    {
        JSI_LOG(JSILOG_INFO, "waiting for THREDID %p to exit\n", thread);
        pthread_join(thread, NULL);
        JSI_LOG(JSILOG_INFO, "THREDID %p exited\n", thread);
    }
    // sleep(1);
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
    // delete globalPAPISampler;
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        if (!pmu_collector_fini()) {
            JSI_WARN("PMU collector finalization failed.\n");
        }
    }
#endif
    auto& process_backtrace_dir = *(_process_backtrace_dir);
    //auto thread_backtrace_dir = process_backtrace_dir->openDirSection(_ConfigHelper::get_pid(), true);
    RecordWriter::metaSectionStart("FINALIZED");
    RecordWriter::metaStore("SUCCESS", 1);
    RecordWriter::metaSectionEnd("FINALIZED");
    RecordWriter::finalize();

    // finalize storage system
    jsi_thread_finalize();
    delete _backend;
    delete _root;
    delete _process_trace_dir;
    delete _process_backtrace_dir;
    JSI_LOG(JSILOG_INFO, "[Record Writer] Main Thread :: finalize for tid=%d\n", _ConfigHelper::get_tid());

    delete meta;
    delete threads;
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
    // spdlog::set_pattern("[%Y-%m-%d %T][thread %t][%l]%v");
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
    // backend_ptr = backend.release();
    // process_backtrace_dir_ptr = process_backtrace_dir.release();
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
        // backtrace initialization should be thread local
        // backtrace_init_recording(backtrace_max_size);
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
    // auto &thread_trace_dir = (_ConfigHelper::get_tid() == _ConfigHelper::get_pid()) ? (*_main_thread_trace_dir) : (*_thread_trace_dir);
    auto &thread_trace_dir = (*_thread_trace_dir);
    if (thread_trace_dir)
    {
        return thread_trace_dir;
    }
    thread_trace_dir = (process_trace_dir)->openDirSection(_ConfigHelper::get_tid(), true);
    return thread_trace_dir;
}

using T1 = std::unordered_map<size_t, std::unique_ptr<pse::ral::DataSectionInterface>>;
using T2 = std::unordered_map<size_t, std::unique_ptr<pse::ral::StreamSectionInterface>>;
// std::unordered_map<size_t, std::unique_ptr<pse::ral::DataSectionInterface>> global_sec_map;
// DECLARE_GLOBAL_VARIABLE(T1, global_sec_map);
// thread_local ThreadLocalWrapper<std::unordered_map<size_t, std::unique_ptr<pse::ral::DataSectionInterface>>> _sec_map(*global_sec_map);
// DECLARE_GLOBAL_VARIABLE(T1, global_sampling_map);
// thread_local ThreadLocalWrapper<std::unordered_map<size_t, std::unique_ptr<pse::ral::DataSectionInterface>>> _sampling_map(*global_sampling_map);
// DECLARE_GLOBAL_VARIABLE(T2, global_ext_map);
// thread_local ThreadLocalWrapper<std::unordered_map<size_t, std::unique_ptr<pse::ral::StreamSectionInterface>>> _ext_map(*global_ext_map);

void RecordWriter::traceStore(const record_t* record)
{
    size_t id = record[0].MsgType;
    const char* t= (const char*)&record->timestamps.enter;
    auto& sec_map = *_thread_sec_map;
    auto iter = sec_map.find(id);
    if (iter == sec_map.end()) {
        auto& dir = get_thread_trace_dir();
        auto generic = dir->openDirSection(StaticSectionDesc::GENERIC_TRACE_SEC_ID, true);
        auto sec = generic->openDataSection(id + StaticSectionDesc::RECORD_SEC_OFFSET, true, record_info[id].add_size(jsi_pmu_bytes()), record_time_offset[id]);
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
    auto& sampling_map = *_thread_sampling_sec_map;
    auto iter = sampling_map.find(id);
    if (iter == sampling_map.end()) {
        auto& dir = get_thread_trace_dir();
        auto sampling = dir->openDirSection(StaticSectionDesc::SAMPLING_TRACE_SEC_ID, true);
        auto sec = sampling->openDataSection(id + StaticSectionDesc::RECORD_SEC_OFFSET, true, record_info[id].add_size(jsi_pmu_bytes()), record_time_offset[id]);
        sec->write(record);
        sampling_map[id] = std::move(sec);
    }
    else
    {
        iter->second->write(record);
    }
}
void RecordWriter::extStore(size_t id, const void* record, size_t size)
{
    auto& ext_map = *_thread_ext_sec_map;
    auto iter = ext_map.find(id);
    if (iter == ext_map.end())
    {
        auto& thread_trace_dir = get_thread_trace_dir();
        auto stream_dir_sec = thread_trace_dir->openDirSection(StaticSectionDesc::EXT_TRACE_SEC_ID, true);
        auto stream_sec = stream_dir_sec->openStreamSection(id + StaticSectionDesc::EXT_SEC_OFFSET, true);
        int size_32 = size;
        stream_sec->write(&size_32, sizeof(size_32));
        stream_sec->write(record, size);
        ext_map[id] = std::move(stream_sec);
    }
    else
    {
        int size_32 = size;
        iter->second->write(&size_32, sizeof(size_32));
        iter->second->write(record, size);
    }
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