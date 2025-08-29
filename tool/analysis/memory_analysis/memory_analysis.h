#ifndef __JSI_MEMORY_ANALYSIS_H__
#define __JSI_MEMORY_ANALYSIS_H__

#include <stdio.h>
#include <vector>
#include "record/wrap_defines.h"
#include <unordered_map>
#include "record/record_type.h"
#include "record/record_reader.h"
#include "record/record_writer.h"
#include "utils/tsc_timer.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>

template<typename K, typename V>
std::vector<std::pair<K, V>> mapToVector(const std::unordered_map<K, V> &map) {
    return std::vector<std::pair<K, V>>(map.begin(), map.end());
}

class MemoryAnalyzerTraceData
{
    public:
        MemoryAnalyzerTraceData() {};

        MemoryAnalyzerTraceData(int rank, std::string id) :rank(rank), id(id) {
            this->alloc_times = 0;
            this->free_times = 0;
            this->event_num = 0;
            this->all_event_num = 0;
        };

        int rank;
        std::string id;

        int event_num;
        int all_event_num;

        uint64_t alloc_times;
        uint64_t free_times;

        struct memory_usage_item {
            metric_t timestamps;
            uint64_t alloc_size;
            uint64_t free_size;
        };
        std::vector<memory_usage_item> memory_usage_list;

        struct memory_leak_item {
            uint64_t size;
            uint64_t ctxt;
        };
        std::unordered_map<uint64_t, memory_leak_item> ptr2size;
};

// Interfaces to derive Statistics from the records
class MemoryAnalyzer {
    public:
        MemoryAnalyzer(RecordTraceCollection& traces, BacktraceCollection *backtraces, RankMetaCollection& metas, std::string output_dir, bool pretty_print, bool enable_check_source,bool enable_mt, double mt_percent,bool enable_general,bool enable_accl,bool enable_usage,bool enable_leak,int rank_offset,int rank_num);
        ~MemoryAnalyzer();

    // private:
        // std::map<std::string,MemoryAnalyzerTraceData*> init_recordtrace(RecordTrace& rtrace,int rank, std::string id);
        std::map<std::string, std::unique_ptr<MemoryAnalyzerTraceData>> init_recordtrace(RecordTrace& rtrace,int rank, std::string id);
        bool accept_record_source(backtrace_context_t &ctx, BacktraceTree &bt_tree);
        void analysis_usage(std::string trace_data_name);
        void analysis_usage_trace(MemoryAnalyzerTraceData* trace_data,std::mutex &print_mutex,std::string trace_data_name);
        void analysis_usage_trace_print(MemoryAnalyzerTraceData* trace_data,std::string trace_data_name);
        void analysis_memory_leak(std::string trace_data_name);
        void analysis_memory_leak_trace(MemoryAnalyzerTraceData* trace_data,std::mutex &print_mutex,std::string trace_data_name);
        void analysis_memory_leak_trace_print(MemoryAnalyzerTraceData* trace_data,int leak_size,int n,std::string trace_data_name);
        // void analysis_usage(std::string trace_data_name);
        // void analysis_usage_trace(std::unique_ptr<MemoryAnalyzerTraceData> trace_data,std::mutex &print_mutex,std::string trace_data_name);
        // void analysis_usage_trace_print(std::unique_ptr<MemoryAnalyzerTraceData> trace_data,std::string trace_data_name);
        // void analysis_memory_leak(std::string trace_data_name);
        // void analysis_memory_leak_trace(std::unique_ptr<MemoryAnalyzerTraceData> trace_data,std::mutex &print_mutex,std::string trace_data_name);
        // void analysis_memory_leak_trace_print(std::unique_ptr<MemoryAnalyzerTraceData> trace_data,int leak_size,int n,std::string trace_data_name);

        void check_meta_info(int rank);
        MetaDataMap::MetaValue_t* get_meta_value(const char *section, const char *key,int rank);

        RecordTraceCollection& traces;
        BacktraceCollection& backtraces;
        RankMetaCollection& metas;
        std::string output_dir;
        bool pretty_print;
        bool enable_check_source;
        bool enable_mt;
        bool enable_general;
        bool enable_accl;
        double mt_percent;
        bool enable_usage;
        bool enable_leak;
        // int record_trace_num;
        //解决一次读入销号内存过大
        int max_round_size = 100;
        int current_round_size;
        int round_index = 0;
        int round_completed_size;

        int rank_offset;
        int rank_num;

        // std::vector<MemoryAnalyzerTraceData*> trace_data_list;
        // std::vector<MemoryAnalyzerTraceData*> trace_data_list_accl;
        std::vector<std::unique_ptr<MemoryAnalyzerTraceData>> trace_data_list;
        std::vector<std::unique_ptr<MemoryAnalyzerTraceData>> trace_data_list_accl;

        // uint64_t alloc_times;
        // uint64_t free_times;

        // struct memory_usage_item {
        //     metric_t timestamps;
        //     uint64_t alloc_size;
        //     uint64_t free_size;
        // };
        // std::vector<memory_usage_item> memory_usage_list;

        // struct memory_leak_item {
        //     uint64_t size;
        //     uint64_t ctxt;
        // };
        // std::unordered_map<uint64_t, memory_leak_item> ptr2size;
};



#endif
