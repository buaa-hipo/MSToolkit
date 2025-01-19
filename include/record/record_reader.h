#ifndef __JSI_RECORD_READER_H__
#define __JSI_RECORD_READER_H__


#include <unordered_map>
#include <vector>

#include "record/record_defines.h"
#include "record/record_meta.h"
#include "record/record_type.h"
#include "utils/jsi_log.h"
#include "ral/section.h"
#include "ral/extractor.h"
#include "record/record_utils.h"

// Helper functions
#include "record/accl_helper.h"
#define JSI_GET_ACCL_NAME(x) (accl_tracer_get_name(JSI_GET_ACCL_API_ID_FROM_MSGTYPE(x)))
#define JSI_ACCL_EVENT_IS_LAUNCH(x) (accl_is_launch(JSI_GET_ACCL_API_ID_FROM_MSGTYPE(x)))
#define JSI_ACCL_EVENT_IS_MEMCPY(x) (accl_is_memcpy(JSI_GET_ACCL_API_ID_FROM_MSGTYPE(x)))
#define JSI_ACCL_EVENT_IS_MEMCPY_ASYNC(x) (accl_is_memcpy_async(JSI_GET_ACCL_API_ID_FROM_MSGTYPE(x)))

struct RecordHelper {
    /* TODO: implement record name lookup function via MsgType */
    static inline __attribute__((always_inline))
    std::string get_record_name(record_t *r) {
        if (JSI_MSG_IS_FUNC(r->MsgType)) {
            return std::string("Function");
        }
        if (JSI_MSG_IS_ACCL_API(r->MsgType)) {
            return std::string(JSI_GET_ACCL_NAME(r->MsgType));
        }
        switch (r->MsgType) {
            __RECORD_MAP_IMPL;
            // case JSI_PROCESS_START:
            //     return std::string("PROCESS_START");
            // case JSI_PROCESS_EXIT:
            //     return std::string("PROCESS_EXIT");
            case JSI_TOOL_IO_MSGTYPE:
                return std::string("Profiler IO");
            case JSI_ACCL_ACTIVITY_EVENT:
                return std::string("ACCL ACTIVITY EVENT");
            default:
                return std::string("Unknown");
        }
        return std::string("Unknown");
    }

    // Get size of record (including tailing pmu event counters if `num_pmu_events` > 0)
    static inline __attribute__((always_inline))
    size_t get_record_size(record_t *r, size_t num_pmu_events = 0) {
        size_t size;
        switch (r->MsgType) {
            // TODO: query based on wrapper defines
            case event_MPI_Send:
            case event_MPI_Recv:
                size = sizeof(record_comm_t);
                break;
            case event_MPI_Isend:
            case event_MPI_Irecv:
                size = sizeof(record_comm_async_t);
                break;
            case event_MPI_Wait:
                size = sizeof(record_comm_wait_t);
                break;
            case event_MPI_Alltoall:
            case event_MPI_Alltoallv:
                size = sizeof(record_all2all_t);
                break;
            case event_MPI_Allreduce:
                size = sizeof(record_allreduce_t);
                break;
            case event_MPI_Reduce:
                size = sizeof(record_reduce_t);
                break;
            case event_MPI_Bcast:
                size = sizeof(record_bcast_t);
                break;
            case event_MPI_Comm_rank:
                size = sizeof(record_comm_rank_t);
                break;
            case event_MPI_Comm_dup:
                size = sizeof(record_comm_dup_t);
                break;
            case event_MPI_Comm_split:
                size = sizeof(record_comm_split_t);
                break;
            case event_MPI_Barrier:
                size = sizeof(record_barrier_t);
                break;
            case event_Memory_Malloc:
                size = sizeof(record_memory_malloc);
                break;
            case event_Memory_Free:
                size = sizeof(record_memory_free);
                break;
            case event_Memory_Calloc:
                size = sizeof(record_memory_calloc);
                break;
            case event_Memory_Realloc:
                size = sizeof(record_memory_realloc);
                break;
            default:
                if(JSI_MSG_IS_ACCL_API(r->MsgType)) {
                    if(JSI_ACCL_EVENT_IS_LAUNCH(r->MsgType)) {
                        size = sizeof(record_activity_launch_t);
                    } else if (JSI_ACCL_EVENT_IS_MEMCPY(r->MsgType)) {
                        size = sizeof(record_activity_memcpy_t);
                    } else if (JSI_ACCL_EVENT_IS_MEMCPY_ASYNC(r->MsgType)) {
                        size = sizeof(record_activity_memcpy_async_t);
                    } else {
                        size = sizeof(record_activity_t);
                    }
                } else {
                    size = sizeof(record_t);
                }
                break;
        }
        // if (r->MsgType != JSI_PROCESS_START && r->MsgType != JSI_PROCESS_EXIT) 
        {   /* FIX20240211@YX: As PMU initialization is completed at record writer 
             * initialization, it is able to get pmu metrics at process start and end.
            */
            size_t size_pmu_metrics = 2 * num_pmu_events * sizeof(uint64_t);
            size += size_pmu_metrics;
        }
        return size;
    }

    static inline __attribute__((always_inline))
    size_t get_compressed_record_size(record_t *r,int num_pmu_events) {
        size_t size = 0;
        switch (r->MsgType) {
            // TODO: query based on wrapper defines
            case event_MPI_Send:
            case event_MPI_Recv:
                size =  sizeof(record_comm_compress_t);
                break;
            case event_MPI_Isend:
            case event_MPI_Irecv:
                size =  sizeof(record_comm_async_compress_t);
                break;
            case event_MPI_Wait:
                size = sizeof(record_comm_wait_t);
                break;
            case event_MPI_Alltoall:
            case event_MPI_Alltoallv:
                size = sizeof(record_all2all_compress_t);
                break;
            case event_MPI_Allreduce:
                size = sizeof(record_allreduce_compress_t);
                break;
            case event_MPI_Reduce:
                size = sizeof(record_reduce_compress_t);
                break;
            case event_MPI_Bcast:
                size = sizeof(record_bcast_compress_t);
                break;
            case event_MPI_Comm_rank:
                size = sizeof(record_comm_rank_t);
                break;
            case event_MPI_Comm_dup:
                size = sizeof(record_comm_dup_t);
                break;
            case event_MPI_Comm_split:
                size = sizeof(record_comm_split_t);
                break;
            case event_MPI_Barrier:
                size = sizeof(record_barrier_t);
                break;
            default:
                size = sizeof(record_t);
                break;
        }
        // if (r->MsgType != JSI_PROCESS_START && r->MsgType != JSI_PROCESS_EXIT) 
        {   /* FIX20240211@YX: As PMU initialization is completed at record writer 
             * initialization, it is able to get pmu metrics at process start and end.
            */
            size_t size_pmu_metrics = 2 * num_pmu_events * sizeof(uint64_t);
            size += size_pmu_metrics;
        }
        return size;
    }

    /* always inlined boolean helper functions to
     * determine the record type via header msg */
    static inline __attribute__((always_inline))
    bool is_function(record_t *r) {
        return JSI_MSG_IS_FUNC(r->MsgType);
    }

    static inline __attribute__((always_inline))
    bool is_mpi(record_t *r) {
        return r->MsgType >= 0;
    }

    static inline __attribute__((always_inline))
    bool is_accl(record_t *r) {
        return JSI_MSG_IS_ACCL_API(r->MsgType);
    }

    static inline __attribute__((always_inline))
    bool is_profiler(record_t *r) {
        return r->MsgType == JSI_TOOL_IO_MSGTYPE;
    }

    static inline __attribute__((always_inline))
    bool is_process_start(record_t *r) {
        return r->MsgType == JSI_PROCESS_START;
    }

    static inline __attribute__((always_inline))
    bool is_process_exit(record_t *r) {
        return r->MsgType == JSI_PROCESS_EXIT;
    }

    static inline __attribute__((always_inline))
    bool is_event(record_t *r, int16_t event_id) {
        return r->MsgType == event_id;
    }

    // check if r_p contains r_c
    static inline __attribute__((always_inline))
    bool is_contain(record_t *r_p, record_t *r_c) {
        return  r_p!=NULL && r_c!=NULL && 
                r_p->timestamps.enter<=r_c->timestamps.enter && 
                r_p->timestamps.exit >=r_c->timestamps.exit;
    }

    static inline __attribute__((always_inline))
    bool is_valid_ctxt(record_t *r) {
        return r->MsgType!=JSI_PROCESS_START && 
               r->MsgType!=JSI_PROCESS_EXIT && 
               r->MsgType!=JSI_TOOL_IO_MSGTYPE &&
               r->MsgType!=JSI_ACCL_ACTIVITY_EVENT;
    }

    // ATTENTION: Use with pointer of accurate record types
    template<typename RecordType>
    static inline __attribute__((always_inline))
    uint64_t *counters(RecordType *r) {
        return reinterpret_cast<uint64_t *>(r + 1);
    }
    // Special care for common type
    static inline __attribute__((always_inline))
    uint64_t *counters(record_t* r) {
        return reinterpret_cast<uint64_t *>(reinterpret_cast<char*>(r) + get_record_size(r));
    }

    // Infer the size of record at runtime using the MsgType field
    static inline __attribute__((always_inline))
    uint64_t *counters_runtime_inferred(record_t *r) {
        return (uint64_t *) ((char *) r + get_record_size(r));
    }

    static inline __attribute__((always_inline))
    uint64_t counter_val_enter(record_t *r, int i, size_t num_counters) {
        return counters_runtime_inferred(r)[i];
    }

    static inline __attribute__((always_inline))
    uint64_t counter_val_exit(record_t *r, int i, size_t num_counters) {
        return counters_runtime_inferred(r)[i + num_counters];
    }

    static inline __attribute__((always_inline))
    uint64_t counter_val(record_t *r, int i, size_t num_counters, bool is_exit) {
        return counters_runtime_inferred(r)[i + num_counters * (is_exit ? 1 : 0)];
    }

    static inline __attribute__((always_inline))
    uint64_t counter_diff(record_t *r, int i, size_t num_counters) {
        auto *counter_base = counters_runtime_inferred(r);
        return counter_base[i + num_counters] - counter_base[i];
    }

    // TODO: list all message types with specific record type other than record_t
    // ...
    // Dump
    static inline __attribute__((always_inline))
    std::string dump_string_v0(record_t *r) {
        std::string desc;
        desc = std::string("Event Name: ") + get_record_name(r) + std::string("\n");
        desc += std::string("Start Time: ") + std::to_string(r->timestamps.enter) + std::string("\n");
        desc += std::string("End Time: ") + std::to_string(r->timestamps.exit) + std::string("\n");
        desc += std::string("Duration: ") + std::to_string(r->timestamps.exit - r->timestamps.enter) +
                std::string("\n");
        desc += std::string("Record Size: ") + std::to_string(get_record_size(r)) + std::string("\n");
        if (JSI_MSG_IS_ACCL_API(r->MsgType)) {
            record_activity_t* ra = (record_activity_t*)r;
            desc += std::string("Correlation ID: ") + std::to_string(ra->correlation_id) + std::string("\n");
            if(JSI_ACCL_EVENT_IS_LAUNCH(r->MsgType)) {
                record_activity_launch_t* rl = (record_activity_launch_t*)r;
                desc += "Thread Dim: <" + std::to_string(rl->blockNum.x) + "," + std::to_string(rl->blockNum.y) + "," + std::to_string(rl->blockNum.z) + ">\n";
                desc += "Block Dim: <" + std::to_string(rl->blockDim.x) + "," + std::to_string(rl->blockDim.y) + "," + std::to_string(rl->blockDim.z) + ">\n";
                desc += "Shared Memory Bytes: " + std::to_string(rl->sharedMemBytes) + "\n";
                desc += "Stream: " + std::to_string((uint64_t)rl->stream) + "\n";
            } else if (JSI_ACCL_EVENT_IS_MEMCPY(r->MsgType)) {
                record_activity_memcpy_t* rm = (record_activity_memcpy_t*)r;
                desc += "Dst: " + std::to_string((uint64_t)rm->dst) + "\n";
                desc += "Src: " + std::to_string((uint64_t)rm->src) + "\n";
                desc += "Size: " + std::to_string(rm->sizeBytes) + "\n";
                desc += "Kind: " + std::to_string(rm->kind) + "\n";
            } else if(JSI_ACCL_EVENT_IS_MEMCPY_ASYNC(r->MsgType)) {
                record_activity_memcpy_async_t* rm = (record_activity_memcpy_async_t*)r;
                desc += "Dst: " + std::to_string((uint64_t)rm->dst) + "\n";
                desc += "Src: " + std::to_string((uint64_t)rm->src) + "\n";
                desc += "Size: " + std::to_string(rm->sizeBytes) + "\n";
                desc += "Kind: " + std::to_string(rm->kind) + "\n";
                desc += "Stream: " + std::to_string((uint64_t)rm->stream) + "\n";
            }
        }
        switch (r->MsgType) {
            // TODO: query based on wrapper defines
            case event_MPI_Send:
            case event_MPI_Recv: {
                record_comm_t *rd = (record_comm_t *) r;
                desc += std::string("Data type: ") + std::to_string(rd->datatype) + std::string("\n");
                desc += std::string("Dest/Src: ") + std::to_string(rd->dest) + std::string("\n");
                desc += std::string("Type Size: ") + std::to_string(rd->typesize) + std::string("\n");
                desc += std::string("Tag: ") + std::to_string(rd->tag) + std::string("\n");
                desc += std::string("Count: ") + std::to_string(rd->count) + std::string("\n");
                desc += std::string("MPI Comm: ") + std::to_string(rd->comm) + std::string("\n");
                break;
            }
            case event_MPI_Isend:
            case event_MPI_Irecv: {
                record_comm_async_t *rd = (record_comm_async_t *) r;
                desc += std::string("Data type: ") + std::to_string(rd->datatype) + std::string("\n");
                desc += std::string("Dest/Src: ") + std::to_string(rd->dest) + std::string("\n");
                desc += std::string("Type Size: ") + std::to_string(rd->typesize) + std::string("\n");
                desc += std::string("Tag: ") + std::to_string(rd->tag) + std::string("\n");
                desc += std::string("Count: ") + std::to_string(rd->count) + std::string("\n");
                desc += std::string("MPI Comm: ") + std::to_string(rd->comm) + std::string("\n");
                desc += std::string("MPI Request: ") + std::to_string(rd->request) + std::string("\n");
                break;
            }
            case event_MPI_Wait: {
                record_comm_wait_t *rd = (record_comm_wait_t *) r;
                desc += std::string("MPI Request: ") + std::to_string(rd->request) + std::string("\n");
                break;
            }
            case event_MPI_Alltoall:
            case event_MPI_Alltoallv: {
                record_all2all_t *rd = (record_all2all_t *) r;
                desc += std::string("Data type: ") + std::to_string(rd->datatype) + std::string("\n");
                desc += std::string("Send count: ") + std::to_string(rd->sendcnt) + std::string("\n");
                desc += std::string("Recv count: ") + std::to_string(rd->recvcnt) + std::string("\n");
                desc += std::string("MPI Comm: ") + std::to_string(rd->comm) + std::string("\n");
                break;
            }
            case event_MPI_Allreduce: {
                record_allreduce_t *rd = (record_allreduce_t *) r;
                desc += std::string("Data type: ") + std::to_string(rd->datatype) + std::string("\n");
                desc += std::string("Count: ") + std::to_string(rd->count) + std::string("\n");
                desc += std::string("MPI Op: ") + std::to_string(rd->op) + std::string("\n");
                desc += std::string("MPI Comm: ") + std::to_string(rd->comm) + std::string("\n");
                break;
            }
            case event_MPI_Reduce: {
                record_reduce_t *rd = (record_reduce_t *) r;
                desc += std::string("Data type: ") + std::to_string(rd->datatype) + std::string("\n");
                desc += std::string("Count: ") + std::to_string(rd->count) + std::string("\n");
                desc += std::string("Root: ") + std::to_string(rd->root) + std::string("\n");
                desc += std::string("MPI Op: ") + std::to_string(rd->op) + std::string("\n");
                desc += std::string("MPI Comm: ") + std::to_string(rd->comm) + std::string("\n");
                break;
            }
            case event_MPI_Bcast: {
                record_bcast_t *rd = (record_bcast_t *) r;
                desc += std::string("Data type: ") + std::to_string(rd->datatype) + std::string("\n");
                desc += std::string("Count: ") + std::to_string(rd->count) + std::string("\n");
                desc += std::string("Root: ") + std::to_string(rd->root) + std::string("\n");
                desc += std::string("MPI Comm: ") + std::to_string(rd->comm) + std::string("\n");
                break;
            }
            case event_MPI_Barrier: {
                record_barrier_t *rd = (record_barrier_t *) r;
                desc += std::string("MPI Comm: ") + std::to_string(rd->comm) + std::string("\n");
                break;
            }
        }
        return desc;
    }

    static inline __attribute__((always_inline))
    std::string dump_string(record_t *r) {
        return record_utils::to_string(r);
    }

    static inline __attribute__((always_inline))
    std::string dump_string(record_t *r, jsi::toolkit::BacktraceTree *tree) {
        std::string desc = dump_string(r);
        if (is_valid_ctxt(r)) {
            desc += std::string("Backtrace: \n");
            if (tree) {
                #pragma omp critical
                {
                    desc += tree->backtrace_get_context_string((backtrace_context_t) r->ctxt);
                }
            }
        }
        return desc;
    }
};


class RecordTrace {
private:
    int _rank;
    int _start, _end;
    // int _size;
    int _cache_size;
    int _model;
    // the trace is ordered in time series
    // TODO: cachable record trace reading
    void *_trace_mmap;

    void *_trace_view_start;
    void *_trace_view_end;
    void *_global_trace_view_start;
    void *_global_trace_view_end;

    bool _is_global;

    std::vector<std::string> _pmu_event_list;

    void detect_size();

    std::unique_ptr<pse::ral::DirSectionInterface> _sec;


public:
    int _size;
    class RecordTraceIterator {
    private:
        record_t *_cur = nullptr;
        RecordTrace *_trace = nullptr;
        pse::ral::SectionExtractor extractor;
        std::vector<std::shared_ptr<pse::ral::DataSectionInterface>> secs;

    public:
        record_t *val() {
            if (_trace->_model == SECTION_MODEL) {
                return (record_t *) *extractor;
            }
            return _cur;
        }

        RecordTraceIterator next() {
//            JSI_INFO("RecordTraceIterator Enter next TraceModel:%d\n",_trace->_model);
            void *next = nullptr;
            if (_trace->_model == DATA_MODEL) {
//                JSI_INFO("RecordTraceIterator GetRecordSize:%d\n",RecordHelper::get_record_size(_cur, _trace->_pmu_event_list.size()));
                next = (uint8_t *) _cur
                       + RecordHelper::get_record_size(_cur, _trace->_pmu_event_list.size());
            } else if (_trace->_model == COMPRESS_MODEL) {
//                JSI_INFO("RecordTraceIterator GetRecordSize:%d\n",RecordHelper::get_compressed_record_size(_cur, _trace->_pmu_event_list.size()));
                next = (uint8_t *) _cur +
                       RecordHelper::get_compressed_record_size(_cur, _trace->_pmu_event_list.size());
            }
            else if (_trace->_model == SECTION_MODEL) {
                auto res = *this;
                ++res.extractor;
                return res;
            }
            return RecordTraceIterator(next, _trace);
        }

        explicit RecordTraceIterator(void *cur, RecordTrace *trace)
            : _cur{(record_t *) cur},
              _trace{trace},
              extractor({}) {
        }

        RecordTraceIterator(pse::ral::DirSectionInterface* sec, RecordTrace *trace)
            : _trace{trace}, extractor({}) {
            std::vector<std::pair<pse::ral::DataSectionInterface::Iterator, pse::ral::DataSectionInterface::Iterator>> iters;
            for (auto iter = sec->begin(); iter != sec->end(); ++iter) {
                if (iter.getDesc() >= StaticSectionDesc::RECORD_SEC_OFFSET)
                {
                    auto sec = iter.getDataSection();
                    if (sec)
                    {
                        iters.push_back({sec->begin(), sec->end()});
                        secs.push_back(std::move(sec));
                    }
                }
            }
            extractor = pse::ral::SectionExtractor(std::move(iters));
        }

        RecordTraceIterator(const RecordTraceIterator &prev)
            : _cur{prev._cur},
              _trace{prev._trace},
              extractor{prev.extractor},
              secs(prev.secs) {
        }

        bool operator==(RecordTraceIterator const &r) {
            if (is_invalid(*this) && is_invalid(r))
            {
                return true;
            }
            if ((is_invalid(*this) && !is_invalid(r)) || (is_invalid(r) && !is_invalid(*this)))
            {
                return false;
            }
            if (r._trace->_model != _trace->_model)
            {
                return false;
            }
            if (_trace->_model == SECTION_MODEL)
            {
                return extractor.get() == r.extractor.get();
            }
            else
            {
                return _cur == r._cur;
            }
        }

        bool operator!=(RecordTraceIterator const &r) {
            return !(*this == r); 
        }

        static inline bool is_invalid(RecordTraceIterator const &r) {
            if (r._trace == nullptr) 
            {
                return true;
            }
            if (r._trace->_model == SECTION_MODEL)
            {
                return r.extractor.invalid();
            }
            return r._cur == nullptr;
        }

        static RecordTraceIterator invalid() {
            return RecordTraceIterator((void*)nullptr, nullptr);
        }
    };
private:
    RecordTraceIterator zoom_begin = RecordTraceIterator::invalid();
    RecordTraceIterator zoom_end = RecordTraceIterator::invalid();

public:

    uint64_t offset;

    /* rfile: Trace File,  */
    RecordTrace(void *trace_mmap, int rank, int start, int end, int size, bool isGlobal,
                const std::vector<std::string> &pmu_event_list,int model, int cache_size = 4096);
    RecordTrace(std::unique_ptr<pse::ral::DirSectionInterface>&& section, int rank, bool isGlobal,
                const std::vector<std::string> &pmu_event_list);

    ~RecordTrace();

    inline int size() {
        if (_end == -1) {
            detect_size();
        }
        return _end - _start;
    }

    inline RecordTraceIterator begin() {
        if (_model == SECTION_MODEL) {
            if (RecordTraceIterator::is_invalid(zoom_begin))
            {
                return global_begin();
            }
            return zoom_begin;
        }
        else
        {
         return RecordTraceIterator(_trace_view_start, this);
        }
    }

    inline RecordTraceIterator end() {
        if (_model == SECTION_MODEL) {
            if (RecordTraceIterator::is_invalid(zoom_end))
            {
                return global_end();
            }
            return zoom_end;
        }
        return RecordTraceIterator(_trace_view_end, this);
    }

    inline RecordTraceIterator global_begin() {
        if (_model == SECTION_MODEL) {
            return RecordTraceIterator(_sec.get(), this);
        }
        return RecordTraceIterator(_global_trace_view_start, this);
    }

    inline RecordTraceIterator global_end() {
        if (_model == SECTION_MODEL)
        {
            return RecordTraceIterator((void*)nullptr, this);
        }
        return RecordTraceIterator(_global_trace_view_end, this);
    }

    /* return the first found record with given MsgType */
    inline RecordTraceIterator find(int MsgType, bool is_global) {
        RecordTraceIterator it = RecordTraceIterator::invalid();
        RecordTraceIterator ie = RecordTraceIterator::invalid();
        if (is_global) {
            it = global_begin();
            ie = global_end();
        } else {
            it = begin();
            ie = end();
        }
        while (it != ie) {
            if (it.val()->MsgType == MsgType) {
                return it;
            }
            it = it.next();
        }
        return RecordTraceIterator::invalid();
    }

    /* get i-th record */
    inline RecordTraceIterator get(int i) {
        ASSERT_MSG(i >= 0 && i < size(), "RecordReader::get Illegal arguments");
        RecordTraceIterator it = begin();
        while (i != 0) {
            it = it.next();
            --i;
        }
        return it;
    }

    inline size_t num_pmu_events() {
        return _pmu_event_list.size();
    }

    inline const std::string &get_pmu_event_name(int i) {
        return _pmu_event_list[i];
    }

    inline const std::vector<std::string> &pmu_event_list() {
        return _pmu_event_list;
    }

    /* filter the trace from timestamp ts_s to timestamp ts_e
     * modify: _start, _end
     * return: whether success or not
     */
    bool zoom(uint64_t ts_s, uint64_t ts_e, uint64_t offset);

    /* filter out all events with MsgType and return the filtered trace */
    // RecordTrace *filter(int MsgType);
};

class RecordTraceExtBase {
    /* Extended record reader: records are managed as sections as follows:
     * <trace header> <sec.size> <sec.data...> <sec.size> <sec.data...> ...
     */
    public:
        RecordTraceExtBase(const char* filename);
        JSI_TRACE_HEADER get_header();
        int loadSection(uint8_t** begin);
        void reset();
        std::string get_string(uint64_t key);
    private:
        uint8_t* _trace;
        JSI_TRACE_HEADER header;
        size_t cur;
        size_t _size;
        std::unordered_map<uint64_t, std::string> _string_map;
};

class RecordTraceExt {
    /*  */
    public:
        class Iterator {
            public:
                explicit Iterator(RecordTraceExt* rte, size_t step_size) :
                    _cur(0), _step_size(step_size), _rte(rte) {
                    _size = rte->loadSection(&_section);
                }
                void* get() { return reinterpret_cast<void*>(&_section[_cur]); }
                bool valid() {
                    return _size>0;
                }
                bool invalid() {
                    return _size==0;
                }
                void next() {
                    _cur += _step_size;
                    if (_cur >= _size) {
                        _size = _rte->loadSection(&_section);
                        _cur = 0;
                    }
                }
            private:
                uint8_t* _section;
                size_t _size;
                size_t _cur;
                size_t _step_size;
                RecordTraceExt* _rte;
        };
        RecordTraceExt(RecordTraceExtBase* rte) : rte(rte) {}
        ~RecordTraceExt() { delete rte; }
        std::string get_string(uint64_t key) { return rte->get_string(key); }
        virtual std::string dump() { return std::string("<Undefined dump handler>"); }
        virtual Iterator begin() { rte->reset(); return Iterator(this, 1); }
    protected:
        RecordTraceExtBase* rte;
        int loadSection(uint8_t** begin) { return rte->loadSection(begin); }
};
#ifdef ROCM
class RecordTraceExtAcclRocm : public RecordTraceExt {
    /* Extended record reader: records are managed as sections as follows:
     * <trace header> <sec.size> <sec.data...> <sec.size> <sec.data...> ...
     */
    public:
        RecordTraceExtAcclRocm(RecordTraceExtBase* rte) 
            : RecordTraceExt(rte) {}
        std::string dump();
        Iterator begin() { rte->reset(); return Iterator(this, sizeof(ext_record_accl)); }
};
#endif
using RecordTraceIterator = RecordTrace::RecordTraceIterator;
using RecordTraceExtIterator = RecordTraceExt::Iterator;
using jsi::toolkit::BacktraceTree;
/* Collection of MPI & Function Traces */
typedef std::unordered_map<int, RecordTrace *> RecordTraceCollection;
typedef std::unordered_map<int, RecordMeta *> RankMetaCollection;
typedef std::unordered_map<int, BacktraceTree *> BacktraceCollection;
typedef std::unordered_map<int, RecordTraceExt *> RecordTraceExtCollection;

class RecordReader {
protected:
    int _model;
    bool _mpi_only;
    RecordTraceCollection _record_collection;
    RankMetaCollection _meta_collection;
    BacktraceCollection _backtrace_collection;
    RecordTraceExtCollection _record_ext_collection;

    std::unordered_map<std::string, int> _id2rank{};
    std::unordered_map<int, std::vector<std::string>> _pmu_event_list_collection{};

    std::unordered_map<std::string, std::unique_ptr<pse::ral::BackendInterface>> _node2backends;

    void _load_trace(const char *fn, int rank);
    // void _load_trace(std::unique_ptr<pse::ral::DirSectionInterface>&& dir);

    void _load_backtrace(const char *fn, int rank, const char* dwarf_dir, bool enable_dbinfo);
    void _load_backtrace(std::unique_ptr<pse::ral::DirSectionInterface>& dir, int rank, const char* dwarf_dir, bool enable_dbinfo);

    RecordReader(int _model) : _model(_model) {}

public:
    RecordReader(const char *dir,int _model, const char* dwarf_dir = nullptr, bool enable_dbinfo = true, bool mpi_only = true, bool enable_backtrace = true);

    ~RecordReader();

    virtual void load(const char *dir, const char* dwarf_dir, bool enable_dbinfo, bool enable_backtrace = true);
    void section_load(const char *dir, const char* dwarf_dir, bool enable_dbinfo, bool enable_backtrace);

    void load_meta(const char* path);
    void load_meta(std::unique_ptr<pse::ral::DirSectionInterface>& dir, const std::string& node_id);

    void load_backtrace(const char* path, const char* dwarf_dir, bool enable_dbinfo);
    void load_backtrace(std::unique_ptr<pse::ral::DirSectionInterface>& dir, const std::string& node_id, const char* dwarf_dir, bool enable_dbinfo);
    void load_trace(const char* path);
    void load_trace(std::unique_ptr<pse::ral::DirSectionInterface>&& dir, const std::string& node_id);
    void load_etr(const char* path);


    RecordTrace &get_trace(int rank);

    RecordTraceCollection &get_all_traces();

    RecordMeta &get_meta_map(int rank);

    RankMetaCollection &get_all_meta_maps();

    BacktraceTree &get_backtrace(int rank);

    BacktraceCollection &get_all_backtraces();

    RecordTraceExtCollection &get_all_ext_traces();

    bool has_backtrace() const;

    int get_pmu_event_num_by_rank(int rank);
};

class ParallelRecordReaderUnordered : public RecordReader {
private:
    int _rank;
    int _size;
    std::vector<std::string> _id_list;
public:
    ParallelRecordReaderUnordered(const char *dir,int _model, const char* dwarf_dir = nullptr, bool enable_dbinfo = true)
         : RecordReader(_model) { load(dir, dwarf_dir, enable_dbinfo); }
    virtual void load(const char *dir, const char* dwarf_dir, bool enable_dbinfo);
};

#endif
