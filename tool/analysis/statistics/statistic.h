#ifndef __JSI_STATISTIC_H__
#define __JSI_STATISTIC_H__
#include <stdio.h>
#include <vector>
#include <omp.h>
#include "record/record_type.h"
#include "record/record_reader.h"
#include "record/wrap_defines.h"
#include "instrument/backtrace.h"
#include "utils/tsc_timer.h"

class TopDownTree;

class BottomUpForest;

class BottomUpTree;

// Interfaces to derive Statistics from the records
class Statistics {
  public:
    // Constructor & Destructor
    Statistics(RecordTraceCollection& record_collection, RankMetaCollection& meta_collection, BacktraceCollection& backtrace_collection);
    ~Statistics();
    // Metric Interfaces
    enum DeriveMode {
        FLAT, BOTTOM_UP/* TODO */, TOP_DOWN /* TODO */
    };
    typedef struct {
        uint64_t cycle;
        /*TODO: helper functions to convert the cycle into seconds using the helper function in tsc_timer.h*/
        double sec()  {
            uint64_t tsc_freq_mhz = get_tsc_freq_mhz();
            return tsc_duration_seconds(cycle, tsc_freq_mhz);
        } // second
        double msec() {
            uint64_t tsc_freq_mhz = get_tsc_freq_mhz();
            return tsc_duration_ms(cycle, tsc_freq_mhz);
        } // mili second (10e-3 sec)
        double usec() {
            uint64_t tsc_freq_mhz = get_tsc_freq_mhz();
            return tsc_duration_us(cycle, tsc_freq_mhz);
        } // micro second (10e-6 sec)
        double nsec() {
            uint64_t tsc_freq_mhz = get_tsc_freq_mhz();
            return tsc_duration_ns(cycle, tsc_freq_mhz);
        } // nano second (10e-9 sec)
    } time_t;
    typedef struct {
        time_t tot_time;
        time_t avg_time;
        time_t std_time;
        time_t square_tot;
        time_t square_avg;
        uint64_t n_call;
    } func_stat_t;
    typedef std::unordered_map<std::string, func_stat_t> FuncStatMap;
    typedef struct {
        /* comm + sync <= 1.0 */
        double comm; /* comm time / total time, events: all other than MPI_Barrier */
        double sync; /* sync time / total time, events: MPI_Barrier */
    } comm_brkdwn_t;
    typedef struct {
        double tot;
        double avg;
        double std;
        uint64_t n_call;
    } pmu_stat_t;
    typedef std::unordered_map<std::string, std::unordered_map<std::string, pmu_stat_t> > PMUStatMap;
    typedef std::unordered_map<std::string, std::unordered_map<std::string, std::vector<double> > > CalMap;
    typedef void (*MetricFunc_t) (std::unordered_map<std::string/*Metric name*/, double/*Value*/>&);
    typedef std::unordered_map<int,time_t> RankTotalTimeMap;
    // interfaces to derive metrics
    void dump_top_down_trees(std::string file_dir);
    void dump_bottom_up_trees(std::string file_dir);
    void iter_record(Statistics::time_t& totalTime, Statistics::FuncStatMap& funcStatMap, std::vector<double>& comm_brkdwn_dist, DeriveMode mode);
    /* Total Execution time measured in the trace, typically: max(ProcessEnd-ProcessStart) */
    time_t get_total_time();
    /* Statistics of all events with the same event type */
    FuncStatMap* get_all_stat(DeriveMode mode);
    /* MPI statistics including execution time and number of calls of each MPI function call */
    FuncStatMap* get_MPI_stat(DeriveMode mode);
    /* User-specified Function statistics including execution time and number of calls of each non-MPI function call */
    FuncStatMap* get_Func_stat(DeriveMode mode);
    /* Query statistics via name */
    func_stat_t get_stat_with_name(std::string name, DeriveMode mode);
    /* Compute breakdown of communication ratio of rank <rank> */
    comm_brkdwn_t get_Comm_Breakdown_with_rank(int rank);
    /* Compute Communication ratio of rank <rank> (comm+sync)/total time */
    double get_Comm_Ratio_with_rank(int rank);
    /* Compute communication ratio of all traced ranks.
     * Return all computed ratio in dist, with rank indexed.
     * Compute the average and standard values of derived comm ratio if not NULL. */
    void get_Comm_Ratio_dist(std::vector<double>& dist, double* avg=NULL, double* std=NULL);
    /* ================ Merge & Derive PMU metrics ==================== */
    /* Register the metric derive functions into the metric list */
    void RegisterMetricFunc(std::string name, MetricFunc_t func);
    /* Derive metrics for <rank> */
    void get_metrics_with_rank(PMUStatMap&, int rank);
    /* Derive the averaged metrics across all ranks */
    void get_metrics_merged(PMUStatMap&);
    /* Derive metrics of all ranks */
    void get_metrics_dist(std::vector<PMUStatMap>&);
    /* ================================================================ */
    // do not extend APIs
    // ----- end API ------
  private:
    RecordTraceCollection _record_collection;
    RankMetaCollection _meta_collection;
    BacktraceCollection _backtrace_collection;
    RankTotalTimeMap _rank_total_time_map;
    std::vector<uint64_t> _rank_total_time_list;
    std::vector<TopDownTree> _top_down_tree_list;
    std::vector<BottomUpForest> _bottom_up_forest_list;
    std::unordered_map<std::string/*Metric name*/, MetricFunc_t> _metricFuncMap;
    // add helper variables and functions as private
    /* Total excution time of each trace*/
    time_t get_total_time_with_rank(int rank);
    /* Count tot and n_call while deriving calMap*/
    void get_cal_map_with_rank(int rank, PMUStatMap&, CalMap&);
    /*Calculate avg&std via tot&n_call existed in pmuStatMap*/
    void cal_avg_std(PMUStatMap&, CalMap&);

    TopDownTree* _topdown_overview;
};

// Built-in metric derive functions
namespace builtin_metrics {
    // Derive and add metric "CPI" into the given metric list. 
    // Report error when required metric is not exist.
    void deriveCPI(std::unordered_map<std::string/*Metric name*/, double/*Value*/>&);
}

#endif