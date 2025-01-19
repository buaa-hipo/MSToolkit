#ifndef __JSI_RECORD_DEFINES_H__
#define __JSI_RECORD_DEFINES_H__


// 采集数据 ---------> 压缩----------------->从Snappy解压---------->解压到原文件
// data.rank.tr-----> data.rank.tmp------->data.rank.tar-------->data.rank.tr
#define DEFAULT_PROFILE_BASE "data."
#define JSI_TRACE_FILE_EXT ".tr"
#define JSI_SECTION_FILE_EXT ".sec-tr"
#define JSI_META_FILE_EXT ".meta"
#define JSI_BACKTRACE_FILE_BASE "backtrace_db."
#define JSI_BACKTRACE_FILE_EXT ".bin"
#define JSI_TRACE_READ_COMPRESS_EXT  ".tar"
#define JSI_TRACE_WRITE_COMPRESS_EXT  ".tmp"
#define JSI_EXT_TRACE_FILE_EXT ".etr"
#define JSI_EXT_TRACE_SYM_FILE_EXT ".etr-sym"
#include <string>
#include <ral/section_declare.h>
namespace StaticSectionDesc
{
enum 
{
    BACKTRACE_STRING_SEC = pse::ral::INIT_STATIC_ID + 1,
    BACKTRACE_NODE_SEC = pse::ral::INIT_STATIC_ID + 2,
    META_SEC_OFFSET = pse::ral::INIT_STATIC_ID + 3,
    TRACE_SEC_ID = pse::ral::INIT_STATIC_ID + 4,
    BACKTRACE_SEC_ID = pse::ral::INIT_STATIC_ID + 5,
    GENERIC_TRACE_SEC_ID = pse::ral::INIT_STATIC_ID + 6,
    SAMPLING_TRACE_SEC_ID = pse::ral::INIT_STATIC_ID + 7,
    EXT_TRACE_SEC_ID = pse::ral::INIT_STATIC_ID + 8,

    RECORD_SEC_OFFSET = pse::ral::INIT_STATIC_ID + 100000,
};
}

#define COMMON_COMPRESS_RANK -1
#define DATA_MODEL 1
#define COMPRESS_MODEL 2
#define SECTION_MODEL 3

#include "record/wrap_defines.h"
#include "utils/jsi_log.h"

static inline std::string getBacktraceFileName(const char* dir, const std::string& identifier) {
    return std::string(dir) + "/" JSI_BACKTRACE_FILE_BASE + identifier + JSI_BACKTRACE_FILE_EXT;
}

static inline std::string getTraceFileName(const char* dir, const std::string& identifier) {
    return std::string(dir) + "/" DEFAULT_PROFILE_BASE + identifier + JSI_TRACE_FILE_EXT;
}

static inline std::string getMetaFileName(const char* dir, const std::string& identifier) {
    return std::string(dir) + "/" DEFAULT_PROFILE_BASE + identifier + JSI_META_FILE_EXT;
}

static inline std::string getTraceCompressReadFileName(const char* dir, const std::string& identifier) {
    return std::string(dir) + "/" DEFAULT_PROFILE_BASE + identifier + JSI_TRACE_READ_COMPRESS_EXT;
}

static inline std::string getTraceFileName(const std::string& identifier) {
    char *envPath = getenv("JSI_MEASUREMENT_DIR");
    if (!envPath) {
        JSI_ERROR("(id=%s): JSI_MEASUREMENT_DIR must be set!\n", identifier.c_str());
    }
    std::string fn = "";
    std::string base_dir = envPath;
    fn = base_dir + std::string("/") + DEFAULT_PROFILE_BASE + identifier + JSI_TRACE_FILE_EXT;
    return fn;
}


static inline std::string getMetaFileName(const std::string& identifier) {
    char *envPath = getenv("JSI_MEASUREMENT_DIR");
    if (!envPath) {
        JSI_ERROR("(id=%s): JSI_MEASUREMENT_DIR must be set!\n", identifier.c_str());
    }
    std::string fn = "";
    std::string base_dir = envPath;
    fn = base_dir + std::string("/") + DEFAULT_PROFILE_BASE + identifier + JSI_META_FILE_EXT;
    return fn;
}

static inline std::string getTraceCompressReadFileName(const std::string& identifier) {
    char *envPath = getenv("JSI_MEASUREMENT_DIR");
    if (!envPath) {
        JSI_ERROR("(id=%s): JSI_MEASUREMENT_DIR must be set!\n", identifier.c_str());
    }
    std::string fn = "";
    std::string base_dir = envPath;
    fn = base_dir + std::string("/") + DEFAULT_PROFILE_BASE + identifier + JSI_TRACE_READ_COMPRESS_EXT;
    return fn;
}


static inline std::string getTraceCompressWriteFileName(const std::string& identifier) {
     char *envPath = getenv("JSI_MEASUREMENT_DIR");
    if (!envPath) {
        JSI_ERROR("(id=%s): JSI_MEASUREMENT_DIR must be set!\n", identifier.c_str());
    }
    std::string fn = "";
    std::string base_dir = envPath;
    fn = base_dir + std::string("/") + DEFAULT_PROFILE_BASE + identifier + JSI_TRACE_WRITE_COMPRESS_EXT;
    return fn;
}




#endif