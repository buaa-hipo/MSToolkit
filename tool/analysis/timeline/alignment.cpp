#include "alignment.h"
#include "utils/jsi_log.h"
#include "math.h"
#include "utils/tsc_timer.h"

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

TimelineAlignment::TimelineAlignment() {}
TimelineAlignment::~TimelineAlignment() {}

char* get_hostname_from_meta(RankMetaCollection& metas, int rank) {
	MetaDataMap::MetaValue_t *t;
	const std::unordered_map<std::string, MetaDataMap *> & metaMap = metas[rank]->getMetaMap();
	metaMap.at("HOST INFO")->get("HOSTNAME", &t);
    return t->ptr;
}

// travese only the main thread
bool TimelineAlignment::align_for_main_thread_traces(RankRecordTraceCollection& collection, RankMetaCollection& metas) {
    uint64_t max_ts = 0;
    uint64_t min_ts = (uint64_t)(-1);
    double std_varience = 0;
    double average = 0;
    std::unordered_map<std::string, uint64_t> offsets;
    for(auto it=collection.begin(), ie=collection.end(); it!=ie; ++it)  {
        RecordTrace& rt = *(it->second);
        int rank = it->first;
        RecordTraceIterator rti = rt.global_begin();
        // RecordTraceIterator rti = rt.find(JSI_PROCESS_START, true/*ignore zoom*/);
        if(RecordTraceIterator::is_invalid(rti)) {
            JSI_ERROR("Error: [align_for_main_thread_traces] Could not find PROCESS_START event in Rank %d\n", rank);
        }
        char* hostname = get_hostname_from_meta(metas, rank);
        auto off = offsets.find(hostname);
        if(off==offsets.end()) {
            offsets[hostname] = rti.val()->timestamps.enter;
        } else {
            offsets[hostname] = MIN(off->second, rti.val()->timestamps.enter);
        }
    }
    for(auto it=collection.begin(), ie=collection.end(); it!=ie; ++it)  {
        RecordTrace& rt = *(it->second);
        int rank = it->first;
        char* hostname = get_hostname_from_meta(metas, rank);
        rt.offset = offsets[hostname];
        JSI_INFO("Rank %d: Alignment offset %lu\n", rank, rt.offset);
    }
    return true;
}

bool TimelineAlignment::align(RecordTraceCollection& collection, RankMetaCollection& metas) {
    uint64_t max_ts = 0;
    uint64_t min_ts = (uint64_t)(-1);
    double std_varience = 0;
    double average = 0;
    std::unordered_map<std::string, uint64_t> offsets;
    for(auto it=collection.begin(), ie=collection.end(); it!=ie; ++it)  {
        auto id = it->first;
        RecordTrace& rt = *(it->second);
        int rank = rt.rank();
        char* hostname = get_hostname_from_meta(metas, rank);
        if (rt.size()==0) {
            JSI_INFO("WARNING: Ignore empty trace for Rank %d\n", rank);
            continue;
        }
        RecordTraceIterator rti = rt.global_begin();
        // RecordTraceIterator rti = rt.find(JSI_PROCESS_START, true/*ignore zoom*/);
        if(RecordTraceIterator::is_invalid(rti)) {
            JSI_ERROR("Error: [TimelineAlignment::align] Could not find PROCESS_START event in Rank %d\n", rank);
        }
        //char* hostname = get_hostname_from_meta(metas, rank);
        auto off = offsets.find(hostname);
        if(off==offsets.end()) {
            offsets[hostname] = rti.val()->timestamps.enter;
        } else {
            offsets[hostname] = MIN(off->second, rti.val()->timestamps.enter);
        }
    }
    for(auto it=collection.begin(), ie=collection.end(); it!=ie; ++it)  {
        RecordTrace& rt = *(it->second);
        if (rt.size()==0) {
            continue;
        }
        int rank = rt.rank();
        char* hostname = get_hostname_from_meta(metas, rank);
        rt.offset = offsets[hostname];
        JSI_INFO("Rank %d: Alignment offset %lu\n", rank, rt.offset);
    }
    return true;
}

bool TimelineAlignment::verify(RecordTraceCollection& collection) {
    std::vector<std::string> violations;
    return verifyWithNotification(collection, violations, 0);
}

bool TimelineAlignment::verifyWithNotification(RecordTraceCollection& collection, std::vector<std::string>& violations, int max_size) {
    bool success = true;
    violations.clear();
    if(max_size>0) {
        violations.resize(max_size);
    }

    

    return success;
}
