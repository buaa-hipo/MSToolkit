#ifndef __INSTRUMENT_H__
#define __INSTRUMENT_H__

#define WRAPPER_VERBOSE

#include "utils/tsc_timer.h"
#include "utils/jsi_log.h"
// #include "record/record_reader.h"
#include "record/record_type.h"
#include "record/record_writer.h"
#include "jsi_sampling.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

#ifdef ENABLE_PMU
#define ENABLE_WRITER
#include "instrument/pmu_collector.h"
#endif

#define DUMMY_ALLOCATE(...) (_rec)

bool jsi_inst_inited = false;
int rank = 0;
int size = -1;
bool jsi_ifsamp = false;
bool jsi_iftime = false;
bool jsi_ifinit = false;
bool jsi_dynscale = false;
ifSampFlag sampflag;

STG* stg = NULL;

__attribute__((always_inline)) inline
bool isSampled(uint16_t MsgType, backtrace_context_t *ctxt) {
    uint64_t key;
    switch(sampflag.samp_temp_mode) {
        case ifSampFlag::TemporalSampleMode::FUNCTION_NAME:
            key = MsgType;
            break;
        case ifSampFlag::TemporalSampleMode::CALL_SITE:
            key = backtrace_get_callsite();
            break;
        case ifSampFlag::TemporalSampleMode::CALL_PATH:
            *ctxt = backtrace_context_get();
            key = (uint64_t)(*ctxt);
            break;
        default:
            JSI_ERROR("#### isSampled: Should not reach here ####");
    }
    STG_Val_t* stg_val = stg->transfer_to(key);
    if(stg_val->count % sampflag.window_disable < sampflag.window_enable) {
        return true;
    }
    return false;
}


void time_handler(int signo)
{
    printf("******* Timer Triggerred\n");
    // Disable All
    sampflag.samp_iftime = 1;
    jsi_iftime =1;
    jsi_ifsamp = 0;
    sampflag.samp_ifSamp = 0;
}

void time_counter(int onTime){
    struct itimerval it;
    //struct itimerval oldit;
    signal(SIGALRM,time_handler);
    //std::cout << "ontime value is " << onTime << "\n";
    it.it_value.tv_sec = onTime;
    it.it_value.tv_usec = 0;
    // disable the timer when timer is triggered
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;

    if(setitimer(ITIMER_REAL,&it,NULL) == -1)
    {
        perror("setitimer error ");
        exit(1);
    }
    printf("******* Timer Enabled with: %d second\n", onTime);
    //sleep(-1);
    printf("******* Exit time_counter\n");
}

#define MPI_WRAPPER_INIT_PRIORITY (RECORD_INIT_PRIORITY+100)
#define MPI_WRAPPER_FINI_PRIORITY (RECORD_FINI_PRIORITY+100)

/* TODO: For better mapping, we can use process id and record 
   corresponding pid with mpi rank information as meta data 
   for post-analysis. */
// FIXME: We currently use a temperal global record to flush it into
// the record buffer after MPI_Init instrumentation.

__attribute__((constructor (MPI_WRAPPER_INIT_PRIORITY)))
void jsi_mpi_tracer_init(){
    JSI_LOG(JSILOG_INFO, "Initialize JSI MPI Wrapper Library.\n");
    // FIX20240125: PROCESS_ENTER record move to record_writer constructor
}

__attribute__((destructor (MPI_WRAPPER_FINI_PRIORITY)))
void jsi_mpi_tracer_finalize(){
    JSI_LOG(JSILOG_INFO, "[Rank %d] Finalize JSI MPI Wrapper Library.\n", rank);
    // FIX20240125: PROCESS_EXIT record move to record_writer destructor
}

inline __attribute__((always_inline))
void jsi_inst_init(record_t* _rec, uint16_t MsgType, uint64_t start) {
    uint64_t init_start = get_tsc_raw();
    if(!jsi_inst_inited) {
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        // RecordWriter::init(rank);
        RecordWriter::metaSectionStart("MPI_COMM_WORLD");
        uint64_t save_comm_world = (uint64_t)MPI_COMM_WORLD;
        RecordWriter::metaStore<int64_t>("MPI_COMM_WORLD", save_comm_world);
        RecordWriter::metaStore<int>("rank", rank);
        RecordWriter::metaStore<int>("size", size);
        RecordWriter::metaSectionEnd("MPI_COMM_WORLD");
        jsi_inst_inited = true;
#ifdef WRAPPER_VERBOSE
        if (rank==0) {
            JSI_INFO("JSI MPI wrapper library loaded\n");
        }
#endif
    } else {
        JSI_WARN("JSI MPI already initialized.\n");
    }

    // sampling class
    sampflag.init(size, rank);
    jsi_ifsamp = sampflag.samp_ifSamp;
    jsi_ifinit = sampflag.samp_extendFlag;
    if (sampflag.getSampTime()){
        time_counter(sampflag.getSampTime());
    }
    jsi_dynscale = sampflag.getDynscale();

    // temporal sampling
    if (sampflag.samp_temp) {
        stg = createOrGetSTG();
    }

    record_t* rec = (record_t*) DUMMY_ALLOCATE(
        sizeof(record_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].MsgType = MsgType;
    rec[0].timestamps.enter = start;
    rec[0].timestamps.exit = init_start;

#ifdef ENABLE_BACKTRACE
    if(jsi_backtrace_enabled) {
        rec[0].ctxt = backtrace_context_get();
    }
#endif

#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        // TODO: Now this is just a workaround to make sure the values are valid
        pmu_collector_get_all(counters);
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(record_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
void jsi_inst_finalize() {
    // FIX20240125: backtrace and pmu finalization move to lib destructor.
    if(sampflag.print_stg) {
        JSI_INFO("STG dumped to %s\n", sampflag.stg_fn.c_str());
        stg->fprint(sampflag.stg_fn.c_str());
    } 
}

inline __attribute__((always_inline))
record_t* jsi_inst_enter(record_t* _rec, uint16_t MsgType) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }
    
    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }

    if (!jsi_ifsamp) {
        return NULL;
    }
    record_t* rec = (record_t*) DUMMY_ALLOCATE(
        sizeof(record_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].MsgType = MsgType;
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].ctxt = ctxt;
        } else {
            rec[0].ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    rec[0].timestamps.enter = get_tsc_raw();
    return rec;
}

inline __attribute__((always_inline))
record_comm_t* jsi_inst_enter_comm(record_comm_t* _rec, uint16_t MsgType, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    // e2e communication, if the dest was in samplist, add this rank into samplist
    if (!jsi_ifsamp)
    {
        // std::cout << "rank " << rank << "jsi_ifsamp " << jsi_ifsamp << "\n";
        // std::cout << sampflag.sampList[0] << "\n";
        // todo recv or dest?
        sampflag.dynSamp(dest);
        jsi_ifsamp = sampflag.samp_ifSamp;
        //jsi_iftime = sampflag.samp_extendFlag;
    }
    if (!jsi_ifsamp)
    {
        return NULL;
    }

    record_comm_t* rec = (record_comm_t*) DUMMY_ALLOCATE(
        sizeof(record_comm_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].count = count;
    rec[0].datatype = (uint64_t) (datatype);
    int typesize;
    PMPI_Type_size(datatype, &typesize);
    rec[0].typesize = typesize;
    rec[0].dest = dest;
    rec[0].tag = tag;
    rec[0].comm = (uint64_t) (comm);
    rec[0].record.MsgType = MsgType;
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    rec[0].record.timestamps.enter = get_tsc_raw();
    return rec;
}

inline __attribute__((always_inline))
record_comm_rank_t* jsi_inst_enter_comm_rank(record_comm_rank_t* _rec, uint16_t MsgType, MPI_Comm comm) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    if (!jsi_ifsamp)
    {
        return NULL;
    }

    record_comm_rank_t* rec = (record_comm_rank_t*) DUMMY_ALLOCATE(
        sizeof(record_comm_rank_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].comm = (uint64_t)(comm);
    rec[0].record.MsgType = MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_inst_exit_comm_rank(record_comm_rank_t* rec, int* rank) {
    // samp_ontime mode identify
    if (rec==NULL) {
	    return ;
    }

    if (!jsi_ifsamp)
    {
        return ;
    }

    record_comm_rank_t& r = static_cast<record_comm_rank_t*>(rec)[0];
    r.record.timestamps.exit = get_tsc_raw();
    r.rank = *rank;
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(reinterpret_cast<record_comm_rank_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(record_comm_rank_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
record_comm_dup_t* jsi_inst_enter_comm_dup(record_comm_dup_t* _rec, uint16_t MsgType, MPI_Comm comm) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    if (!jsi_ifsamp)
    {
        return NULL;
    }

    record_comm_dup_t* rec = (record_comm_dup_t*) DUMMY_ALLOCATE(
        sizeof(record_comm_dup_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].comm = (uint64_t)(comm);
    rec[0].record.MsgType = MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_inst_exit_comm_dup(record_comm_dup_t* rec, MPI_Comm* new_comm) {
    
    // samp_ontime mode identify
    if (rec==NULL) {
	    return ;
    }

    if (!jsi_ifsamp)
    {
        return ;
    }

    record_comm_dup_t& r = static_cast<record_comm_dup_t*>(rec)[0];
    r.record.timestamps.exit = get_tsc_raw();
    r.new_comm = (uint64_t)*new_comm;
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t *counters = RecordWriterHelper::counters(reinterpret_cast<record_comm_dup_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(record_comm_dup_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
record_comm_split_t* jsi_inst_enter_comm_split(record_comm_split_t* _rec, uint16_t MsgType, MPI_Comm comm, int color) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    if (!jsi_ifsamp)
    {
        return NULL;
    }

    record_comm_split_t* rec = (record_comm_split_t*) DUMMY_ALLOCATE(
        sizeof(record_comm_split_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].comm = (uint64_t)(comm);
    rec[0].color = color;
    rec[0].record.MsgType = MsgType;
    rec[0].record.timestamps.enter = get_tsc_raw();
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    return rec;
}

inline __attribute__((always_inline))
void jsi_inst_exit_comm_split(record_comm_split_t* rec, MPI_Comm* new_comm) {
    // samp_ontime mode identify
    if (rec==NULL) {
	    return;
    }

    if (!jsi_ifsamp)
    {
        return ;
    }

    record_comm_split_t& r = static_cast<record_comm_split_t*>(rec)[0];
    r.record.timestamps.exit = get_tsc_raw();
    r.new_comm = *(uint64_t*)new_comm;

    int sub_rank = 0;
    int sub_size = -1;
    PMPI_Comm_rank(*new_comm, &sub_rank);
    PMPI_Comm_size(*new_comm, &sub_size);
    uint64_t save_sub_comm = (uint64_t)new_comm;
    RecordWriter::metaSectionStart(("SUB COMM " + std::to_string(save_sub_comm)).c_str());
    RecordWriter::metaStore<int64_t>("sub_comm", save_sub_comm);
    RecordWriter::metaStore<int>("rank", sub_rank);
    RecordWriter::metaStore<int>("size", sub_size);
    RecordWriter::metaSectionEnd(("SUB COMM " + std::to_string(save_sub_comm)).c_str());

#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t *counters = RecordWriterHelper::counters(reinterpret_cast<record_comm_split_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(record_comm_split_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

inline __attribute__((always_inline))
record_comm_async_t* jsi_inst_enter_comm_async(record_comm_async_t* _rec, uint16_t MsgType, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    // e2e communication, if the dest was in samplist, add this rank into samplist
    if (!jsi_ifsamp)
    {
        // std::cout << "rank " << rank << "jsi_ifsamp " << jsi_ifsamp << "\n";
        // std::cout << sampflag.sampList[0] << "\n";
        // todo recv or dest?
        sampflag.dynSamp(dest);
        jsi_ifsamp = sampflag.samp_ifSamp;
        //jsi_iftime = sampflag.samp_extendFlag;
    }
    if (!jsi_ifsamp)
    {
        return NULL;
    }

    record_comm_async_t* rec = (record_comm_async_t*) DUMMY_ALLOCATE(
        sizeof(record_comm_async_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].count = count;
    rec[0].datatype = (uint64_t) (datatype);
    int typesize;
    PMPI_Type_size(datatype, &typesize);
    rec[0].typesize = typesize;
    rec[0].dest = dest;
    rec[0].tag = tag;
    rec[0].comm = (uint64_t) (comm);
    rec[0].record.MsgType = MsgType;
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t *counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    rec[0].record.timestamps.enter = get_tsc_raw();
    return rec;
}

inline __attribute__((always_inline))
void jsi_inst_exit_comm_async(record_comm_async_t* rec, MPI_Request *request) {
    // samp_ontime mode identify
    if(rec==NULL) { return ; }
    if (jsi_ifsamp) {
        jsi_ifsamp = (jsi_ifinit || jsi_dynscale);
        sampflag.samp_ifSamp = jsi_ifsamp;
    }
    reinterpret_cast<record_t*>(rec)[0].timestamps.exit = get_tsc_raw();
    reinterpret_cast<record_comm_async_t*>(rec)[0].request = (uint64_t) (*request);
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t *counters = RecordWriterHelper::counters(reinterpret_cast<record_comm_async_t*>(rec));
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(record_comm_async_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

// only record value of (*request), which can be paired by post-momentum analysis
inline __attribute__((always_inline))
record_comm_wait_t* jsi_inst_enter_wait(record_comm_wait_t* _rec, uint16_t MsgType, MPI_Request *request) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    // same with sampflag
    if (!jsi_ifsamp)
    {
        return NULL;
    }

    record_comm_wait_t* rec = (record_comm_wait_t*) DUMMY_ALLOCATE(
        sizeof(record_comm_wait_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].request = (uint64_t) (*request);
    rec[0].record.MsgType = MsgType;
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    rec[0].record.timestamps.enter = get_tsc_raw();
    return rec;
}

inline __attribute__((always_inline))
record_all2all_t* jsi_inst_enter_alltoall(record_all2all_t* _rec, uint16_t MsgType, int sendcnt, MPI_Datatype datatype, int recvcnt, MPI_Comm comm) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    // same with sampflag
    if (!jsi_ifsamp)
    {
        return NULL;
    }

    record_all2all_t* rec = (record_all2all_t*) DUMMY_ALLOCATE(
        sizeof(record_all2all_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].datatype = (uint64_t) (datatype);
    int typesize;
    PMPI_Type_size(datatype, &typesize);
    rec[0].typesize = typesize;
    rec[0].sendcnt = sendcnt;
    rec[0].recvcnt = recvcnt;
    rec[0].comm = (uint64_t) (comm);
    rec[0].record.MsgType = MsgType;
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    rec[0].record.timestamps.enter = get_tsc_raw();
    return rec;
}

inline __attribute__((always_inline))
record_all2all_t* jsi_inst_enter_alltoallv(record_all2all_t* _rec, uint16_t MsgType, const int *sendcounts, MPI_Datatype datatype, const int* recvcounts, MPI_Comm comm) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    // same with sampflag
    if (!jsi_ifsamp)
    {
        return NULL;
    }

    record_all2all_t* rec = (record_all2all_t*) DUMMY_ALLOCATE(
        sizeof(record_all2all_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    int procNum;
    PMPI_Comm_size(comm,&procNum);
    int recvcount = 0;
    int sendcount = 0;

	  for (int i=0;i<procNum;i++)
	  {
		    recvcount = recvcount + recvcounts[i];
		    sendcount = sendcount + sendcounts[i];
	  }
    rec[0].datatype = (uint64_t) (datatype);
    int typesize;
    PMPI_Type_size(datatype, &typesize);
    rec[0].typesize = typesize;
    rec[0].sendcnt = sendcount;
    rec[0].recvcnt = recvcount;
    rec[0].comm = (uint64_t) (comm);
    rec[0].record.MsgType = MsgType;
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    rec[0].record.timestamps.enter = get_tsc_raw();
    return rec;
}

inline __attribute__((always_inline))
record_allreduce_t* jsi_inst_enter_allreduce(record_allreduce_t* _rec, uint16_t MsgType, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    // same with sampflag
    if (!jsi_ifsamp)
    {
        return NULL;
    }
    record_allreduce_t* rec = (record_allreduce_t*) DUMMY_ALLOCATE(
        sizeof(record_allreduce_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].datatype = (uint64_t) (datatype);
    int typesize;
    PMPI_Type_size(datatype, &typesize);
    rec[0].typesize = typesize;
    rec[0].count = count;
    rec[0].op = (uint64_t) (op);
    rec[0].comm = (uint64_t) (comm);
    rec[0].record.MsgType = MsgType;
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    rec[0].record.timestamps.enter = get_tsc_raw();
    return rec;
}
// todo
inline __attribute__((always_inline))
record_reduce_t* jsi_inst_enter_reduce(record_reduce_t* _rec, uint16_t MsgType, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    // if not the root rank, same with sampflag
    // else extended into samplist
    if ( root == rank )
    {
        sampflag.samp_ifSamp = true;
        jsi_ifsamp = true;
    }
    if (!jsi_ifsamp)
    {
        return NULL;
    }
    record_reduce_t* rec = (record_reduce_t*) DUMMY_ALLOCATE(
        sizeof(record_reduce_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].datatype = (uint64_t) (datatype);
    int typesize;
    PMPI_Type_size(datatype, &typesize);
    rec[0].typesize = typesize;
    rec[0].count = count;
    rec[0].op = (uint64_t) (op);
    rec[0].root = root;
    rec[0].comm = (uint64_t) (comm);
    rec[0].record.MsgType = MsgType;
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    rec[0].record.timestamps.enter = get_tsc_raw();
    return rec;
}
//
inline __attribute__((always_inline))
record_bcast_t* jsi_inst_enter_bcast(record_bcast_t* _rec, uint16_t MsgType, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    // if not the root rank, same with sampflag
    // else extended into samplist
    if ( root == rank )
    {
        sampflag.samp_ifSamp = true;
        jsi_ifsamp = true;
    }
    if (!jsi_ifsamp)
    {
        return NULL;
    }
    record_bcast_t* rec = (record_bcast_t*) DUMMY_ALLOCATE(
        sizeof(record_bcast_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].datatype = (uint64_t) (datatype);
    int typesize;
    PMPI_Type_size(datatype, &typesize);
    rec[0].typesize = typesize;
    rec[0].count = count;
    rec[0].root = root;
    rec[0].comm = (uint64_t) (comm);
    rec[0].record.MsgType = MsgType;
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    rec[0].record.timestamps.enter = get_tsc_raw();
    return rec;
}

inline __attribute__((always_inline))
record_barrier_t* jsi_inst_enter_barrier(record_barrier_t* _rec, uint16_t MsgType, MPI_Comm comm) {
    // when CALL_PATH mode, the backtrace will be obtained in advance for STG construction
    // to avoid redundant backtrace collection, we will cache the backtrace context if needed
    backtrace_context_t ctxt=0;
    if (sampflag.samp_temp && !isSampled(MsgType, &ctxt)) {
        return NULL;
    }

    // samp_ontime mode identify
    if (jsi_iftime)
    {
        return NULL;
    }
    if (!jsi_ifsamp)
    {
        return NULL;
    }
    record_barrier_t* rec = (record_barrier_t*) DUMMY_ALLOCATE(
        sizeof(record_barrier_t) + sizeof(uint64_t) * 2 * jsi_pmu_num);
    rec[0].comm = (uint64_t) (comm);
    rec[0].record.MsgType = MsgType;
#ifdef ENABLE_BACKTRACE
    if (jsi_backtrace_enabled) {
        if(sampflag.samp_temp && sampflag.samp_temp_mode==ifSampFlag::TemporalSampleMode::CALL_PATH) {
            rec[0].record.ctxt = ctxt;
        } else {
            rec[0].record.ctxt = backtrace_context_get();
        }
    }
#endif
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters);
    }
#endif
    rec[0].record.timestamps.enter = get_tsc_raw();
    return rec;
}

template<typename RecordType>
inline void jsi_inst_exit(RecordType* rec) {
    // samp_ontime mode identify
    if(rec==NULL) { return ; }
    if (jsi_ifsamp) {
        jsi_ifsamp = (jsi_ifinit || jsi_dynscale);
        sampflag.samp_ifSamp = jsi_ifsamp;
    }
    reinterpret_cast<record_t*>(rec)->timestamps.exit = get_tsc_raw();
#ifdef ENABLE_PMU
    if (jsi_pmu_enabled) {
        uint64_t* counters = RecordWriterHelper::counters(rec);
        pmu_collector_get_all(counters + jsi_pmu_num);
    }
#endif
    RecordWriter::traceStore((record_t*)rec);
    DEALLOCATE(rec, sizeof(RecordType) + sizeof(uint64_t) * 2 * jsi_pmu_num);
}

#endif
