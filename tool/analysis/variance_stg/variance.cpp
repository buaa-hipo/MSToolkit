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

#define VAR_CLUSTER_RATIO (1.05)
#define INS_COUNT_THRESHOLD (0)
#define COMM_COUNT_THRESHOLD (100)

namespace jsi {

namespace variance {

uint64_t encode(record_t* rec, BacktraceTree* bt) {
    // printf("Encoding %lu %p: %s\n", rec->ctxt, bt, RecordHelper::dump_string(rec).c_str());
    backtrace_context_t ctxt = rec->ctxt;
    std::string bt_str(bt->backtrace_get_context_string(ctxt));
    while (bt_str.find("libmpi_wrapper.so") != std::string::npos) {
        ctxt = bt->get_parent(ctxt);
        if(ctxt!=BACKTRACE_UNKNOWN_NODE) {
            bt_str = std::string(bt->backtrace_get_context_string(ctxt));
        } else {
            JSI_ERROR("May be corrupted backtrace!\n");
        }
    } 
    backtrace_context_t pctxt = bt->get_parent(ctxt);
    if(pctxt!=BACKTRACE_UNKNOWN_NODE) {
        return ctxt ^ pctxt;
    }
    return ctxt ^ pctxt;
}

string encode_comm(record_comm_t* r) {
    int count = r->count;
    if (count < COMM_COUNT_THRESHOLD) {
        // Too few comm for stablity detection, ignore
        return "";
    }
    return string("comm/") + to_string(r->datatype) + string("/") + to_string(count) + string("/") +
        to_string(r->record.MsgType) + string("/") + to_string(r->dest);
}

typedef uint64_t STG_Key_t; /* current ip OR backtrace context*/
// per rank state tranfer graph for online loop detection & counting
struct STG_NVal_t {
    // record counts for temporal sampling, count from 0
    std::vector<VarianceMap::CommVarianceRecord*> rlist;
};

struct STG_EVal_t {
    std::vector<VarianceMap::CalcVarianceRecord*> rlist;
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
    STG() { last=0; last_r=NULL; }
    ~STG() {}
    STG_Key_t transfer_to(record_t* r, 
                          BacktraceTree* bt, 
                          int tot_ins_counter_idx, 
                          int num_events, 
                          STG_Edge_t& e, 
                          std::vector<VarianceMap::CommVarianceRecord*>& commRecs, 
                          std::vector<VarianceMap::CalcVarianceRecord*>& calcRecs) {
        STG_Key_t key = encode(r, bt);
        STG_NVal_t& nval = nodes[key];
        VarianceMap::CommVarianceRecord* cr = new VarianceMap::CommVarianceRecord({0, r});
        nval.rlist.push_back(cr);
        if( RecordHelper::is_event(r, event_MPI_Send) ||
            RecordHelper::is_event(r, event_MPI_Recv)) {
                commRecs.push_back(cr);
        }
        if (last_r!=NULL) {
            e = std::make_pair(last, key);
            STG_EVal_t& eval = edges[e];
            uint64_t tot_ins = RecordHelper::counter_diff(r, tot_ins_counter_idx, num_events);
            VarianceMap::CalcVarianceRecord* cr2 = new VarianceMap::CalcVarianceRecord({0, tot_ins, last_r->timestamps.exit, r->timestamps.enter});
            eval.rlist.push_back(cr2);
            calcRecs.push_back(cr2);
        }
        last = key;
        last_r = r;
        return key;
    }
    std::unordered_map<STG_Key_t, STG_NVal_t> nodes;
    std::unordered_map<STG_Edge_t, STG_EVal_t, edge_hash> edges;
  private:
    STG_Key_t last;
    record_t* last_r;
};

}; // namespace variance
}; // namespace jsi

bool compareIns (const VarianceMap::CalcVarianceRecord* r1, const VarianceMap::CalcVarianceRecord* r2) {
    return r1->ins_count < r2->ins_count;
}

std::unordered_map<void*, int> check;

VarianceMap::VarianceMap(RecordTraceCollection& collection, BacktraceCollection& bt_collection, const std::string &ref_metric) {
    _ref_metric = ref_metric;
    /* construct global STG */
    std::set<jsi::variance::STG_Key_t> key_set;
    std::set<jsi::variance::STG_Edge_t> edge_set;
    std::unordered_map<int, jsi::variance::STG> stg_list;
    for (auto it = collection.begin(), ie = collection.end(); it != ie; ++it) {
        int rank = it->first;
        RecordTrace& rtrace = *(it->second);
        BacktraceTree* bt = bt_collection[rank];
        jsi::variance::STG& stg = stg_list[rank];

        size_t num_events = rtrace.num_pmu_events();
        int tot_ins_counter_idx;
        for (tot_ins_counter_idx = 0; tot_ins_counter_idx < num_events; tot_ins_counter_idx++) {
            auto& event_name = rtrace.get_pmu_event_name(tot_ins_counter_idx);
            if (event_name == ref_metric) {
                break;
            }
        }
        if (tot_ins_counter_idx == num_events) {
            JSI_WARN("No %s event counter collected on rank %d. Skipping this rank...\n",
                     ref_metric.c_str(), rank);
            continue;
        }

        VarianceData* vdata = new VarianceData;
        vdata->offset = rtrace.offset;
        int cnt = 0;
        for (auto ri = rtrace.begin(), re = rtrace.end(); ri != re; ri = ri.next()) {
            record_t* r = ri.val();
            // printf("==== [Rank %d] %d: %d, %s\n", rank, cnt++, r->MsgType, RecordHelper::dump_string(r).c_str());
            if(RecordHelper::is_mpi(r) || RecordHelper::is_function(r)) { 
                jsi::variance::STG_Edge_t e;
                jsi::variance::STG_Key_t key = stg.transfer_to(r, bt, tot_ins_counter_idx, num_events, e, vdata->comm_vars, vdata->calc_vars);
                key_set.emplace(key);
                edge_set.emplace(e);
            }
        }

        _var_map[rank] = vdata;
    }

    // clustering and normalization within each STG
    // Communications
    unordered_map<string /*encoding*/, uint64_t /*duration*/> best;
    for (auto ki = key_set.begin(), ke = key_set.end(); ki!=ke; ++ki) {
        const jsi::variance::STG_Key_t& key = *ki;
        best.clear();
        for (auto it = stg_list.begin(), ie = stg_list.end(); it != ie; ++it) {
            int rank = it->first;
            jsi::variance::STG& stg = it->second;
            std::vector<VarianceMap::CommVarianceRecord*>& rlist = stg.nodes[key].rlist;
            for(int i=0; i<rlist.size(); ++i) {
                record_t* r = rlist[i]->data;
                if( RecordHelper::is_event(r, event_MPI_Send) ||
                    RecordHelper::is_event(r, event_MPI_Recv)) {
                        string enc = jsi::variance::encode_comm((record_comm_t*)r);
                        if (!enc.empty()) {
                            uint64_t dur = (r->timestamps.exit) - (r->timestamps.enter);
                            // Update to the most performant duration
                            auto bi = best.find(enc);
                            if (bi == best.end() || bi->second > dur) {
                                best[enc] = dur;
                            }
                            rlist[i]->variance = dur;
                        }
                }
            }
        }
        // communication variance
        for (auto it = stg_list.begin(), ie = stg_list.end(); it != ie; ++it) {
            int rank = it->first;
            jsi::variance::STG& stg = it->second;
            std::vector<VarianceMap::CommVarianceRecord*>& rlist = stg.nodes[key].rlist;
            for(int i=0; i<rlist.size(); ++i) {
                record_t* r = rlist[i]->data;
                if( RecordHelper::is_event(r, event_MPI_Send) ||
                    RecordHelper::is_event(r, event_MPI_Recv)) {
                        string enc = jsi::variance::encode_comm((record_comm_t*)r);
                        if (!enc.empty()) {
                            uint64_t dur = (r->timestamps.exit) - (r->timestamps.enter);
                            rlist[i]->variance = best[enc] / rlist[i]->variance;
                        }
                }
            }
        }
    }

    // Computations
    std::vector<VarianceMap::CalcVarianceRecord*> rlist_all;
    for (auto ki = edge_set.begin(), ke = edge_set.end(); ki!=ke; ++ki) {
        const jsi::variance::STG_Edge_t& key = *ki;
        rlist_all.clear();
        for (auto it = stg_list.begin(), ie = stg_list.end(); it != ie; ++it) {
            int rank = it->first;
            jsi::variance::STG& stg = it->second;
            std::vector<VarianceMap::CalcVarianceRecord*>& rlist = stg.edges[key].rlist;
            // first clustering them into different clusters
            for(int i=0; i<rlist.size(); ++i) {
                if(rlist[i]->ins_count>INS_COUNT_THRESHOLD) {
                    rlist_all.push_back(rlist[i]);
                } else {
                    rlist[i]->variance = -1;
                    if(check.find(rlist[i])==check.end()) {
                        check[rlist[i]] = 1;
                    } else {
                        JSI_ERROR("???????error!!!!!\n");
                    }
                }
            }
        }
        std::sort(rlist_all.begin(), rlist_all.end(), compareIns);
        // clustering and compute variance
        int cnt = 0;
        for (int i=0; i<rlist_all.size();) {
            uint64_t ref_ins = VAR_CLUSTER_RATIO*rlist_all[i]->ins_count;
            int n=i+1;
            double worst = (double)(rlist_all[i]->ts_exit - rlist_all[i]->ts_enter);
            double best = (double)(rlist_all[i]->ts_exit - rlist_all[i]->ts_enter);
            rlist_all[i]->variance = best;
            while(n<rlist_all.size()) {
                uint64_t tot_ins = rlist_all[n]->ins_count;
                if(tot_ins > ref_ins) break;
                double dur = (double)(rlist_all[n]->ts_exit - rlist_all[n]->ts_enter);
                rlist_all[n]->variance = dur;
                if(dur<best) { best = dur; }
                if(dur>worst) { worst = dur; }
                ++n;
            }
            printf("[CLUSTER %d] %d-%d (num=%d), ref_ins_low=%ld, ref_ins=%ld, best=%lf, worst=%lf, ratio=%lf\n", cnt++, i, n, n-i, rlist_all[i]->ins_count, ref_ins, best, worst, best/worst);
            for(;i<n;++i) {
                rlist_all[i]->variance = best / rlist_all[i]->variance;
                if(check.find(rlist_all[i])==check.end()) {
                    check[rlist_all[i]] = 1;
                } else {
                    JSI_ERROR("???????error!!!!!\n");
                }
            }
        }
    }

}

VarianceMap::~VarianceMap() {
    for (auto it = _var_map.begin(), ie = _var_map.end(); it != ie; ++it) {
        delete it->second;
    }
    _var_map.clear();
}

void VarianceMap::dump_comm_csv(const char* filename) {
    uint64_t mhz = get_tsc_freq_mhz();
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        JSI_ERROR("Failed to open file for write: %s\n", filename);
    }
    for (auto it = _var_map.begin(), ie = _var_map.end(); it != ie; ++it) {
        int rank = it->first;
        fprintf(fp, "%d", rank);
        VarianceData* vdata = it->second;
        uint64_t offset = vdata->offset;
        vector<CommVarianceRecord*>& comm_vars = vdata->comm_vars;
        int n = comm_vars.size();
        for (int i = 0; i < n; ++i) {
            //if(comm_vars[i]->data==0) continue;
            fprintf(fp, ",%lf,%lf,%lf", tsc_duration_ns(comm_vars[i]->data->timestamps.enter-offset, mhz),
                    tsc_duration_ns(comm_vars[i]->data->timestamps.exit-offset, mhz),
                    comm_vars[i]->variance);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void VarianceMap::dump_calc_csv(const char* filename) {
    uint64_t mhz = get_tsc_freq_mhz();
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        JSI_ERROR("Failed to open file for write: %s\n", filename);
    }
    for (auto it = _var_map.begin(), ie = _var_map.end(); it != ie; ++it) {
        int rank = it->first;
        fprintf(fp, "%d", rank);
        VarianceData* vdata = it->second;
        uint64_t offset = vdata->offset;
        vector<CalcVarianceRecord*>& calc_vars = vdata->calc_vars;
        int n = calc_vars.size();
        int cnt = 0;
        for (int i = 0; i < n; ++i) {
            if(check.find(calc_vars[i])==check.end()) {
                // JSI_ERROR("dljafeljfkajflkejafjk?????FAE");
                cnt ++;
                continue;
            }
            if(calc_vars[i]->variance<=0) {
                continue;
                // printf("???????????%d\n",i);
            }
            fprintf(fp, ",%lf,%lf,%lf", tsc_duration_ns(calc_vars[i]->ts_enter-offset, mhz),
                    tsc_duration_ns(calc_vars[i]->ts_exit-offset, mhz), calc_vars[i]->variance);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

const std::unordered_map<int, VarianceMap::VarianceData*>& VarianceMap::getVarMap() {
    return _var_map;
}