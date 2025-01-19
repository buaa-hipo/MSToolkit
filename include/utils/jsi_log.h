#ifndef __JSI_LOG_H__
#define __JSI_LOG_H__

#include <stdlib.h>
#include <stdio.h>

#define __JSI_WARN__ "=== JSI Warning ==="
#define __JSI_ERROR__ "=== JSI Error ==="
#define __JSI_INFO__ "=== JSI Info ==="

enum JSILOG_LEVEL {
    JSILOG_ALWAYS=0,
    JSILOG_WARN,
    JSILOG_INFO,
    JSILOG_DEBUG,
    JSILOG_ALL
};
#define LOG_LEVEL JSILOG_LEVEL::JSILOG_ALL
#define JSI_LOG(level, format...) do { if(level<=LOG_LEVEL) fprintf(stderr, "[JSILOG] " format); } while(0)

#define JSI_INFO(format...) JSI_LOG(JSILOG_INFO, format)
#define JSI_WARN(format...) JSI_LOG(JSILOG_WARN, format)
#define JSI_ERROR(format...) do { \
    fprintf(stderr, __JSI_ERROR__ format); \
    exit(-1); \
} while(0)

#define ASSERT(stat) do {\
    if(!(stat)) fprintf(stderr, "JSI Assert Failed: " #stat);\
} while(0)
#define ASSERT_MSG(stat, msg) do {\
    if(!(stat)) fprintf(stderr, "JSI Assert Failed: %s (%s)", msg, #stat);\
} while(0)

#ifdef DEBUG
#  define DEBUG_JSIINFO(format...) JSI_INFO(format)
#  define DASSERT(stat) ASSERT(stat) 
#  define DASSERT_MSG(stat, msg) ASSERT_MSG(stat, msg)
#else
#  define DEBUG_JSIINFO(format...)
#  define DASSERT(stat)
#  define DASSERT_MSG(stat, msg)
#endif

#endif