#ifndef __JSI_COMM_ANALYSIS_H__
#define __JSI_COMM_ANALYSIS_H__
#include <stdio.h>
#include <vector>
#include "record/record_type.h"
#include "record/record_reader.h"
#include "record/wrap_defines.h"
#include "utils/tsc_timer.h"

// Interfaces to derive Statistics from the records
class CommunicationAnalyzer {
  public:
    struct CommAnalyzeItem {
      CommAnalyzeItem(int rank, record_t* r, uint64_t dur, uint64_t volume, int pmu_num) : rank(rank), data(r), duration(dur), volume(volume) {
        int size = RecordHelper::get_record_size(r, pmu_num);
        data = (record_t*) malloc(size);
        memcpy(data, r, size);
      }

      // CommAnalyzeItem(CommAnalyzeItem&& item) {
      //   move_from(std::move(item));
      // }

      // CommAnalyzeItem& operator=(CommAnalyzeItem &&item) {
      //   move_from(std::move(item));
      //   return *this;
      // }
      // CommAnalyzeItem&operator=(const CommAnalyzeItem &) = delete;

      // void move_from(CommAnalyzeItem &&item) {
      //     this->data = item.data;
      //     item.data = nullptr;
      // }

      ~CommAnalyzeItem() {
        free(data);
      }
      int rank;
      record_t *data;
      double duration;
      uint64_t volume;
      double time_percentage;
    } ;

    typedef struct {
        std::vector<CommAnalyzeItem*> items;
        uint64_t t_total;
    } CommAnalyzeRecord;

    // Constructor & Destructor
    CommunicationAnalyzer(RecordReader& reader, RecordTraceCollection& collection, BacktraceCollection *backtraces, RankMetaCollection& metas, int topk, double threshold_lwt, double threshold_wtv, double threshold_S, double threshold_n, double threshold_t, bool pretty_print, const char *filename);
    ~CommunicationAnalyzer();
    
    void get_comm_world_from_meta(RankMetaCollection& metas, int rank);

    void get_comm_world_from_meta(RankMetaCollection& metas, int rank, std::unordered_map<uint64_t, std::string> &comm2name);
    
    void add_Comm_to_map(int rank, uint64_t old_comm, uint64_t new_comm);
    
    void add_Comm_to_map(int rank, uint64_t old_comm, int color, uint64_t new_comm);

    void print_high_proportion_MPI_calls(std::ofstream &f_out);

    void print_long_wait(std::ofstream &f_out);

    void print_high_comm_volume_MPI_calls(std::ofstream &f_out);

    void print_high_freq_MPI_calls(std::ofstream &f_out);
    
    void dump_report(const char* filename);

  private:
    BacktraceCollection& _backtraces;

    int _num_top_k;
    bool _pretty_print;

    double _threshold_lwt; // long wait time
    double _threshold_wtv; // wait time variance

    double _threshold_S; // Comm volume

    double _threshold_n; // comm number (<=)
    double _threshold_t; // comm time (>=)

    /* barrier */
    // use ori comm name and current rank number to index dup/split order.
    std::unordered_map<std::string, int> _commrank2dup_count, _commrank2split_count;
    std::unordered_map<std::string, int> _ori_comm2dup_count, _ori_comm2split_count;

    std::unordered_map<uint64_t, std::string> _comm2name;

    // all the comm split from the same old comm with the same color have the same new name.
    std::unordered_map<std::string, std::unordered_map<int,std::string>> _old_comm_name_to_color2new_name;

    typedef std::vector<std::pair<int,record_barrier_t*>> _barrier_rank_event_list; 
    typedef std::unordered_map<int32_t, _barrier_rank_event_list> _barrier_order_to_event_list;
    std::unordered_map<std::string, _barrier_order_to_event_list> _mpi_comm_2_event_map;

    std::unordered_map<int, double> _rank_e2etime_map; // e2e i.e. t_procexit - t_procstart?

    std::unordered_map<std::string, std::pair<int, uint64_t>> _bt_funcfreq_map;
    
    CommAnalyzeRecord _data_comm;
    CommAnalyzeRecord _data_wait;

};

#endif
