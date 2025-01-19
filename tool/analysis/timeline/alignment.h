#ifndef __JSI_TIMELINE_ALIGNMENT_H__
#define __JSI_TIMELINE_ALIGNMENT_H__

#include "record/record_reader.h"
#include <vector>
#include <string>

class TimelineAlignment {
    public:
    TimelineAlignment();
    ~TimelineAlignment();
    bool align(RecordTraceCollection& collection, RankMetaCollection& metas);
    bool verify(RecordTraceCollection& collection);
    /* verify the timeline alignment by checking if there are violations 
     * of the semantic orders, including 1) Recv end before Send begin.
     * @param collection is a trace collection loaded by RecordReader.
     * @param violations is a empty string vector for output notifications
     * @param max_size is the maximum notifications generated. -1 results
     * in all notification generated.
     */
    bool verifyWithNotification(RecordTraceCollection& collection, std::vector<std::string>& violations, int max_size);
};

class BF_TimelineAlignment {
    public:
    BF_TimelineAlignment() {}
    ~BF_TimelineAlignment() {}
    bool align(RecordTraceCollection& collection, RankMetaCollection& metas);
};

#endif
