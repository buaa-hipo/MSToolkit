#include "variance.h"

#include <cmath>
#include <string>

#include "utils/tsc_timer.h"
using namespace std;

#include <stdint.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <set>
#include <algorithm>
#include <omp.h>

#include <unistd.h>

#include <sstream>
#include <list>

#include <atomic>

#define VAR_CLUSTER_RATIO (1.05)
#define INS_COUNT_THRESHOLD (1000)
#define COMM_COUNT_THRESHOLD (0)
#define MEMCPY_COUNT_THRESHOLD (0)
#define LOW_CNT_THRESHOLD (5)

// smooth resolution: us
#define SMOOTH_RES (10000)
#define USE_PC_ENCODING

namespace jsi {

namespace variance {

#ifdef USE_PC_ENCODING
uint64_t encode(record_t* rec, BacktraceTree* bt) {
    // printf("Encoding %lu %p: %s\n", rec->ctxt, bt, RecordHelper::dump_string(rec).c_str());
    backtrace_context_t ctxt = rec->ctxt;
    /* currently, by forcely inlining the enter/exit instrumentation, 
     * we can assume that the leaf node is not the inner function of tracer
     */
    // std::string bt_str(bt->backtrace_get_context_string(ctxt));
    // while (bt_str.find("libmpi_wrapper.so") != std::string::npos) {
    //     ctxt = bt->get_parent(ctxt);
    //     if(ctxt!=BACKTRACE_UNKNOWN_NODE) {
    //         bt_str = std::string(bt->backtrace_get_context_string(ctxt));
    //     } else {
    //         JSI_ERROR("May be corrupted backtrace!\n");
    //     }
    // }
    backtrace_context_t pctxt = bt->get_parent(ctxt);
    if(pctxt!=BACKTRACE_UNKNOWN_NODE) {
        return reinterpret_cast<uint64_t>(bt->backtrace_context_ip(ctxt)) ^ reinterpret_cast<uint64_t>(bt->backtrace_context_ip(pctxt));
    }
    return reinterpret_cast<uint64_t>(bt->backtrace_context_ip(ctxt));
}
#else
uint64_t encode(record_t* rec, BacktraceTree* bt) {
    // printf("Encoding %lu %p: %s\n", rec->ctxt, bt, RecordHelper::dump_string(rec).c_str());
    backtrace_context_t ctxt = rec->ctxt;
    /* currently, by forcely inlining the enter/exit instrumentation, 
     * we can assume that the leaf node is not the inner function of tracer
     */
    // std::string bt_str(bt->backtrace_get_context_string(ctxt));
    // while (bt_str.find("libmpi_wrapper.so") != std::string::npos) {
    //     ctxt = bt->get_parent(ctxt);
    //     if(ctxt!=BACKTRACE_UNKNOWN_NODE) {
    //         bt_str = std::string(bt->backtrace_get_context_string(ctxt));
    //     } else {
    //         JSI_ERROR("May be corrupted backtrace!\n");
    //     }
    // } 
    return ctxt;
}
#endif

string encode_comm(int src, record_comm_t* r) {
    int count = r->count;
    if (count < COMM_COUNT_THRESHOLD) {
        // Too few comm for stablity detection, ignore
        return "";
    }
    //return to_string(count)+string("/")+to_string(src)+string("/")+to_string(r->dest)+string("/")+to_string(r->tag);
    return string("C")+to_string(count)+string("/")+to_string(src)+string("/")+to_string(r->dest)+string("/")+to_string(r->tag)+string("/")+to_string(r->comm);
}

// 10,000 nanoseconds (10us)
#define OVERLAP_WAIT_THREASHOLD 10000
struct _var_record_async_comm {
    record_t rec;
    uint64_t count;
    string signature;
};

string encode_comm(_var_record_async_comm* r) {
    //printf("ecnode_comm: <%s> enter=%lu, exit=%lu, count=%lu\n", RecordHelper::get_record_name((record_t*)r).c_str(), r->rec.timestamps.enter, r->rec.timestamps.exit, r->count);
    int count = r->count;
    if (count < COMM_COUNT_THRESHOLD) {
        // Too few comm for stablity detection, ignore
        return "";
    }
    //return string("CW")+to_string(count);
    return string("CW") + r->signature;
}

string encode_memcpy(record_activity_memcpy_t* r) {
    size_t count = r->sizeBytes;
    if (count < MEMCPY_COUNT_THRESHOLD) {
        return "";
    }
    return string("M")+to_string(count) + "/" + to_string(r->kind);
}

string encode_launch_hip(record_activity_launch_t* r) {
    // Only consider thread counts
    return string("A")+to_string(r->blockNum.x) + "_" + to_string(r->blockNum.y) + "_" + to_string(r->blockNum.z) + "/"
            + to_string(r->blockDim.x) + "_" + to_string(r->blockDim.y) + "_" + to_string(r->blockDim.z);
}

string encode_launch_mt(mt_record_kernel_launch_t* r) {
    return string("A")+to_string(r->cluster_id) + "_" + to_string(r->thread_num) + "_" + to_string(r->group_id) + "_" + to_string(r->thread_mask) + "_" + to_string(r->scalar_args_num) + "_" + to_string(r->ptr_args_num);
}

string encode_launch(record_t* r) {
    if(RecordHelper::is_hip_kernel_launch(r)) {
        return encode_launch_hip((record_activity_launch_t*)r);
    }
    if(RecordHelper::is_mt_kernel_launch(r)) {
        return encode_launch_mt((mt_record_kernel_launch_t*)r);
    }
    JSI_ERROR("UNKNOWN LAUNCH RECORD\n"); 
}

typedef uint64_t STG_Key_t; /* current ip OR backtrace context*/
// per rank state tranfer graph for online loop detection & counting
struct STG_NVal_t {
    // record counts for temporal sampling, count from 0
    std::list<VarianceMap::CommVarianceRecord*> rlist;
};

struct STG_EVal_t {
    std::list<VarianceMap::CalcVarianceRecord*> rlist;
};

typedef std::pair<STG_Key_t/*src*/, STG_Key_t/*dst*/> STG_Edge_t;

struct edge_hash {
    std::size_t operator() (const STG_Edge_t &edge) const
    {
        return std::hash<STG_Key_t>()(edge.first) ^ std::hash<STG_Key_t>()(edge.second);
    }
};

class STG {
  public:
    bool enable_async_comm;
    void reset() { last=0; last_r=std::make_pair(0,std::make_pair(0,0)); last_r_cached=false; enable_async_comm=true; }
    void clear() { 
        reset();
        nodes.clear();
        edges.clear();
    }
    STG() { 
        reset();
        overlap_wait_threshold=OVERLAP_WAIT_THREASHOLD*get_tsc_freq_mhz()/1000;
    }
    ~STG() { clear(); }
    STG_Key_t transfer_to(int rank,
		                  record_t* r, 
                          BacktraceTree* bt, 
                          int tot_ins_counter_idx, 
                          int opt_counter_idx,
                          int num_events, 
                          STG_Edge_t& e, 
                          VarianceMap::VarianceData* vdata) {
        std::vector<VarianceMap::CommVarianceRecord*>& commRecs = vdata->comm_vars;
        std::vector<VarianceMap::CalcVarianceRecord*>& calcRecs = vdata->calc_vars;
        STG_Key_t key = encode(r, bt);
        STG_NVal_t& nval = nodes[key];
        if( RecordHelper::is_event(r, event_MPI_Send) ||
            RecordHelper::is_event(r, event_MPI_Recv)) {
                VarianceMap::CommVarianceRecord* cr = new VarianceMap::CommVarianceRecord({-1, rank, r});
                nval.rlist.push_back(cr);
                commRecs.push_back(cr);
        } else if(RecordHelper::is_hip_kernel_launch(r) || RecordHelper::is_mt_kernel_launch(r)) {
            // printf("## LAUNCH event ##\n");
            VarianceMap::AcclVarianceRecord* cr = new VarianceMap::AcclVarianceRecord({-1, rank, r});
            nval.rlist.push_back((VarianceMap::CommVarianceRecord*)cr);
            vdata->accl_calc_vars.push_back(cr);
        } else if(RecordHelper::is_hip_memcpy(r) || RecordHelper::is_hip_memcpy_async(r)) {
            // printf("## MEMCPY event ##\n");
            VarianceMap::AcclVarianceRecord* cr = new VarianceMap::AcclVarianceRecord({-1, rank, r});
            nval.rlist.push_back((VarianceMap::CommVarianceRecord*)cr);
            vdata->accl_memcpy_vars.push_back(cr);
        } else if(enable_async_comm) {
            if ( RecordHelper::is_event(r, event_MPI_Irecv) ||
                  RecordHelper::is_event(r, event_MPI_Isend)) {
                record_comm_async_t* rec = reinterpret_cast<record_comm_async_t*>(r);
                // when multiple isend/irecv uses the same request handler, preserve the last one
                waiting[rec->request] = rec;
                //printf("Pushing into waiting list: %s\n", RecordHelper::dump_string(r).c_str());
            }
            else if ( RecordHelper::is_event(r, event_MPI_Wait)) {
                record_comm_wait_t* rec = reinterpret_cast<record_comm_wait_t*>(r);
                auto it = waiting.find(rec->request);
                if (it!=waiting.end()) {
                    uint64_t dur = r->timestamps.exit-r->timestamps.enter;
                    if (dur > overlap_wait_threshold) {
                        _var_record_async_comm* ra = new _var_record_async_comm;
                        ra->rec.MsgType = event_MPI_Wait;
                        ra->rec.timestamps.enter = it->second->record.timestamps.enter;
                        ra->rec.timestamps.exit = r->timestamps.exit;
                        ra->count = it->second->count;
                        ra->signature = encode_comm(rank, (record_comm_t*)it->second);
                        VarianceMap::CommVarianceRecord* cr = new VarianceMap::CommVarianceRecord({-1, rank, (record_t*)ra});
                        nval.rlist.push_back(cr);
                        commRecs.push_back(cr);
                    }
                    waiting.erase(it);
                }
            }
            else if ( RecordHelper::is_event(r, event_MPI_Waitall) ) {
                uint64_t dur = r->timestamps.exit-r->timestamps.enter;
                if (dur > overlap_wait_threshold) {
                    // estimate all waiting communications are processes in this MPI call
                    uint64_t start = r->timestamps.enter;
                    uint64_t count = 0;
                    string sig_all;
                    for(auto it=waiting.begin(); it!=waiting.end(); ++it) {
                        record_comm_async_t* rc = it->second;
                        if (vdata->offset > rc->record.timestamps.enter) {
                            printf("Record in waiting list (count=%lu, start=%lu): %s\n", start, count, RecordHelper::dump_string((record_t*)rc).c_str());
                        }
                        uint64_t cbeg = rc->record.timestamps.enter;
                        if (cbeg<start) {
                            start = cbeg;
                        }
                        count += rc->count;
                        sig_all += encode_comm(rank, (record_comm_t*)rc);
                    }
                    _var_record_async_comm* ra = new _var_record_async_comm;
                    ra->rec.MsgType = event_MPI_Waitall;
                    ra->rec.timestamps.enter = start;
                    ra->rec.timestamps.exit = r->timestamps.exit;
                    ra->count = count;
                    ra->signature = sig_all;
                    if (vdata->offset > start) {
                        printf("## Rank=%d ## <%s> enter=%lu, exit=%lu, count=%lu\n", rank, RecordHelper::get_record_name((record_t*)ra).c_str(),ra->rec.timestamps.enter, ra->rec.timestamps.exit, ra->count);
                        JSI_ERROR("DEBUGGING\n");
                    }
                    VarianceMap::CommVarianceRecord* cr = new VarianceMap::CommVarianceRecord({-1, rank, (record_t*)ra});
                    nval.rlist.push_back(cr);
                    commRecs.push_back(cr);
                }
                waiting.clear();
            }
        }
        
        if (last_r_cached) {
            e = std::make_pair(last, key);
            STG_EVal_t& eval = edges[e];
            // uint64_t tot_ins = RecordHelper::counter_diff(r, tot_ins_counter_idx, num_events);
            uint64_t* c_cnt = RecordHelper::counters_runtime_inferred(r);
            uint64_t count = c_cnt[tot_ins_counter_idx] - last_r.second.first;
            uint64_t count_opt_2 = 0;
            if (opt_counter_idx>=0) {
                count_opt_2 = c_cnt[tot_ins_counter_idx] - last_r.second.second;
            }
            VarianceMap::CalcVarianceRecord* cr2 = new VarianceMap::CalcVarianceRecord({-1, rank, last_r.first, r->timestamps.enter, count, count_opt_2});
            eval.rlist.push_back(cr2);
            calcRecs.push_back(cr2);
        }
        last = key;
        uint64_t* cnt = RecordHelper::counters_runtime_inferred(r) + num_events;
        if (opt_counter_idx>=0) {
            last_r = std::make_pair(r->timestamps.exit, std::make_pair(cnt[tot_ins_counter_idx], cnt[opt_counter_idx]));
        } else {
            last_r = std::make_pair(r->timestamps.exit, std::make_pair(cnt[tot_ins_counter_idx], 0));
        }
        last_r_cached = true;
        return key;
    }
    void merge(STG& stg) {
#ifdef DEBUG_TIMER
	printf("[%d] MERGING: #nodes=%lu, #edges=%lu\n", omp_get_thread_num(), stg.nodes.size(), stg.edges.size());
	double ts, te;
	ts = omp_get_wtime();
#endif
        for(auto it=stg.nodes.begin(), ie=stg.nodes.end(); it!=ie; ++it) {
            STG_Key_t key = it->first;
            STG_NVal_t& val = it->second;
            auto ni = nodes.find(key);
            if (ni==nodes.end()) {
                nodes[key] = val;
            // printf("[%d] key=%lu not found, adding %p\n", omp_get_thread_num(), key, this);
            } else {
                ni->second.rlist.splice(ni->second.rlist.begin(), val.rlist);
            }
        }
#ifdef DEBUG_TIMER
	te = omp_get_wtime();
	printf("[%d] Node merging time: %lf\n", omp_get_thread_num(), te-ts);
    ts = omp_get_wtime();
#endif
        for(auto it=stg.edges.begin(), ie=stg.edges.end(); it!=ie; ++it) {
            STG_Edge_t key = it->first;
            STG_EVal_t& val = it->second;
            auto ei = edges.find(key);
            if (ei==edges.end()) {
                edges[key] = val;
            } else {
                ei->second.rlist.splice(ei->second.rlist.begin(), val.rlist);
            }
        }
#ifdef DEBUG_TIMER
	te = omp_get_wtime();
	printf("[%d] Edge merging time: %lf\n", omp_get_thread_num(), te-ts);
#endif
    }
    std::unordered_map<STG_Key_t, STG_NVal_t> nodes;
    std::unordered_map<STG_Edge_t, STG_EVal_t, edge_hash> edges;
    std::unordered_map<uint64_t/*request*/, record_comm_async_t*> waiting;
  private:
    STG_Key_t last;
    bool last_r_cached;
    std::pair<uint64_t/*ts exit cache*/,std::pair<uint64_t,uint64_t>/*counter cache*/> last_r;
    uint64_t overlap_wait_threshold;
    std::vector<_var_record_async_comm*> async_comms;
};

}; // namespace variance
}; // namespace jsi

bool compareIns (const VarianceMap::CalcVarianceRecord* r1, const VarianceMap::CalcVarianceRecord* r2) {
    return r1->count < r2->count;
}

bool compareIns_opt2 (const VarianceMap::CalcVarianceRecord* r1, const VarianceMap::CalcVarianceRecord* r2) {
    return r1->count_opt_2 < r2->count_opt_2;
}

template<typename K, typename V>
std::vector<std::pair<K, V>> mapToVector(const std::unordered_map<K, V> &map) {
    return std::vector<std::pair<K, V>>(map.begin(), map.end());
}

template<typename V>
std::vector<V> setToVector(const std::set<V> &set_) {
    return std::vector<V>(set_.begin(), set_.end());
}

struct SmoothData_t {
    uint64_t dur;
    uint64_t cnt;
    std::list<VarianceMap::CommVarianceRecord*> rlist;
};

enum PROB_STAT_T {
    A100US=0,
    A10MS,
    A1S,
    AOTHER,
    C100US,
    C10MS,
    C1S,
    COTHER,
    M100US,
    M10MS,
    M1S,
    MOTHER,
    CALC100US,
    CALC10MS,
    CALC1S,
    CALCOTHER,
    LAST_NUM
};

VarianceMap::VarianceMap(RankMetaCollection& metas, RecordTraceCollection& collection, BacktraceCollection& bt_collection, RankExtRecordTraceCollection& etraces_collection, StringSectionCollection& string_sections, const std::string &ref_metric, const std::string &ref_metric_opt, bool enable_async_comm, bool enable_warning) {
    if (enable_async_comm) {
        printf("Enable asynchronized communication event probe detection\n");
    }
    _ref_metric = ref_metric;
    bool enable_opt = false;
    /* construct global STG */
    double ts,te;
    ts = omp_get_wtime();
    auto collection_vec = mapToVector(collection);
    int n = collection_vec.size();
    int nThreads;
    std::set<jsi::variance::STG_Key_t> lks_ptr[256];
    std::set<jsi::variance::STG_Edge_t> les_ptr[256];
    jsi::variance::STG stg_local_ptr[256];

    std::set<jsi::variance::STG_Key_t>& key_set = lks_ptr[0];
    std::set<jsi::variance::STG_Edge_t>& edge_set = les_ptr[0];
    jsi::variance::STG& stg = stg_local_ptr[0];

    std::unordered_map<int, std::unordered_map<int, std::vector<std::pair<uint64_t, uint64_t> >* >* > rank2cidmap;
    double tot = 0;
    double mpi_time = 0;
    double calc_time = 0;
    double gpu_time = 0;
    const uint64_t mhz = get_tsc_freq_mhz();

    enable_accl = false;
    MetaDataMap::MetaValue_t* t;
    std::unordered_map<std::string, MetaDataMap *> tmp_metaMap;
    for (auto meta_it = metas.begin(); meta_it != metas.end(); ++meta_it) {
        tmp_metaMap = meta_it->second->getMetaMap();
        if (tmp_metaMap.contains("ACCL TRACE META")) {
            enable_accl = true;
            break;
        }
    }

    // currently, only matrix has device-level pmu events
    int dev_pmu_num;
    const char* dev_event_str;
    bool is_matrix;
    const char* dev_type;

    if (enable_accl) {
        auto accl_trace_meta_map = tmp_metaMap.at("ACCL TRACE META");
        accl_trace_meta_map->get("ACCL_DEVICE_TYPE", &t);
        dev_type = t->ptr;
        if (std::string(dev_type).substr(0, 6) == "MATRIX") {
            accl_trace_meta_map->get("ACCL_PMU_NUM_EVENTS", &t);
            dev_pmu_num = t->i32;
            accl_trace_meta_map->get("ACCL_PMU_EVENT_LIST", &t);
            dev_event_str = t->ptr;
            is_matrix = true;
        } else {
            dev_pmu_num = 0;
            dev_event_str = nullptr;
            is_matrix = false;
        }
        JSI_INFO("ACCL COLLECTION DETECTED\n");
    } else {
        dev_type = "";
        dev_pmu_num = 0;
        dev_event_str = nullptr;
        is_matrix = false;
        JSI_WARN("No ACCL collection enabled. Skip ACCL\n");
    }

    //for (auto it = collection.begin(), ie = collection.end(); it != ie; ++it) {
    #pragma omp parallel
    {
    #pragma omp single
    {
	nThreads = omp_get_num_threads();
    }
    #pragma barrier
    int tid = omp_get_thread_num();
    double __total_time = 0;
    double __mpi_time = 0;
    double __calc_time = 0;
    #pragma omp for
    for (int i=0; i<n; ++i) {
        // extract info
        auto id = collection_vec[i].first;
        RecordTrace& rtrace = *(collection_vec[i].second);
        int rank = rtrace.rank();
        //RecordTrace& rtrace = *(it->second);
        BacktraceTree* bt = bt_collection[id];

        // printf("Rank %d offset %lu\n", rank, rtrace.offset);

        size_t num_events = rtrace.num_pmu_events();
        int tot_ins_counter_idx=-1;
        int opt_counter_idx=-1;
        for (int idx = 0; idx < num_events; idx++) {
            auto& event_name = rtrace.get_pmu_event_name(idx);
            if (event_name == ref_metric) {
                tot_ins_counter_idx = idx;
            } else if (event_name == ref_metric_opt) {
                opt_counter_idx = idx;
                enable_opt = true;
            }
        }
        if (tot_ins_counter_idx<0) {
            JSI_WARN("No %s event counter collected on rank %d. Skipping this rank...\n",
                     ref_metric.c_str(), rank);
            continue;
        }
        if (enable_opt && opt_counter_idx<0) {
            JSI_WARN("No %s event counter collected on rank %d. Skipping this rank...\n",
                     ref_metric_opt.c_str(), rank);
            continue;
        }

        VarianceData* vdata = new VarianceData;
        vdata->offset = rtrace.offset;
        /* No need */
        vdata->gpu_offset = 0;

        // query for the etraces for this rank
        auto& etraces = etraces_collection[rank];

        if (etraces.empty()) {
            enable_accl = false;
        }
        if (enable_accl) {
            vdata->gpu_offset = 0;
            std::unordered_map<int, std::vector<std::pair<uint64_t, uint64_t> >* >* p_cid2reclist;
            bool try_cache;
            #pragma omp critical
            {
                if (rank2cidmap.find(rank)==rank2cidmap.end()) {
                    p_cid2reclist = new std::unordered_map<int, std::vector<std::pair<uint64_t, uint64_t> >* >();
                    rank2cidmap[rank] = p_cid2reclist;
                    try_cache = true;
                    JSI_INFO("Loading ACCL & contruct cid map cache for rank [%d]\n", rank);
                } else {
                    try_cache = false;
                    JSI_INFO("ACCL cid map Already Cached for rank [%d]\n", rank);
                }
            }
            if (try_cache) {
                if (etraces.size()>1) {
                    JSI_INFO("Multiple etraces for rank %d detected, use the first one [legency].\n", rank);
                }
                auto& etrace = etraces[0];
                // iterate all device cores trace start timestamps 
                auto& cid2reclist = (*p_cid2reclist);
                for(auto& it: etrace->getIteratorList()) {
                    uint64_t off=UINT64_MAX;
                    uint64_t last=0;
                    JSI_INFO(">>> ACCL LOADING: RANK=%d, ID=%d\n", rank, it.id);
                    // if (it.begin_iter!=it.end_iter) {
                    //     auto p_accl_trace = it.begin_iter.get();
                    //     AcclRecordTrace accl_record_trace(p_accl_trace, dev_pmu_num, it.begin_iter.record_size(), dev_event_str, dev_type, string_sections[id]);
                    //     off = accl_record_trace.begin().get_begin_ns();
                    //     // JSI_WARN("==============> BEGIN EVENT INFO (off=%ld) =>\n%s\n=======================================", off, accl_record_trace.begin().to_string().c_str());
                    // }
                    if (std::string(dev_type).substr(0, 6) == "MATRIX") {
                        off = 0;
                    } else {
                        for(auto outer_it = it.begin_iter; outer_it!=it.end_iter; ++outer_it) {
                            auto rec = outer_it.get();
                            AcclRecordTrace accl_record_trace(rec, dev_pmu_num, outer_it.record_size(), dev_event_str, dev_type, string_sections[id]);
                            off = min(off, accl_record_trace.begin().get_begin_ns());
                        }
                    }
                    for(auto outer_it = it.begin_iter; outer_it!=it.end_iter; ++outer_it) {
                        auto rec = outer_it.get();
                        AcclRecordTrace accl_record_trace(rec, dev_pmu_num, outer_it.record_size(), dev_event_str, dev_type, string_sections[id]);
                        for(auto ait = accl_record_trace.begin(); ait!=accl_record_trace.end(); ++ait) {
                            auto cid = ait.get_cid();
                            auto p_reclist_it = p_cid2reclist->find(cid);
                            std::vector<std::pair<uint64_t, uint64_t> >* p_reclist;
                            if(p_reclist_it == p_cid2reclist->end()) {
                                p_reclist = new std::vector<std::pair<uint64_t, uint64_t> >();
                                (*p_cid2reclist)[cid] = p_reclist;
                            } else {
                                p_reclist = p_reclist_it->second;
                            }
                            uint64_t beg = ait.get_begin_ns() - off;
                            uint64_t end = ait.get_end_ns() - off;
                            if (ait.get_begin_ns()<off) {
                                JSI_WARN("Curious timestamp data: beg (%lu)< off (%lu)\n", ait.get_begin_ns(), off);
                                JSI_WARN("==============> Invalid EVENT INFO =>\n%s\n=======================================", ait.to_string().c_str());
                                continue;
                            }
                            if (end<=beg) {
                                JSI_WARN("Ignore event due to invalid timestamp relationship: end (%lu)<=beg (%lu)\n", end, beg);
                                JSI_WARN("==============> Invalid EVENT INFO =>\n%s\n=======================================", ait.to_string().c_str());
                                continue;
                            }
                            p_reclist->push_back(std::make_pair(beg, end));
                            #pragma omp atomic
                            gpu_time += (end - beg)/1000000000.0 /* nsec->sec */;
                            last = max(last, end);
                        }
                    }
                }
            } // end if try_cache
        }
        // printf("rank=%d, size=%ld, offset=%ld, start=%ld, start-offset=%ld\n", rank, rtrace.size(), rtrace.offset, rtrace.find(JSI_PROCESS_START, true/*ignore zoom*/).val()->timestamps.enter, rtrace.find(JSI_PROCESS_START, true/*ignore zoom*/).val()->timestamps.enter-rtrace.offset);
        // STG construction
        stg_local_ptr[tid].reset();
        stg_local_ptr[tid].enable_async_comm = enable_async_comm;
        int cnt = 0;
        uint64_t ts = 0;
        record_t last_r;
        bool last_r_cached = false;
        for (auto ri = rtrace.begin(), re = rtrace.end(); ri != re; ri = ri.next()) {
            record_t* r = ri.val();
            // printf("==== [Rank %d] %d: %d, %s\n", rank, cnt++, r->MsgType, RecordHelper::dump_string(r).c_str());
            // accumulate mpi time for coverage statistics
            if(RecordHelper::is_mpi(r)) {
                __mpi_time += tsc_duration_seconds((r->timestamps.exit) - (r->timestamps.enter), mhz);
            }
            if(RecordHelper::is_process_start(r)) {
                ts = r->timestamps.enter;
                last_r = *r;
                last_r_cached = true;
            }
            else if(RecordHelper::is_process_exit(r)) {
                __total_time += tsc_duration_seconds(r->timestamps.enter-ts, mhz);
            }
            else {
                /* r may be a event within last_r, ignore it */
                if (RecordHelper::is_contain(r, &last_r)) {
                    continue;
                }
                jsi::variance::STG_Edge_t e;
                jsi::variance::STG_Key_t key = stg_local_ptr[tid].transfer_to(rank, r, bt, tot_ins_counter_idx, opt_counter_idx, num_events, e, vdata);
                lks_ptr[tid].emplace(key);
                les_ptr[tid].emplace(e);
                // accumulate calc time (edges)
                if (last_r_cached) {
                    __calc_time += tsc_duration_seconds((r->timestamps.enter) - (last_r.timestamps.exit), mhz);
                }
                last_r = *r;
                last_r_cached = true;
            }
        }
        stg_local_ptr[tid].waiting.clear();
        #pragma omp critical
        _var_map[rank] = vdata;
    }
    #pragma omp critical
    {
        tot += __total_time;
        mpi_time += __mpi_time;
        calc_time += __calc_time;
    }

    #pragma omp single
    collection_vec.clear();
    int nmax = nThreads;
    for (int s=(nThreads+1)/2; s<nmax; s=(s+1)/2, nmax=(nmax+1)/2) {
        #pragma omp barrier
	    if (tid+s<nmax) {
	        stg_local_ptr[tid].merge(stg_local_ptr[tid+s]);
            for(auto it = lks_ptr[tid+s].begin(), ie=lks_ptr[tid+s].end(); it!=ie; ++it) {
	            lks_ptr[tid].emplace(*it);
	        }
            for(auto it = les_ptr[tid+s].begin(), ie=les_ptr[tid+s].end(); it!=ie; ++it) {
	            les_ptr[tid].emplace(*it);
	        }
	    }
    }
    #pragma omp barrier
    if (tid!=0) {
        stg_local_ptr[tid].clear();
        lks_ptr[tid].clear();
        les_ptr[tid].clear();
    }
    } // end of parallel
    te = omp_get_wtime();
    printf("Stage 1: %lf sec\n", te-ts);
    printf("Key Set Size: %lu\n", key_set.size());
    printf("Edge Set Size: %lu\n", edge_set.size());

    printf("============== profile statistics ===============\n");
    printf("## Total time elasped (all processes):                  %lf\n", tot);
    if (enable_accl) {
        printf("## GPU activity in trace total time elasped (all proc): %lf (%.2lf%%)\n", gpu_time, 100*gpu_time/tot);
    }
    printf("=================================================\n");
    ts = omp_get_wtime();
    // if (enable_accl) {
    //     for (auto it=rte_collection.begin(); it!=rte_collection.end(); ++it) {
    //         RecordTraceExt* rte = it->second;
    //         for(RecordTraceExtIterator rit = rte->begin(); rit.valid(); rit.next()) {
    //             ext_record_accl* era = (ext_record_accl*)rit.get();
    //             gpu_time += (era->end_ns - era->begin_ns)/1000000000.0 /* nsec->sec */;
    //         }
    //     }
    // }
    // clustering and normalization within each STG
    // Communications
    auto key_vec = setToVector(key_set);
    auto edge_vec = setToVector(edge_set);
    int nkeys = key_vec.size();
    int nedges = edge_vec.size();
    double vpt = 0;
    double cpt = 0;
    double gct = 0;
    double gmt = 0;
    double pct = 0;
    double pmt = 0;
    unordered_map<int, uint64_t> prob_cnt;
    for(int i=0; i<(int)LAST_NUM; ++i) {
        prob_cnt[i] = 0;
    }
    #pragma omp parallel
    {
    // unordered_map<string /*encoding*/, unordered_map<uint64_t /*us*/, SmoothData_t > > smooth_map;
    unordered_map<string /*encoding*/, SmoothData_t > smooth_map;
    //for (auto ki = key_set.begin(), ke = key_set.end(); ki!=ke; ++ki) {
    #pragma omp for nowait schedule(dynamic)
    for (int i=0; i<nkeys; ++i) {
        const jsi::variance::STG_Key_t& key = key_vec[i];
        if(stg.nodes.find(key)==stg.nodes.end()) {
            JSI_ERROR("Key %lu not found in STG!\n", key);
        }
        std::list<VarianceMap::CommVarianceRecord*>& rlist = stg.nodes[key].rlist;
        if (enable_warning) {
            if (rlist.size()>0) {
                JSI_INFO("[%d] key=%lu, #rlist=%lu, MsgType=%s\n", omp_get_thread_num(), key, rlist.size(), RecordHelper::get_record_name((*rlist.begin())->data).c_str());
            } else {
                JSI_INFO("[%d] key=%lu, #rlist=%lu\n", omp_get_thread_num(), key, rlist.size());
            }
        }
        {
            uint8_t* begin = 0;
            int counter = 0;
            string base_enc = string(":") + to_string(key);
            for(auto it=rlist.begin(), ie=rlist.end(); it!=ie; ++it) {
                int rank = (*it)->rank;
                record_t* r = (*it)->data;
                string enc;
                uint64_t dur = 0; 
                double enter_us;
                // Communication
                if( RecordHelper::is_event(r, event_MPI_Send) ||
                    RecordHelper::is_event(r, event_MPI_Recv)) {
                        enc = jsi::variance::encode_comm(rank, (record_comm_t*)r);
                        dur = (r->timestamps.exit) - (r->timestamps.enter);
                        enter_us = tsc_duration_us(r->timestamps.enter, mhz);
                } else if( RecordHelper::is_event(r, event_MPI_Wait) ||
                           RecordHelper::is_event(r, event_MPI_Waitall)) {
                        // for MPI_Wait, transformed record is encoded in enter
                        enc = jsi::variance::encode_comm((jsi::variance::_var_record_async_comm*)r);
                        dur = (r->timestamps.exit) - (r->timestamps.enter);
                        enter_us = tsc_duration_us(r->timestamps.enter, mhz);
                }
                // ACCL (GPU) Async events
                else if (enable_accl && RecordHelper::is_accl_memcpy_activity(r)) {
                        record_activity_memcpy_t* rec = (record_activity_memcpy_t*)r;
                        enc = jsi::variance::encode_memcpy(rec);
                        int cid = rec->correlation_id;
                        auto p_cidmap = rank2cidmap.find(rank);
                        if(p_cidmap==rank2cidmap.end()) {
                            JSI_ERROR("ACCL EVENT TRACE not found for rank %d\n", rank);
                        }
                        auto p_reclist = p_cidmap->second->find(cid);
                        if(p_reclist!=p_cidmap->second->end()) {
                            auto& _reclist = *(p_reclist->second);
                            if(_reclist.size()>0) {
                                uint64_t enter_ns = _reclist[0].first;
                                uint64_t exit_ns  = _reclist[0].second;
                                for(size_t i=0; i<_reclist.size(); ++i) {
                                    enter_ns = min(enter_ns, _reclist[i].first);
                                    exit_ns = min(exit_ns, _reclist[i].second);
                                }
                                if (std::string(dev_type).substr(0, 6) == "MATRIX") {
                                    auto _kernel_start_ns = tsc_duration_ns(rec->record.timestamps.enter-_var_map[rank]->offset, mhz);
                                    enter_ns += _kernel_start_ns;
                                    exit_ns += _kernel_start_ns;
                                }
                                enter_us = enter_ns / (double)1000;
                                dur = exit_ns - enter_ns;
                                auto dur_sec = dur / 1000000000.0 /* nsec->sec */;
                                #pragma omp atomic
                                gmt+= dur_sec;
                                AcclVarianceRecord* avr = reinterpret_cast<AcclVarianceRecord*>(*it);
                                avr->accl_enter = enter_ns;
                                avr->accl_exit = exit_ns;
                            } else {
                                if (enable_warning) {
                                    JSI_WARN("[MEMCPY] Ignore event with empty record list: correlation id %d\n", cid);
                                }
                                dur = 0;
                            }
                        } else {
                            if (enable_warning) {
                                JSI_WARN("[MEMCPY] Ignore event with unmatched correlation id %d\n", cid);
                            }
                            dur = 0;
                        }
                }
                // ACCL (GPU) Async Calc events
                else if(enable_accl && RecordHelper::is_kernel_launch(r)) {
                        record_activity_t* rec = (record_activity_t*)r;
                        enc = jsi::variance::encode_launch(r);
                        int cid = rec->correlation_id;
                        auto p_cidmap = rank2cidmap.find(rank);
                        if(p_cidmap==rank2cidmap.end()) {
                            JSI_ERROR("ACCL EVENT TRACE not found for rank %d\n", rank);
                        }
                        auto p_reclist = p_cidmap->second->find(cid);
                        if(p_reclist!=p_cidmap->second->end()) {
                            auto& _reclist = *(p_reclist->second);
                            if(_reclist.size()>0) {
                                uint64_t enter_ns = _reclist[0].first;
                                uint64_t exit_ns  = _reclist[0].second;
                                for(size_t i=0; i<_reclist.size(); ++i) {
                                    enter_ns = min(enter_ns, _reclist[i].first);
                                    exit_ns = min(exit_ns, _reclist[i].second);
                                }
                                if (std::string(dev_type).substr(0, 6) == "MATRIX") {
                                    auto _kernel_start_ns = tsc_duration_ns(rec->record.timestamps.enter-_var_map[rank]->offset, mhz);
                                    enter_ns += _kernel_start_ns;
                                    exit_ns += _kernel_start_ns;
                                }
                                enter_us = enter_ns / (double)1000;
                                dur = exit_ns - enter_ns;
                                auto dur_sec = dur / 1000000000.0; /* nsec->sec */;
                                #pragma omp atomic
                                gct+= dur_sec; 
                                AcclVarianceRecord* avr = reinterpret_cast<AcclVarianceRecord*>(*it);
                                avr->accl_enter = enter_ns;
                                avr->accl_exit = exit_ns;
                            } else {
                                if (enable_warning) {
                                    JSI_WARN("[LAUNCH] Ignore event with empty record list: correlation id %d\n", cid);
                                }
                                dur = 0;
                            }
                        } else {
                            if (enable_warning) {
                                JSI_WARN("[LAUNCH] Ignore event with unmatched correlation id %d\n", cid);
                            }
                            dur = 0;
                        }
                }
                if (!enc.empty() && dur) {
                    (*it)->variance = dur;
                    enc += base_enc;
                    auto si = smooth_map.find(enc);
                    if(si==smooth_map.end()) {
                        smooth_map[enc] = {dur, 1, {*it}};
                    } else {
                        si->second.rlist.push_back(*it);
                        if (si->second.dur > dur) {
                            si->second.dur = dur;
                        }
                    }
                }
            }
        }
        {
            double __pct = 0;
            double __pmt = 0;
            double __cpt = 0;
            for(auto si=smooth_map.begin(), se=smooth_map.end(); si!=se; ++si) {
                const string& enc = si->first;
                const double b = (double)si->second.dur;
                int cnt = si->second.rlist.size();
                // Ignore infrequent variance probes
                if (cnt > LOW_CNT_THRESHOLD) {
                    if (enc.c_str()[0]=='M') {
                        if (si->second.dur < 100) {
                            if(prob_cnt.find(M100US) == prob_cnt.end()) prob_cnt[M100US] = 1;
                            else                                        prob_cnt[M100US] += 1;
                        } else if (si->second.dur < 10000) {
                            if(prob_cnt.find(M10MS) == prob_cnt.end())  prob_cnt[M10MS] = 1;
                            else                                        prob_cnt[M10MS] += 1;
                        } else if (si->second.dur < 1000000) {
                            if(prob_cnt.find(M1S) == prob_cnt.end())    prob_cnt[M1S] = 1;
                            else                                        prob_cnt[M1S] += 1;
                        } else {
                            if(prob_cnt.find(MOTHER) == prob_cnt.end()) prob_cnt[MOTHER] = 1;
                            else                                        prob_cnt[MOTHER] += 1;
                        }
                        // __pmt += si->second.dur / 1000000000.0 /*usec->sec*/;
                        for(auto ri=si->second.rlist.begin(), re=si->second.rlist.end(); ri!=re; ++ri) {
                            __pmt += (*ri)->variance / 1000000000.0 /*usec->sec*/;
                        }
                    } else if (enc.c_str()[0]=='A') {
                        if (si->second.dur < 100) {
                            if(prob_cnt.find(A100US) == prob_cnt.end()) prob_cnt[A100US] = 1;
                            else                                        prob_cnt[A100US] += 1;
                        } else if (si->second.dur < 10000) {
                            if(prob_cnt.find(A10MS) == prob_cnt.end())  prob_cnt[A10MS] = 1;
                            else                                        prob_cnt[A10MS] += 1;
                        } else if (si->second.dur < 1000000) {
                            if(prob_cnt.find(A1S) == prob_cnt.end())    prob_cnt[A1S] = 1;
                            else                                        prob_cnt[A1S] += 1;
                        } else {
                            if(prob_cnt.find(AOTHER) == prob_cnt.end()) prob_cnt[AOTHER] = 1;
                            else                                        prob_cnt[AOTHER] += 1;
                        }
                        // __pct += si->second.dur / 1000000000.0 /*usec->sec*/;
                        for(auto ri=si->second.rlist.begin(), re=si->second.rlist.end(); ri!=re; ++ri) {
                            __pct += (*ri)->variance / 1000000000.0 /*usec->sec*/;
                        }
                    } else {
                        double t = tsc_duration_us(si->second.dur, mhz);
                        if (t < 100) {
                            if(prob_cnt.find(C100US) == prob_cnt.end()) prob_cnt[C100US] = 1;
                            else                                        prob_cnt[C100US] += 1;
                        } else if (t < 10000) {
                            if(prob_cnt.find(C10MS) == prob_cnt.end())  prob_cnt[C10MS] = 1;
                            else                                        prob_cnt[C10MS] += 1;
                        } else if (t < 1000000) {
                            if(prob_cnt.find(C1S) == prob_cnt.end())    prob_cnt[C1S] = 1;
                            else                                        prob_cnt[C1S] += 1;
                        } else {
                            if(prob_cnt.find(COTHER) == prob_cnt.end()) prob_cnt[COTHER] = 1;
                            else                                        prob_cnt[COTHER] += 1;
                        }
                        // __cpt += tsc_duration_seconds(si->second.dur, mhz);
                        for(auto ri=si->second.rlist.begin(), re=si->second.rlist.end(); ri!=re; ++ri) {
                            __cpt += tsc_duration_seconds((*ri)->variance, mhz);
                        }
                    }
                    // communication variance
                    for(auto ri=si->second.rlist.begin(), re=si->second.rlist.end(); ri!=re; ++ri) {
                        assert(b <= (*ri)->variance);
                        (*ri)->variance = b / (*ri)->variance;
                    }
                } else {
                    for(auto ri=si->second.rlist.begin(), re=si->second.rlist.end(); ri!=re; ++ri) {
                        (*ri)->variance = 1;
                    }
                }
            }
            #pragma omp critical
            {
                pct += __pct;
                pmt += __pmt;
                cpt += __cpt;
            }
        }
        smooth_map.clear();
    }
    // Computations
    double __valid_prob_time = 0;
    uint64_t __calc_cnt[4] = {0};
    std::vector<VarianceMap::CalcVarianceRecord*> rlist_all;
    //for (auto ki = edge_set.begin(), ke = edge_set.end(); ki!=ke; ++ki) {
    #pragma omp for nowait schedule(dynamic)
    for (int i=0; i<nedges; ++i) {
        const jsi::variance::STG_Edge_t& key = edge_vec[i];
	    {
            std::list<VarianceMap::CalcVarianceRecord*>& rlist = stg.edges[key].rlist;
            rlist_all.reserve(rlist.size());
            // printf("[%d] key=<%lu,%lu>, #rlist=%lu\n", omp_get_thread_num(), key.first, key.second, rlist.size());
            // first clustering them into different clusters
            for(auto it=rlist.begin(), ie=rlist.end(); it!=ie; ++it) {
                // rlist_all.push_back(*it);
                if((*it)->count>INS_COUNT_THRESHOLD) {
                    rlist_all.push_back(*it);
                }
            }
            // printf("[%d] key=<%lu,%lu>, #rlist=%lu, #rlist_all=%lu\n", omp_get_thread_num(), key.first, key.second, rlist.size(), rlist_all.size());
        }
        std::sort(rlist_all.begin(), rlist_all.end(), compareIns);
        // clustering and compute variance
        int cnt = 0;
        for (int i=0; i<rlist_all.size();) {
            uint64_t ref_ins = VAR_CLUSTER_RATIO*rlist_all[i]->count;
            int c2_s=i;
            int c2_e=i+1;
            while(c2_e<rlist_all.size()) {
                uint64_t tot_ins = rlist_all[c2_e]->count;
                if(tot_ins > ref_ins) break;
                ++c2_e;
            }
            if (c2_e-c2_s<=LOW_CNT_THRESHOLD) {
                // Ignore infrequent variance probes
                for(;i<c2_e;++i) {
                    rlist_all[i]->variance = 1;
                }
                continue;
            }
            
            if(enable_opt) {
                std::sort(std::next(rlist_all.begin(),c2_s), std::next(rlist_all.begin(),c2_e), compareIns_opt2);
            }
            for(; i<c2_e; ++i) {
                uint64_t ref_opt2 = enable_opt ? VAR_CLUSTER_RATIO*rlist_all[i]->count_opt_2 : (uint64_t)(-1);
                int n=i+1;
                double worst = (double)(CalcVarianceRecordGetExitTS(rlist_all[i]) - CalcVarianceRecordGetEnterTS(rlist_all[i]));
                double best = worst;
                rlist_all[i]->variance = best;
                while(n<c2_e) {
                    uint64_t tot_opt2 = rlist_all[n]->count_opt_2;
                    if(tot_opt2 > ref_opt2) break;
                    double dur = (double)(CalcVarianceRecordGetExitTS(rlist_all[n]) - CalcVarianceRecordGetEnterTS(rlist_all[n]));
                    rlist_all[n]->variance = dur;
                    if(dur<best) { best = dur; }
                    if(dur>worst) { worst = dur; }
                    ++n;
                }
                if (n-i<=LOW_CNT_THRESHOLD) {
                    // Ignore infrequent variance probes
                    for(;i<n;++i) {
                        rlist_all[i]->variance = 1;
                    }
                } else {
                    // JSI_INFO("[CLUSTER %d] %d-%d (num=%d), ref_ins_low=%ld, ref_ins=%ld, best=%lf, worst=%lf, ratio=%lf\n", cnt++, i, n, n-i, rlist_all[i]->count, ref_ins, best, worst, best/worst);
                    for(;i<n;++i) {
                        double t = tsc_duration_us(rlist_all[i]->variance, mhz);
                        if (t < 100) {
                            __calc_cnt[0]++;
                        } else if (t < 10000) {
                            __calc_cnt[1]++;
                        } else if (t < 1000000) {
                            __calc_cnt[2]++;
                        } else {
                            __calc_cnt[3]++;
                        }
                        __valid_prob_time += tsc_duration_seconds(rlist_all[i]->variance, mhz);
                        assert(best <= rlist_all[i]->variance);
                        rlist_all[i]->variance = best / rlist_all[i]->variance;
                    }
                }
            }
        }
        rlist_all.clear();
    }
    #pragma omp critical
    {
    vpt += __valid_prob_time;
    prob_cnt[CALC100US] += __calc_cnt[0];
    prob_cnt[CALC10MS]  += __calc_cnt[1];
    prob_cnt[CALC1S]    += __calc_cnt[2];
    prob_cnt[CALCOTHER] += __calc_cnt[3];
    }
    } // end of parallel (comm & calc)
    te = omp_get_wtime();
    printf("## Total time elasped (all processes):                  %lf\n", tot);
    if (enable_accl) {
        printf("## GPU activity in trace total time elasped (all proc): %lf (%.2lf%%)\n", gpu_time, 100*gpu_time/tot);
    }
    printf("## Coverage statistics at each scope:\n");
    printf("## [COMM       ] valid probe time=%lf, total time=%lf, coverage=%.2lf%% (MPI Time %.2lf%%)\n", cpt, mpi_time, 100*cpt/mpi_time, 100*mpi_time/tot);
    printf("## [CALC       ] valid probe time=%lf, total time=%lf, coverage=%.2lf%% (CALC Time %.2lf%%)\n", vpt, calc_time, 100*vpt/calc_time, 100*calc_time/tot);
    if(enable_accl) {
        printf("## [ACCL CALC  ] valid probe time=%lf, total time=%lf, coverage=%.2lf%% (ACCL Time %.2lf%%)\n", pct, gct, 100*pct/gct, 100*gct/tot);
        printf("## [ACCL MEMCPY] valid probe time=%lf, total time=%lf, coverage=%.2lf%% (ACCL Time %.2lf%%)\n", pmt, gmt, 100*pmt/gmt, 100*gmt/tot);
    }
    printf("## COUNT STATISTICS\n");
    printf("## [COMM       ] 100us %lu, 10ms %lu, 1s %lu, other %lu\n", prob_cnt[C100US], prob_cnt[C10MS], prob_cnt[C1S], prob_cnt[COTHER]);
    printf("## [CALC       ] 100us %lu, 10ms %lu, 1s %lu, other %lu\n", prob_cnt[CALC100US], prob_cnt[CALC10MS], prob_cnt[CALC1S], prob_cnt[CALCOTHER]);
    if(enable_accl) {
        printf("## [ACCL CALC  ] 100us %lu, 10ms %lu, 1s %lu, other %lu\n", prob_cnt[A100US], prob_cnt[A10MS], prob_cnt[A1S], prob_cnt[AOTHER]);
        printf("## [ACCL MEMCPY] 100us %lu, 10ms %lu, 1s %lu, other %lu\n", prob_cnt[M100US], prob_cnt[M10MS], prob_cnt[M1S], prob_cnt[MOTHER]);
    }
    printf("Stage 2 COMM && CALC: %lf sec\n", te-ts);
    ts = omp_get_wtime();
    stg.clear();
    te = omp_get_wtime();
    printf("Stage 3 CLEAR: %lf sec\n", te-ts);
}

VarianceMap::~VarianceMap() {
    for (auto it = _var_map.begin(), ie = _var_map.end(); it != ie; ++it) {
        auto& comm_vars = it->second->comm_vars;
        for (auto it : comm_vars) {
            record_t* r = it->data;
            if (RecordHelper::is_event(r, event_MPI_Wait) ||
                RecordHelper::is_event(r, event_MPI_Waitall)) {
                    delete r;    
            }
        }
        delete it->second;
    }
    _var_map.clear();
}

void VarianceMap::dump_accl_memcpy_csv(const char* filename) {
    if (!enable_accl) return ;
    uint64_t mhz = get_tsc_freq_mhz();
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        JSI_ERROR("Failed to open file for write: %s\n", filename);
    }
    auto var_vec = mapToVector(_var_map);
    int nvar = var_vec.size();
    printf("DUMP COMM: nvar = %d\n", nvar);
    #pragma omp parallel for schedule(dynamic)
    for (int i=0; i<nvar; ++i) {
        int rank = var_vec[i].first;
        ostringstream sout;
        sout << rank;
        VarianceData* vdata = var_vec[i].second;
        uint64_t offset = tsc_duration_ns(vdata->offset, mhz);
        vector<AcclVarianceRecord*>& accl_memcpy_vars = vdata->accl_memcpy_vars;
        int n = accl_memcpy_vars.size();
        for (int i = 0; i < n; ++i) {
            if(accl_memcpy_vars[i]->comm_rec.variance<=0) continue;
	        sout << "," << accl_memcpy_vars[i]->accl_enter-offset
                 << "," << accl_memcpy_vars[i]->accl_exit-offset
                 << "," << accl_memcpy_vars[i]->comm_rec.variance;
        }
	    sout << "\n";
        #pragma omp critical
	    if(write(fileno(fp), sout.str().c_str(), sout.str().size()) !=sout.str().size()) {
            JSI_ERROR("Could not append line to file: %s\n", filename);
	    }
        //fprintf(fp, "%s\n", sout.str().c_str());
    }
    fclose(fp);
}

void VarianceMap::dump_accl_calc_csv(const char* filename) {
    if (!enable_accl) return ;
    uint64_t mhz = get_tsc_freq_mhz();
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        JSI_ERROR("Failed to open file for write: %s\n", filename);
    }
    auto var_vec = mapToVector(_var_map);
    int nvar = var_vec.size();
    printf("DUMP COMM: nvar = %d\n", nvar);
    #pragma omp parallel for schedule(dynamic)
    for (int i=0; i<nvar; ++i) {
        int rank = var_vec[i].first;
        ostringstream sout;
        sout << rank;
        VarianceData* vdata = var_vec[i].second;
        uint64_t offset = tsc_duration_ns(vdata->offset, mhz);
        vector<AcclVarianceRecord*>& accl_calc_vars = vdata->accl_calc_vars;
        int n = accl_calc_vars.size();
        for (int i = 0; i < n; ++i) {
            if(accl_calc_vars[i]->comm_rec.variance<=0) continue;
	        sout << "," << accl_calc_vars[i]->accl_enter-offset
                 << "," << accl_calc_vars[i]->accl_exit-offset
                 << "," << accl_calc_vars[i]->comm_rec.variance;
        }
	    sout << "\n";
        #pragma omp critical
	    if(write(fileno(fp), sout.str().c_str(), sout.str().size()) !=sout.str().size()) {
            JSI_ERROR("Could not append line to file: %s\n", filename);
	    }
        //fprintf(fp, "%s\n", sout.str().c_str());
    }
    fclose(fp);
}

void VarianceMap::dump_comm_csv(const char* filename) {
    uint64_t mhz = get_tsc_freq_mhz();
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        JSI_ERROR("Failed to open file for write: %s\n", filename);
    }
    auto var_vec = mapToVector(_var_map);
    int nvar = var_vec.size();
    printf("DUMP COMM: nvar = %d\n", nvar);
    // for (auto it = _var_map.begin(), ie = _var_map.end(); it != ie; ++it) {
    #pragma omp parallel for schedule(dynamic)
    for (int i=0; i<nvar; ++i) {
        int rank = var_vec[i].first;
        //fprintf(fp, "%d", rank);
        ostringstream sout;
        sout << rank;
        VarianceData* vdata = var_vec[i].second;
        uint64_t offset = vdata->offset;
        vector<CommVarianceRecord*>& comm_vars = vdata->comm_vars;
	// printf("rank=%d, offset=%ld, [0] enter=%ld, exit=%ld\n", rank, offset, comm_vars[0]->data->timestamps.enter, comm_vars[0]->data->timestamps.exit);
	// printf("++ enter-offset=%ld, ns=%lf, mhz=%ld\n", comm_vars[0]->data->timestamps.enter-offset, tsc_duration_ns(comm_vars[0]->data->timestamps.enter-offset, mhz), mhz);
        int n = comm_vars.size();
        for (int i = 0; i < n; ++i) {
            if(comm_vars[i]->variance<=0) continue;
	        sout << "," << tsc_duration_ns(comm_vars[i]->data->timestamps.enter-offset, mhz)
                 << "," << tsc_duration_ns(comm_vars[i]->data->timestamps.exit-offset, mhz)
                 << "," << comm_vars[i]->variance;
            // fprintf(fp, ",%lf,%lf,%lf", tsc_duration_ns(comm_vars[i]->data->timestamps.enter-offset, mhz),
            //         tsc_duration_ns(comm_vars[i]->data->timestamps.exit-offset, mhz),
            //         comm_vars[i]->variance);
        }
	    sout << "\n";
        #pragma omp critical
	    if(write(fileno(fp), sout.str().c_str(), sout.str().size()) !=sout.str().size()) {
            JSI_ERROR("Could not append line to file: %s\n", filename);
	    }
        //fprintf(fp, "%s\n", sout.str().c_str());
    }
    fclose(fp);
}

void VarianceMap::dump_calc_csv(const char* filename) {
    uint64_t mhz = get_tsc_freq_mhz();
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        JSI_ERROR("Failed to open file for write: %s\n", filename);
    }
    auto var_vec = mapToVector(_var_map);
    int nvar = var_vec.size();
    printf("DUMP CALC: nvar = %d\n", nvar);
    // for (auto it = _var_map.begin(), ie = _var_map.end(); it != ie; ++it) {
    #pragma omp parallel for schedule(dynamic)
    for (int i=0; i<nvar; ++i) {
        int rank = var_vec[i].first;
        //fprintf(fp, "%d", rank);
	ostringstream sout;
	sout << rank;
        VarianceData* vdata = var_vec[i].second;
        uint64_t offset = vdata->offset;
        vector<CalcVarianceRecord*>& calc_vars = vdata->calc_vars;
        int n = calc_vars.size();
        for (int i = 0; i < n; ++i) {
            if(calc_vars[i]->variance<=0) {
                continue;
                // printf("???????????%d\n",i);
            }
            sout << "," << tsc_duration_ms(CalcVarianceRecordGetEnterTS(calc_vars[i])-offset, mhz)
		 << "," << tsc_duration_ms(CalcVarianceRecordGetExitTS(calc_vars[i])-offset, mhz)
		 << "," << calc_vars[i]->variance;
            // fprintf(fp, ",%lf,%lf,%lf", tsc_duration_ns(calc_vars[i]->ts_enter-offset, mhz),
            //         tsc_duration_ns(calc_vars[i]->ts_exit-offset, mhz), calc_vars[i]->variance);
        }
	sout << "\n";
        #pragma omp critical
	if(write(fileno(fp), sout.str().c_str(), sout.str().size()) !=sout.str().size()) {
            JSI_ERROR("Could not append line to file: %s\n", filename);
	}
        //fprintf(fp, "%s\n", sout.str().c_str());
    }
    fclose(fp);
}

const std::unordered_map<int, VarianceMap::VarianceData*>& VarianceMap::getVarMap() {
    return _var_map;
}


void VarianceMap::dump_comm_heatmap(const char* filename, uint64_t resolution_ms) {
    uint64_t mhz = get_tsc_freq_mhz();
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        JSI_ERROR("Failed to open file for write: %s\n", filename);
    }
    fprintf(fp, "%lu\n", resolution_ms);
    fflush(fp);
    auto var_vec = mapToVector(_var_map);
    int nvar = var_vec.size();
    printf("DUMP COMM heatmap: nvar = %d, resolution = %lu ms\n", nvar, resolution_ms);
    #pragma omp parallel for schedule(dynamic)
    for (int i=0; i<nvar; ++i) {
        int rank = var_vec[i].first;
        ostringstream sout;
        sout << rank;
        VarianceData* vdata = var_vec[i].second;
        uint64_t offset = vdata->offset;
        vector<CommVarianceRecord*>& comm_vars = vdata->comm_vars;
        int n = comm_vars.size();
        uint64_t last_r = 0;
        double variance = 0;
        int count = 0;
        // record in comm_vars should be ordered by the exiting timestamp
        for (int ci = 0; ci < n; ++ci) {
            if(comm_vars[ci]->variance<=0) continue;
            record_t* r = comm_vars[ci]->data;
            uint64_t r_enter = (uint64_t)(tsc_duration_ms(r->timestamps.enter-offset, mhz) / resolution_ms);
            uint64_t r_exit  = (uint64_t)(tsc_duration_ms(r->timestamps.exit-offset, mhz) / resolution_ms);
            // printf("[Rank=%d] i=%d (n=%d, off=%lu): last_r=%d, Msg=%s, enter=%lu (ts=%lu), exit=%lu (ts=%lu), variance=%lf, acc. count=%d, acc. variance=%lf\n",
            //     rank, i, n, offset, last_r, RecordHelper::get_record_name(r).c_str(), r_enter, r->timestamps.enter, r_exit, r->timestamps.exit, comm_vars[ci]->variance, count, variance);
            // add padding variance value when there are no probes detected
            if (r_enter!=last_r) {
                //assert(r_enter > last_r);
                if (count==0) {
                    sout << ",1";
                } else {
                    sout << "," << variance / count;
                }
                for(uint64_t ri=last_r+1; ri<r_enter; ++ri) {
                    sout << ",1";
                }
                variance = 0;
                count = 0;
            }
            if (r_enter!=r_exit) {
                for (uint64_t ri=r_enter; ri<r_exit; ++ri) {
                    sout << "," << comm_vars[ci]->variance;
                }
                count = 1;
                variance = comm_vars[ci]->variance;
            } else {
                count++;
                variance += comm_vars[ci]->variance;
            }
            last_r = r_exit;
        }
	    sout << "\n";
        // printf("## RANK %d dumping to file: size=%u\n", rank, sout.str().size());
        #pragma omp critical
	    if(write(fileno(fp), sout.str().c_str(), sout.str().size()) !=sout.str().size()) {
            JSI_ERROR("Could not append line to file: %s\n", filename);
	    }
        // printf("## RANK %d file dumped\n", rank);
    }
    fclose(fp);
}
void VarianceMap::dump_calc_heatmap(const char* filename, uint64_t resolution_ms) {
    uint64_t mhz = get_tsc_freq_mhz();
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        JSI_ERROR("Failed to open file for write: %s\n", filename);
    }
    fprintf(fp, "%lu\n", resolution_ms);
    fflush(fp);
    auto var_vec = mapToVector(_var_map);
    int nvar = var_vec.size();
    printf("DUMP CALC heatmap: nvar = %d, resolution = %lu ms\n", nvar, resolution_ms);
    #pragma omp parallel for schedule(dynamic)
    for (int i=0; i<nvar; ++i) {
        int rank = var_vec[i].first;
        std::unordered_map<uint64_t/*derived from resolution*/, std::pair<double, int>> heatmap;
        ostringstream sout;
        sout << rank;
        VarianceData* vdata = var_vec[i].second;
        uint64_t offset = vdata->offset;
        vector<CalcVarianceRecord*>& calc_vars = vdata->calc_vars;
        int n = calc_vars.size();
        uint64_t r_max = 0;
        for (int i = 0; i < n; ++i) {
            if(calc_vars[i]->variance<=0) continue;
            uint64_t r_enter = (uint64_t)(tsc_duration_ms(CalcVarianceRecordGetEnterTS(calc_vars[i])-offset, mhz) / resolution_ms);
            uint64_t r_exit  = (uint64_t)(tsc_duration_ms(CalcVarianceRecordGetExitTS(calc_vars[i])-offset, mhz) / resolution_ms);
            for(uint64_t r=r_enter; r<=r_exit; ++r) {
                if (heatmap.find(r)==heatmap.end()) {
                    heatmap[r] = std::make_pair(calc_vars[i]->variance, 1);
                } else {
                    heatmap[r].first += calc_vars[i]->variance;
                    heatmap[r].second += 1;
                }
            }
            if (r_max < r_exit) {
                r_max = r_exit;
            }
        }
        uint64_t uncovered = 0;
        for(uint64_t i=0; i<r_max; ++i) {
            if (heatmap.find(i)==heatmap.end()) {
                uncovered++;
                sout << ",1";
            } else {
                sout << "," << heatmap[i].first/heatmap[i].second;
            }
        }
	    sout << "\n";
        #pragma omp critical
	    if(write(fileno(fp), sout.str().c_str(), sout.str().size()) !=sout.str().size()) {
            JSI_ERROR("Could not append line to file: %s\n", filename);
	    }
    }
    fclose(fp);
}
void VarianceMap::dump_accl_calc_heatmap(const char* filename, uint64_t resolution_ms) {
    if (!enable_accl) return ;
    uint64_t mhz = get_tsc_freq_mhz();
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        JSI_ERROR("Failed to open file for write: %s\n", filename);
    }
    fprintf(fp, "%lu\n", resolution_ms);
    fflush(fp);
    auto var_vec = mapToVector(_var_map);
    int nvar = var_vec.size();
    printf("DUMP ACCL CALC heatmap: nvar = %d, resolution = %lu ms\n", nvar, resolution_ms);
    #pragma omp parallel for schedule(dynamic)
    for (int i=0; i<nvar; ++i) {
        int rank = var_vec[i].first;
        std::unordered_map<uint64_t/*derived from resolution*/, std::pair<double, int>> heatmap;
        ostringstream sout;
        sout << rank;
        VarianceData* vdata = var_vec[i].second;
        uint64_t offset = vdata->gpu_offset;
        //printf("offset=%lu ns, gpu offset %lu ns\n", tsc_duration_ns(vdata->offset, mhz), offset);
        vector<AcclVarianceRecord*>& accl_calc_vars = vdata->accl_calc_vars;
        int n = accl_calc_vars.size();
        uint64_t r_max = 0;
        for (int i = 0; i < n; ++i) {
            if(accl_calc_vars[i]->comm_rec.variance<=0) continue;
            if(accl_calc_vars[i]->accl_enter < offset) { JSI_ERROR("enter %lu < offset %lu!\n", accl_calc_vars[i]->accl_enter, offset); }
            uint64_t r_enter = (uint64_t)((accl_calc_vars[i]->accl_enter-offset) / 1000000.0/*ns->ms*/ / resolution_ms);
            uint64_t r_exit  = (uint64_t)((accl_calc_vars[i]->accl_exit-offset) / 1000000.0/*ns->ms*/ / resolution_ms);
            for(uint64_t r=r_enter; r<=r_exit; ++r) {
                if (heatmap.find(r)==heatmap.end()) {
                    heatmap[r] = std::make_pair(accl_calc_vars[i]->comm_rec.variance, 1);
                } else {
                    heatmap[r].first += accl_calc_vars[i]->comm_rec.variance;
                    heatmap[r].second += 1;
                }
            }
            if (r_max < r_exit) {
                r_max = r_exit;
            }
        }
        uint64_t uncovered = 0;
        for(uint64_t i=0; i<r_max; ++i) {
            if (heatmap.find(i)==heatmap.end()) {
                uncovered++;
                sout << ",1";
            } else {
                sout << "," << heatmap[i].first/heatmap[i].second;
            }
        }
	    sout << "\n";
        #pragma omp critical
	    if(write(fileno(fp), sout.str().c_str(), sout.str().size()) !=sout.str().size()) {
            JSI_ERROR("Could not append line to file: %s\n", filename);
	    }
    }
    fclose(fp);
}
void VarianceMap::dump_accl_memcpy_heatmap(const char* filename, uint64_t resolution_ms) {
    if (!enable_accl) return ;
    uint64_t mhz = get_tsc_freq_mhz();
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        JSI_ERROR("Failed to open file for write: %s\n", filename);
    }
    fprintf(fp, "%lu\n", resolution_ms);
    fflush(fp);
    auto var_vec = mapToVector(_var_map);
    int nvar = var_vec.size();
    printf("DUMP ACCL MEMCPY heatmap: nvar = %d, resolution = %lu ms\n", nvar, resolution_ms);
    #pragma omp parallel for schedule(dynamic)
    for (int i=0; i<nvar; ++i) {
        int rank = var_vec[i].first;
        std::unordered_map<uint64_t/*derived from resolution*/, std::pair<double, int>> heatmap;
        ostringstream sout;
        sout << rank;
        VarianceData* vdata = var_vec[i].second;
        uint64_t offset = vdata->gpu_offset;
        vector<AcclVarianceRecord*>& accl_memcpy_vars = vdata->accl_memcpy_vars;
        int n = accl_memcpy_vars.size();
        uint64_t r_max = 0;
        for (int i = 0; i < n; ++i) {
            if(accl_memcpy_vars[i]->comm_rec.variance<=0) continue;
            uint64_t r_enter = (uint64_t)((accl_memcpy_vars[i]->accl_enter-offset) / 1000000.0/*ns->ms*/ / resolution_ms);
            uint64_t r_exit  = (uint64_t)((accl_memcpy_vars[i]->accl_exit-offset) / 1000000.0/*ns->ms*/ / resolution_ms);
            for(uint64_t r=r_enter; r<=r_exit; ++r) {
                if (heatmap.find(r)==heatmap.end()) {
                    heatmap[r] = std::make_pair(accl_memcpy_vars[i]->comm_rec.variance, 1);
                } else {
                    heatmap[r].first += accl_memcpy_vars[i]->comm_rec.variance;
                    heatmap[r].second += 1;
                }
            }
            if (r_max < r_exit) {
                r_max = r_exit;
            }
        }
        uint64_t uncovered = 0;
        for(uint64_t i=0; i<r_max; ++i) {
            if (heatmap.find(i)==heatmap.end()) {
                uncovered++;
                sout << ",1";
            } else {
                sout << "," << heatmap[i].first/heatmap[i].second;
            }
        }
	    sout << "\n";
        #pragma omp critical
	    if(write(fileno(fp), sout.str().c_str(), sout.str().size()) !=sout.str().size()) {
            JSI_ERROR("Could not append line to file: %s\n", filename);
	    }
    }
    fclose(fp);
}