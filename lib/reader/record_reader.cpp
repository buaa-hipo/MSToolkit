#include "record/record_reader.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "instrument/backtrace.h"
#include "instrument/pmu_collector.h"
#include "record/record_defines.h"
#include "record/record_type.h"

#include "utils/tsc_timer.h"
#include "fsl/raw_backend.h"

// Using <filesystem> introduced in C++17
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

#include <mpi.h>
#include <set>

namespace fs = std::filesystem;

RecordTraceExt* loadRecordTraceExt(const char* filename) {
    RecordTraceExtBase* rte = new RecordTraceExtBase(filename);
    switch(rte->get_header()) {
#ifdef ROCM
        case JSI_TRACE_ACCL_ROCM:
            return new RecordTraceExtAcclRocm(rte);
#endif
        default:
            return new RecordTraceExt(rte);
    }
    return NULL;
}

RecordTrace::RecordTrace(void *trace_mmap, int rank, int start, int end, int size, bool isGlobal,
                         const std::vector<std::string> &pmu_event_list,int model, int cache_size) {
    _rank = rank;
    _trace_mmap = trace_mmap;
    _trace_view_start = trace_mmap;
    _trace_view_end = (void *) (((char *) trace_mmap) + size);
    _global_trace_view_start = trace_mmap;
    _global_trace_view_end = (void *) (((char *) trace_mmap) + size);
    _start = start;
    _end = end;
    _size = size;
    _cache_size = cache_size;
    _is_global = isGlobal;
    _model = model;
    _pmu_event_list = pmu_event_list;
}

RecordTrace::RecordTrace(std::unique_ptr<pse::ral::DirSectionInterface>&& _sec, int rank, bool isGlobal,
                         const std::vector<std::string> &pmu_event_list) :_sec(std::move(_sec)){
    _rank = rank;
    _is_global = isGlobal;
    _model = SECTION_MODEL;
    _pmu_event_list = pmu_event_list;
    _trace_mmap = nullptr;
    _trace_view_start = nullptr;
    _trace_view_end = nullptr;
}

RecordTrace::~RecordTrace() {
    if (_is_global && _trace_mmap) {
        munmap(_trace_mmap, _size);
    } else {
        free(_trace_view_start);
    }
}

void RecordTrace::detect_size() {
    _start = 0;
    _end = 0;
    RecordTraceIterator it = begin();
    while (it != end()) {
        it = it.next();
        ++_end;
    }
}

bool RecordTrace::zoom(uint64_t ts_s, uint64_t ts_e, uint64_t offset) {
    // 前置校验
    if (ts_s > ts_e) {
        return false;
    }
    // 设置头节点和末尾节点，记录具体信息
    RecordTraceIterator it = begin();
    RecordTraceIterator newIt = RecordTraceIterator::invalid();
    RecordTraceIterator newIe = RecordTraceIterator::invalid();
    int iterNum = 0;
    while (it != global_end() && it.val()->timestamps.enter - offset < ts_s) {
        it = it.next();
    }
    if (it != global_end() && it.val()->timestamps.enter- offset >= ts_s && it.val()->timestamps.enter- offset < ts_e) {
        newIt = it;
        _start = iterNum;
        while (it != global_end() && it.val()->timestamps.enter- offset < ts_e) {
            // printf("%ld, %ld, %ld\n", it.val()->timestamps.enter, ts_s, ts_e);
            it = it.next();
            iterNum++;
        }
        _end = iterNum;
        newIe = it;
    }
    if (!RecordTraceIterator::is_invalid(newIt)) {
        if (_model == SECTION_MODEL)
        {
            zoom_begin = newIt;
            zoom_end = newIe;
            return true;
        }
        _trace_view_start = (void *) ((uint8_t *) newIt.val());
        _trace_view_end = (void *) ((uint8_t *) newIe.val());
        return true;
    }
    return false;
}

// star
// RecordTrace *RecordTrace::filter(int MsgType) {
//     RecordTraceIterator it = begin();
//     RecordTraceIterator ie = end();
//     std::vector<void *> ptrList;
//     size_t memAllocateSize = 0;
//     int iterNum = 0, firstIdx = -1, lastIdx = -1;
//     while (it != ie) {
//         if (it.val()->MsgType == MsgType) {
//             lastIdx = iterNum;
//             ptrList.push_back((void *) ((uint8_t *) it.val()));
//             memAllocateSize += RecordHelper::get_record_size(it.val(), _pmu_event_list.size());
//         }
//         if (ptrList.size() == 1) {
//             firstIdx = iterNum;
//         }
//         iterNum++;
//         it = it.next();
//     }
//     if (firstIdx == -1) {
//         return nullptr;
//     }
//     void *ptr = malloc(memAllocateSize);
//     size_t recordSize = memAllocateSize / ptrList.size();
//     for (int i = 0; i < ptrList.size(); i++) {
//         memcpy((void *) ((uint8_t *) ptr + recordSize * i), ptrList[i], recordSize);
//     }
//     RecordTrace *trace = new RecordTrace(ptr, this->_rank, firstIdx, lastIdx, memAllocateSize,
//                                          false, _pmu_event_list,this->_model);
//     return trace;
// }

RecordReader::RecordReader(const char *dir, int model, const char* dwarf_dir, bool enable_dbinfo, bool mpi_only, bool enable_backtrace) {
    this->_model = model;
    this->_mpi_only = mpi_only;
    load(dir, dwarf_dir, enable_dbinfo, enable_backtrace);
}

RecordReader::~RecordReader() {
    for (auto it = _record_collection.begin(); it != _record_collection.end(); ++it) {
        delete it->second;
    }
    _record_collection.clear();
    for (auto it = _meta_collection.begin(); it != _meta_collection.end(); ++it) {
        delete it->second;
    }
    _meta_collection.clear();
}

// This interface will be removed during finali release of new version.
void RecordReader::load_meta(const char* path) {
    RankMetaCollection& metas = _meta_collection;
    std::string id_str(fs::path(path).stem().extension().c_str() + 1);
    if (id_str.size() > 0) {
        JSI_INFO("RecordReader::load(): loading meta... identifier=%s\n", id_str.c_str());
        RecordMeta *meta = new RecordMeta(path);
        auto &metaMap = meta->getMetaMap();
        MetaDataMap::MetaValue_t *value_ranks;
        if(metaMap.count("MEMORY_INFO") != 0) {
            metaMap.at("MEMORY_INFO")->get("rank", &value_ranks);
        }
        else if(metaMap.count("MPI_COMM_WORLD") != 0) {
            metaMap.at("MPI_COMM_WORLD")->get("rank", &value_ranks);
        }
        else {
            JSI_ERROR("Broken trace data: missed metaSection info for identifier %s.\n", id_str.c_str());
        }   
        // metaMap.at("MPI_COMM_WORLD")->get("rank", &value_ranks);
        int rank = value_ranks->i32;
        metas[rank] = meta;
        _id2rank[id_str] = rank;
        if (metaMap.count("PMU COLLECTOR")) {
            // initialize pmu collector related stuff if it exists
            MetaDataMap::MetaValue_t *value_num_events, *value_event_list;
            metaMap.at("PMU COLLECTOR")->get("PMU_NUM_EVENTS", &value_num_events);
            metaMap.at("PMU COLLECTOR")->get("PMU_EVENT_LIST", &value_event_list);

            auto &event_list = _pmu_event_list_collection[rank];
            event_list.clear();
            parse_pmu_event_list(value_event_list->ptr, &event_list);
            if (value_num_events->i32 != event_list.size()) {
                JSI_ERROR(
                        "PMU_NUM_EVENTS and PMU_EVENT_LIST do not match!\n"
                        "PMU_NUM_EVENTS==%d while %zu events exist in PMU_EVENT_LIST\n",
                        value_num_events->i32, event_list.size());
            }
            JSI_INFO(
                    "RecordReader::load(): rank %d has %d pmu event(s). "
                    "The full event list: %s\n",
                    rank, value_num_events->i32, value_event_list->ptr);
        }
    }
}

void RecordReader::load_meta(std::unique_ptr<pse::ral::DirSectionInterface>& dir, const std::string& node_id) {
    RankMetaCollection& metas = _meta_collection;
    RecordMeta *meta = new RecordMeta(dir);
    auto &metaMap = meta->getMetaMap();
    MetaDataMap::MetaValue_t *value_ranks;
    if(metaMap.count("MPI_COMM_WORLD") != 0) {
        metaMap.at("MPI_COMM_WORLD")->get("rank", &value_ranks);
    }
    else if(metaMap.count("HOST INFO") != 0) {
        if(_mpi_only) {
            JSI_WARN("Ignore as the trace is not a MPI trace: %s", node_id + std::to_string(dir->self_desc()));
            _id2rank[node_id + std::to_string(dir->self_desc())] = -1;
            delete meta;
            return ;
        }
        // if no rank info, encode from PID
        metaMap.at("HOST INFO")->get("PID", &value_ranks);
        value_ranks->i32 = -value_ranks->i32;
    }
    else {
        JSI_ERROR("Broken trace data: missed metaSection info for identifier not yet.\n");
    }   
    // metaMap.at("MPI_COMM_WORLD")->get("rank", &value_ranks);
    int rank = value_ranks->i32;
    if (metas.count(rank)!=0) {
        JSI_ERROR("Multiple Meta with the same rank detected!");
    }
    _id2rank[node_id + std::to_string(dir->self_desc())] = rank;
    metas[rank] = meta;
    if (metaMap.count("PMU COLLECTOR")) {
        // initialize pmu collector related stuff if it exists
        MetaDataMap::MetaValue_t *value_num_events, *value_event_list;
        metaMap.at("PMU COLLECTOR")->get("PMU_NUM_EVENTS", &value_num_events);
        metaMap.at("PMU COLLECTOR")->get("PMU_EVENT_LIST", &value_event_list);

        auto &event_list = _pmu_event_list_collection[rank];
        event_list.clear();
        parse_pmu_event_list(value_event_list->ptr, &event_list);
        if (value_num_events->i32 != event_list.size()) {
            JSI_ERROR(
                    "PMU_NUM_EVENTS and PMU_EVENT_LIST do not match!\n"
                    "PMU_NUM_EVENTS==%d while %zu events exist in PMU_EVENT_LIST\n",
                    value_num_events->i32, event_list.size());
        }
        JSI_INFO(
                "RecordReader::load(): rank %d has %d pmu event(s). "
                "The full event list: %s\n",
                rank, value_num_events->i32, value_event_list->ptr);
    }
}

void RecordReader::load_trace(const char* path) {
    JSI_INFO("## loading %s\n", path);
    std::string id_str(fs::path(path).stem().extension().c_str() + 1);
    if (id_str.size() > 0) {
        if (_id2rank.find(id_str) == _id2rank.end()) {
            JSI_ERROR("Broken trace data: missed rank info for identifier %s.\n", id_str.c_str());
        }
        int rank = _id2rank[id_str];
        if (rank >= 0) {
            JSI_INFO("RecordReader::load(): rank=%d\n", rank);
            _load_trace(path, rank);
        }
    }
}

void RecordReader::load_trace(std::unique_ptr<pse::ral::DirSectionInterface>&& dir, const std::string& node_id) {
    auto rank = _id2rank.at(node_id + std::to_string(dir->self_desc()));
    auto generic_dir = dir->openDirSection(StaticSectionDesc::GENERIC_TRACE_SEC_ID, false);
    if (!_mpi_only || rank>=0) {
        _record_collection[rank] = new RecordTrace(std::move(generic_dir), rank, true,
                                                   _pmu_event_list_collection[rank]);
    }
}

void RecordReader::load_backtrace(const char* path, const char* dwarf_dir, bool enable_dbinfo) {
    JSI_INFO("## loading %s\n", path);
    std::string id_str(fs::path(path).stem().extension().c_str() + 1);
    if (!id_str.empty()) {
        if (_id2rank.find(id_str) == _id2rank.end()) {
            JSI_ERROR("Broken trace data: missed rank info for identifier %s.\n", id_str.c_str());
        }
        int rank = _id2rank[id_str];
        JSI_INFO("RecordReader::load(): loading backtrace... rank=%d\n", rank);
        _load_backtrace(path, rank, dwarf_dir, enable_dbinfo);
    }
}

void RecordReader::load_backtrace(std::unique_ptr<pse::ral::DirSectionInterface>& dir, const std::string& node_id, const char* dwarf_dir, bool enable_dbinfo) {
    auto rank = _id2rank.at(node_id + std::to_string(dir->self_desc()));
    if (!_mpi_only || rank>=0) {
        JSI_INFO("RecordReader::load(): loading backtrace... rank=%d\n", rank);
        _load_backtrace(dir, rank, dwarf_dir, enable_dbinfo);
    }
}

void RecordReader::load_etr(const char* path) {
    JSI_INFO("## loading %s\n", path);
    std::string id_str(fs::path(path).stem().extension().c_str() + 1);
    if (!id_str.empty()) {
        if (_id2rank.find(id_str) == _id2rank.end()) {
            JSI_ERROR("Broken trace data: missed rank info for identifier %s.\n", id_str.c_str());
        }
        int rank = _id2rank[id_str];
        JSI_INFO("RecordReader::load(): loading extended trace... rank=%d\n", rank);
        _record_ext_collection[rank] = loadRecordTraceExt(path);
    }
}

void RecordReader::load(const char *dir, const char* dwarf_dir, bool enable_dbinfo, bool enable_backtrace) {
    TICK("RecordReader::load initialization");
    auto trace_ext = fs::path(JSI_TRACE_FILE_EXT);
    auto meta_ext = fs::path(JSI_META_FILE_EXT);
    auto backtrace_ext = fs::path(JSI_BACKTRACE_FILE_EXT);
    auto etr_ext = fs::path(JSI_EXT_TRACE_FILE_EXT);
    auto etr_sym_ext = fs::path(JSI_EXT_TRACE_SYM_FILE_EXT);
    if (this->_model == DATA_MODEL) {
        trace_ext = fs::path(JSI_TRACE_FILE_EXT);
    } else if (this->_model == COMPRESS_MODEL) {
        trace_ext = fs::path(JSI_TRACE_READ_COMPRESS_EXT);
    } else if (this->_model == SECTION_MODEL) {
        section_load(dir, dwarf_dir, enable_dbinfo, enable_backtrace);
        return;
    } else {
        JSI_ERROR("Invalid data model:%d", this->_model);
        exit(-1);
    }

    TOCK("RecordReader::load initialization");
    TICK("RecordReader::load metadata loading");

    for (const auto &dirEntry: fs::directory_iterator(dir)) {
        if (dirEntry.is_regular_file()) {
            const auto &path = dirEntry.path();
            if (path.extension() == meta_ext) {
                load_meta(path.c_str());
            }
        }
    }

    TOCK("RecordReader::load metadata loading");

    TICK("RecordReader::load trace & bt loading");
    // walk through the directory
    for (const auto &dirEntry: fs::directory_iterator(dir)) {
        //std::cout << dirEntry << std::endl;
        if (dirEntry.is_regular_file()) {
            const auto &path = dirEntry.path();
            if (path.extension() == trace_ext) {
                load_trace(path.c_str());
            }  else if (path.extension() == backtrace_ext && this->_model == DATA_MODEL) {
                load_backtrace(path.c_str(), dwarf_dir, enable_dbinfo);
            } else if (path.extension() == meta_ext) {
                // Skip. Metas are already read
            } else if (path.extension() == etr_sym_ext) {
                // Skip. Handle together with etr extension
            } else if (path.extension() == etr_ext) {
                load_etr(path.c_str());
            } else {
                JSI_WARN("RecordReader::load>> Ignore file %s with unknown extension.\n",
                         path.c_str());
            }

        }
    }
    TOCK("RecordReader::load trace & bt loading");
}
void RecordReader::section_load(const char *dir, const char* dwarf_dir, bool enable_dbinfo, bool enable_backtrace) {
    for (const auto &dirEntry: fs::directory_iterator(dir)) {
        //std::cout << dirEntry << std::endl;
        if (dirEntry.is_regular_file()) {
            const auto &path = dirEntry.path();
            auto path_s = path.string();
            if (path.extension() == JSI_SECTION_FILE_EXT) {
                auto backend = pse::fsl::RawSectionBackend(std::string_view(path_s), pse::ral::READ);
                auto wrapper = std::make_unique<pse::ral::BackendWrapper<decltype(backend)>>(std::move(backend));
                std::unique_ptr<pse::ral::BackendInterface> backend_ptr = std::move(wrapper);

                _node2backends.emplace(path_s, std::move(backend_ptr));
            } else {
                JSI_WARN("RecordReader::load>> Ignore file %s with unknown extension.\n",
                         path.c_str());
            }

        }
    }

    for (auto& [node_id, backend] : _node2backends) {
        auto root = backend->openRootSection();
        auto trace_dir = root->openDirSection(StaticSectionDesc::TRACE_SEC_ID, false);
        auto backtrace_dir = root->openDirSection(StaticSectionDesc::BACKTRACE_SEC_ID, false);
        for (auto process_dir_iter = trace_dir->begin(); process_dir_iter != trace_dir->end(); ++process_dir_iter)
        {
            spdlog::info("process_dir_iter desc: {}", process_dir_iter.getDesc());
            auto process_dir = process_dir_iter.getDirSection();
            load_meta(process_dir, node_id);
            for (auto thread_dir_iter = process_dir->begin(); thread_dir_iter != process_dir->end(); ++thread_dir_iter)
            {
                if (!thread_dir_iter.isa(pse::ral::SectionBase::DIR)) {
                    continue;
                }
                spdlog::info("thread_dir_iter desc: {}", thread_dir_iter.getDesc());
                auto thread_dir = thread_dir_iter.getDirSection();
                spdlog::info("load meta success");
                //load_backtrace(thread_dir, node_id, dwarf_dir, enable_dbinfo);
                load_trace(std::move(thread_dir), node_id);
            }

        }
        if (backtrace_dir!=nullptr) {
            for (auto process_dir_iter = backtrace_dir->begin(); process_dir_iter != backtrace_dir->end(); ++process_dir_iter)
            {
                spdlog::info("process_dir_iter desc: {}", process_dir_iter.getDesc());
                auto process_dir = process_dir_iter.getDirSection();
                if (enable_backtrace)
                {
                    load_backtrace(process_dir, node_id, dwarf_dir, enable_dbinfo);
                }
            }
        }
    }
}


void RecordReader::_load_trace(const char *fn, int rank) {
    // open the file, read only
    int fd = open(fn, O_RDONLY);
    off_t fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);// reset
    // load trace
    void *trace_mmap = mmap(0, fsize, PROT_READ, MAP_PRIVATE /*may use MAP_SHARED?*/, fd, 0);
    _record_collection[rank] = new RecordTrace(trace_mmap, rank, -1, -1, fsize, true,
                                               _pmu_event_list_collection[rank],this->_model);
    // close the file
    close(fd);
}

// void RecordReader::_load_trace(std::unique_ptr<pse::ral::DirSectionInterface> &dir) {
//     // open the file, read only
//     // close the file
// }

void RecordReader::_load_backtrace(const char *fn, int rank, const char* dwarf_dir, bool enable_dbinfo) {
    auto f = fopen(fn, "r");
    auto bt_tree = jsi::toolkit::BacktraceTree::create_loading_bt_tree();
    bt_tree->backtrace_db_load(f, dwarf_dir, enable_dbinfo);
    fclose(f);
    _backtrace_collection[rank] = bt_tree;
}
void RecordReader::_load_backtrace(std::unique_ptr<pse::ral::DirSectionInterface>& dir, int rank, const char* dwarf_dir, bool enable_dbinfo) {
    auto bt_tree = jsi::toolkit::BacktraceTree::create_loading_bt_tree();
    bt_tree->backtrace_db_load(dir, dwarf_dir, enable_dbinfo);
    _backtrace_collection[rank] = bt_tree;
}

RecordTrace &RecordReader::get_trace(int rank) {
    return *(_record_collection[rank]);
}

RecordTraceCollection &RecordReader::get_all_traces() {
    return _record_collection;
}

RecordTraceExtCollection &RecordReader::get_all_ext_traces() {
    return _record_ext_collection;
}

// make sure that rank is existed.
RecordMeta &RecordReader::get_meta_map(int rank) {
    return *(_meta_collection[rank]);
}

RankMetaCollection &RecordReader::get_all_meta_maps() {
    return _meta_collection;
}

BacktraceTree &RecordReader::get_backtrace(int rank) {
    return *(_backtrace_collection[rank]);
}

BacktraceCollection &RecordReader::get_all_backtraces() {
    return _backtrace_collection;
}

bool RecordReader::has_backtrace() const {
    return !_backtrace_collection.empty();
}

int RecordReader::get_pmu_event_num_by_rank(int rank){
    return this->_pmu_event_list_collection[rank].size();
}


RecordTraceExtBase::RecordTraceExtBase(const char *filename) {
    // open the file, read only
    int fd = open(filename, O_RDONLY);
    off_t fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);// reset
    // load trace
    _trace = (uint8_t*)mmap(0, fsize, PROT_READ, MAP_PRIVATE /*may use MAP_SHARED?*/, fd, 0);
    header = (JSI_TRACE_HEADER)_trace[0];
    cur = 1;
    _size = fsize;
    // close the file
    close(fd);
    // open the symbol file if exists
    std::string file_path_sym(filename);
    file_path_sym += "-sym";
    fd = open(file_path_sym.c_str(), O_RDONLY);
    fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char* smap = (char*)mmap(0, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    int scur = 0;
    while(scur < fsize) {
        int sk = scur;
        uint64_t key = *(reinterpret_cast<uint64_t*>(&smap[sk]));
        int ss = scur + sizeof(uint64_t);
        std::string str = std::string(&smap[ss]);
        _string_map[key] = str;
        scur = ss + str.size() + 1;
        // printf("### key=%llu, str=%s\n", key, str.c_str());
    }
    // close the file
    close(fd);
}

JSI_TRACE_HEADER RecordTraceExtBase::get_header() {
    return header;
}

/* Unsafe string getter. */
std::string RecordTraceExtBase::get_string(uint64_t key) {
    if(_string_map.find(key)==_string_map.end()) {
        printf("WARNING: [RecordTraceExtBase::get_string] key=%lx not found in symbol map!\n", key);
    }
    return _string_map[key];
}

int RecordTraceExtBase::loadSection(uint8_t** begin) {
    // printf("cur=%d, _size=%d, _trace=%p\n", cur, _size, _trace);
    if (cur>=_size) { return 0; }
    uint64_t* sec_start = reinterpret_cast<uint64_t*>(&_trace[cur]);
    uint64_t size = sec_start[0];
    *begin = reinterpret_cast<uint8_t*>(&sec_start[1]);
    cur += size + sizeof(uint64_t);
    // printf("^^ update: cur=%d, sec_start=%p, size=%d, begin=%p\n", cur, sec_start, size, *begin);
    return size;
}

void RecordTraceExtBase::reset() {
    // the head 1 byte is the magical code for EXT TRACE type identifier.
    cur = 1;
}

#ifdef ROCM
std::string RecordTraceExtAcclRocm::dump() {
    std::stringstream desc;
    // desc << "#### <ACCL ROCM> Section " << (void*)begin << ", " << (void*)end << ", " << size << "\n";
    for(RecordTraceExtIterator it = begin(); it.valid(); it.next()) {
        ext_record_accl* record = (ext_record_accl*)it.get();
        desc << "\t^^ " << this->get_string(record->sym_key.key) 
             << "(" << record->sym_key.accl_key.domain << "," << record->sym_key.accl_key.op << "," << record->sym_key.accl_key.kind << ")\n" 
             << "\t\tcorrelation_id(" << record->correlation_id << ")\n"
             << "\t\ttime_ns(" << record->begin_ns << ":" << record->end_ns << ")\n"
             << "\t\tduration_ns(" << record->end_ns-record->begin_ns << ")\n";
    }
    return desc.str();
}
#endif

void ParallelRecordReaderUnordered::load(const char *dir, const char* dwarf_dir, bool enable_dbinfo) {
    int initialized;
    MPI_Initialized(&initialized);
    if(!initialized) {
        JSI_ERROR("ParallelRecordReader: MPI not initialized!");
    }
    
    TICK("ParallelRecordReaderUnordered::load split read workloads");

    MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &_size);
    auto trace_ext = fs::path(JSI_TRACE_FILE_EXT);
    auto meta_ext = fs::path(JSI_META_FILE_EXT);
    auto backtrace_ext = fs::path(JSI_BACKTRACE_FILE_EXT);
    auto etr_ext = fs::path(JSI_EXT_TRACE_FILE_EXT);
    auto etr_sym_ext = fs::path(JSI_EXT_TRACE_SYM_FILE_EXT);
    if (this->_model == DATA_MODEL) {
        trace_ext = fs::path(JSI_TRACE_FILE_EXT);
    } else if (this->_model == COMPRESS_MODEL) {
        trace_ext = fs::path(JSI_TRACE_READ_COMPRESS_EXT);
    } else {
        JSI_ERROR("Invalid data model:%d", this->_model);
        exit(-1);
    }
    // trace, meta, and backtrace can be formed via ids, so only distribute ids.
    // etr may be differed due to its extention trace type, so we need the exact prefix to generate correct filename.
    std::set<std::string> etr_prefix_list;
    // only master rank load all the filename and split to each rank;
    if (_rank==0) {
        int i = 0;
        std::vector<std::string> id_str_list(_size);
        std::unordered_map<std::string, int> id_map;
        for (const auto &dirEntry: fs::directory_iterator(dir)) {
            if (dirEntry.is_regular_file()) {
                const auto &path = dirEntry.path();
                if (path.extension() == meta_ext) {
                    std::string id_str(path.stem().extension().c_str() + 1);
                    if (id_str.size() > 0) {
                        id_map[id_str] = i;
                        if(i==0) {
                            _id_list.push_back(id_str);
                        } else {
                            id_str_list[i] = id_str_list[i] + id_str + std::string("\n");
                        }
                        i = (i+1) % _size;
                    }
                } else if (path.extension() == etr_ext) {
                    // MEASUREMENT_DIR/<prefix>.<id_str>.etr
                    std::string prefix(path.filename().stem().stem().c_str());
                    if (prefix.size()>0) {
                        etr_prefix_list.insert(prefix);
                    }
                }
            }
        }
        // printf("Requiring %d MPI_Request\n", 2*2*(_size-1));
        MPI_Request* req = new MPI_Request[2*2*(_size-1)];
        int* send_size = new int[2*_size];
        for (i=1; i<_size; ++i) {
            send_size[i] = id_str_list[i].size();
            // printf("SEND SIZE to Rank %d: %d\n", i, send_size[i]);
            MPI_Isend(&send_size[i], 1, MPI_INT, i, 100, MPI_COMM_WORLD, &req[i-1]);
            MPI_Isend(id_str_list[i].c_str(), send_size[i], MPI_CHAR, i, 101, MPI_COMM_WORLD, &req[_size+i-2]);
        }
        std::string prefix_encode;
        for (const auto &p: etr_prefix_list) {
            prefix_encode += p + "\n";
        }
        for (i=1; i<_size; ++i) {
            send_size[_size+i] = prefix_encode.size();
            // printf("SEND PREFIX SIZE to Rank %d: %d\n", i, send_size[_size+i]);
            MPI_Isend(&send_size[_size+i], 1, MPI_INT, i, 200, MPI_COMM_WORLD, &req[2*(_size-1)+i-1]);
            MPI_Isend(prefix_encode.c_str(), send_size[_size+i], MPI_CHAR, i, 201, MPI_COMM_WORLD, &req[3*(_size-1)+i-1]);
        }
        MPI_Waitall(2*2*(_size-1), req, MPI_STATUSES_IGNORE);
        delete[] req;
        delete[] send_size;
    } else {
        {
            int recv_size;
            MPI_Recv(&recv_size, 1, MPI_INT, 0, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // printf("RECV SIZE: %d\n", recv_size);
            char* recv_data = new char[recv_size+1];
            MPI_Recv(recv_data, recv_size, MPI_CHAR, 0, 101, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            recv_data[recv_size] = '\0';
            std::stringstream ss(recv_data);
            std::string item;
            while (std::getline(ss, item)) {
                if (!item.empty()) {
                    _id_list.push_back(item);
                }
            }
            delete[] recv_data;
        }
        {
            int recv_size;
            MPI_Recv(&recv_size, 1, MPI_INT, 0, 200, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // printf("RECV ETR PREFIX SIZE: %d\n", recv_size);
            char* recv_data = new char[recv_size+1];
            MPI_Recv(recv_data, recv_size, MPI_CHAR, 0, 201, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            recv_data[recv_size] = '\0';
            std::stringstream ss(recv_data);
            std::string item;
            while (std::getline(ss, item)) {
                if (!item.empty()) {
                    etr_prefix_list.insert(item);
                }
            }
            delete[] recv_data;
        }
    }
    TOCK("ParallelRecordReaderUnordered::load split read workloads");
    TICK("ParallelRecordReaderUnordered::load load meta");
    // load meta data
    for (const auto &it: _id_list) {
        fs::path meta_file = getMetaFileName(dir, it);
        load_meta(meta_file.c_str());
    }
    TOCK("ParallelRecordReaderUnordered::load load meta");
    // load backtrace data
    TICK("ParallelRecordReaderUnordered::load load backtrace");
    for (const auto &it: _id_list) {
        fs::path bt_file = getBacktraceFileName(dir, it);
        load_backtrace(bt_file.c_str(), dwarf_dir, enable_dbinfo);
    }
    TOCK("ParallelRecordReaderUnordered::load load backtrace");
    // load trace
    TICK("ParallelRecordReaderUnordered::load load trace");
    for (const auto &it: _id_list) {
        fs::path tr_file;
        switch(this->_model) {
            case DATA_MODEL:
                tr_file = getTraceFileName(dir, it);
                break;
            case COMPRESS_MODEL:
                tr_file = getTraceCompressReadFileName(dir, it);
                break;
            default:
                JSI_ERROR("Unknown trace record model: %d\n", this->_model);
        }
        load_trace(tr_file.c_str());
    }
    TOCK("ParallelRecordReaderUnordered::load load trace");
    // load trace ext if exist
    TICK("ParallelRecordReaderUnordered::load load etr");
    for (const auto &p: etr_prefix_list) {
        for (const auto &it: _id_list) {
            fs::path etr_file = std::string(dir) + "/" + p + "." + it + JSI_EXT_TRACE_FILE_EXT;
            load_etr(etr_file.c_str());
        }
    }
    TOCK("ParallelRecordReaderUnordered::load load etr");
}