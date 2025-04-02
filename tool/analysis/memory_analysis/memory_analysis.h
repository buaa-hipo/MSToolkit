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

template<typename K, typename V>
std::vector<std::pair<K, V>> mapToVector(const std::unordered_map<K, V> &map) {
    return std::vector<std::pair<K, V>>(map.begin(), map.end());
}

// Interfaces to derive Statistics from the records
class MemoryAnalyzer {
    public:
        MemoryAnalyzer(RecordTraceCollection& traces, BacktraceCollection *backtraces, RankMetaCollection& metas, std::string output_dir, bool pretty_print);
        ~MemoryAnalyzer();

    private:
        void init_recordtrace(RecordTrace& rtrace,int rank);
        bool accept_record_source(backtrace_context_t &ctx, BacktraceTree &bt_tree);
        void analysis_usage(int rank,int count);
        void analysis_memory_leak(int rank, int count);

        void check_meta_info(int rank);
        MetaDataMap::MetaValue_t* get_meta_value(const char *section, const char *key,int rank);


        RecordTraceCollection& traces;
        BacktraceCollection& backtraces;
        RankMetaCollection& metas;
        std::string output_dir;
        bool pretty_print;

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

#endif
