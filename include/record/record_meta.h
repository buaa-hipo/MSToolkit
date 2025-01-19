//
// Created by chengyanshang on 2022/10/15.
//

#ifndef JSI_TOOLKIT_RECORD_META_H
#define JSI_TOOLKIT_RECORD_META_H


#include <unordered_map>
#include <string>
#include <stack>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "record/record_defines.h"
#include "ral/backend.h"
#include "ral/section.h"
#include "utils/jsi_log.h"
#include "utils/jsi_io.h"

static const int META_SECTION_NAME = 1;
static const int META_INFO = 2;

/* Data structure to store meta datas */
class MetaDataMap {
public:
    typedef enum _meta_type_t {
        BOOL,
        INT8,
        INT16,
        INT32,
        INT64,
        SP,
        DP,
        RAW_PTR,
        STRING,
        UNKNOW
    } MetaType_t;

    union MetaValue_t {
        bool bl;
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;
        float flt;
        double dp;
        char *ptr;
        struct {
            int64_t size;
            void *ptr;
        } raw;
    };

    MetaDataMap(std::string);

    bool set(const char *, const void *p, size_t size);
    bool set(const char *, const char *p, size_t size);


    /* The section can be seperated by '/', e.g., "section/meta" */
    template<typename T>
    bool set(const char *name, T val);

    MetaType_t get(const char *name, MetaValue_t ** t);

    const std::string &getSectionName() const;

    const std::unordered_map <std::string, std::pair<MetaType_t, MetaValue_t> > &getMetaMap() const;

    std::string to_string();


private:
    typedef std::unordered_map <std::string, std::pair<MetaType_t, MetaValue_t> > _meta_map_t;
    _meta_map_t _meta_map;
    std::string sectionName;
};


class RecordMeta {
private:
    std::unordered_map<std::string, MetaDataMap *> metaMap;
    std::stack <std::string> sectionList;         //当前的sectionName
    jsio metaIO;
    bool readOnly;
    std::string section;
    void * ptr;
    // read a section data to metaMap
    int readFileSection();


    // read a key value to MetaDataMap
    int readFileKV();


    void writeFileKV(std::pair <std::string, std::pair<MetaDataMap::MetaType_t, MetaDataMap::MetaValue_t>> p);
public:
    // WriteModel Init Function
    RecordMeta(std::unique_ptr<pse::ral::DirSectionInterface> &dir);
    explicit RecordMeta(const std::string& identifier);


    // ReadModel Init Function
    RecordMeta(std::unique_ptr<pse::ral::DirSectionInterface> &dir, bool isRead); // for write mode
    RecordMeta(const char *fileName);


    ~RecordMeta();


    void sectionStart(std::string);

    void sectionEnd(std::string);


    template<typename T>
    void metaRaw(const std::string &name, T val) {
        if (sectionList.empty()) {
            JSI_ERROR("section is empty");
        }
        auto m = this->metaMap[sectionList.top()];
        if (!m->set(name.c_str(), val)) {
            JSI_ERROR("metaRaw Exist same key:%s", name.c_str());
        }
    }

    void metaRawStore(std::string metaKey, void *p, size_t size);

    const std::unordered_map<std::string, MetaDataMap *> &getMetaMap() const;

    std::string to_string();
};


#endif //JSI_TOOLKIT_RECORD_META_H
