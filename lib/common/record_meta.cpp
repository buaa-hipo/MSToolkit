//
// Created by chengyanshang on 2022/10/15.
//

#include "record/record_meta.h"


MetaDataMap::MetaDataMap(std::string name) {
    this->sectionName = name;
}


const std::unordered_map <std::string, std::pair<MetaDataMap::MetaType_t, MetaDataMap::MetaValue_t>> &
MetaDataMap::getMetaMap() const {
    return _meta_map;
}

const std::string &MetaDataMap::getSectionName() const {
    return sectionName;
}


bool MetaDataMap::set(const char *name, const void *p, size_t size) {
    if (_meta_map.find(name) != _meta_map.end()) {
        return false;
    }
    MetaValue_t t;
    void *ptr = malloc(size);
    memcpy(ptr, p, size);
    t.raw.ptr = ptr;
    t.raw.size = size;
    this->_meta_map[name] = std::pair < MetaType_t, MetaValue_t > {RAW_PTR, t};
    return true;
}

bool MetaDataMap::set(const char *name, const char* val, size_t size) {
    if (_meta_map.find(name) != _meta_map.end()) {
        return false;
    }
    MetaValue_t t;
    t.ptr = (char *) malloc(size + 1);
    memcpy(t.ptr, val, size);
    t.ptr[size] = '\0';
    this->_meta_map[name] = std::pair < MetaType_t, MetaValue_t > {STRING, t};
    return true;
}

template<>
bool MetaDataMap::set(const char *name, bool val) {
    if (_meta_map.find(name) != _meta_map.end()) {
        return false;
    }
    MetaValue_t t;
    t.bl = val;
    this->_meta_map[name] = std::pair < MetaType_t, MetaValue_t > {BOOL, t};
    return true;
}

template<>
bool MetaDataMap::set(const char *name, int8_t val) {
    if (_meta_map.find(name) != _meta_map.end()) {
        return false;
    }
    MetaValue_t t;
    t.i64 = val;
    this->_meta_map[name] = std::pair < MetaType_t, MetaValue_t > {INT8, t};
    return true;
}


template<>
bool MetaDataMap::set(const char *name, int16_t val) {
    if (_meta_map.find(name) != _meta_map.end()) {
        return false;
    }
    MetaValue_t t;
    t.i16 = val;
    this->_meta_map[name] = std::pair < MetaType_t, MetaValue_t > {INT16, t};
    return true;

}

template<>
bool MetaDataMap::set(const char *name, int32_t val) {
    if (_meta_map.find(name) != _meta_map.end()) {
        return false;
    }
    MetaValue_t t;
    t.i32 = val;
    this->_meta_map[name] = std::pair < MetaType_t, MetaValue_t > {INT32, t};
    return true;

}

template<>
bool MetaDataMap::set(const char *name, int64_t val) {
    if (_meta_map.find(name) != _meta_map.end()) {
        return false;
    }
    MetaValue_t t;
    t.i64 = val;
    this->_meta_map[name] = std::pair < MetaType_t, MetaValue_t > {INT64, t};
    return true;

}


template<>
bool MetaDataMap::set(const char *name, float val) {
    if (_meta_map.find(name) != _meta_map.end()) {
        return false;
    }
    MetaValue_t t;
    t.flt = val;
    this->_meta_map[name] = std::pair < MetaType_t, MetaValue_t > {SP, t};
    return true;

}

template<>
bool MetaDataMap::set(const char *name, double val) {
    if (_meta_map.find(name) != _meta_map.end()) {
        return false;
    }
    MetaValue_t t;
    t.dp = val;
    this->_meta_map[name] = std::pair < MetaType_t, MetaValue_t > {DP, t};
    return true;
}

template<>
bool MetaDataMap::set(const char *name, std::string val) {
    if (_meta_map.find(name) != _meta_map.end()) {
        return false;
    }
    MetaValue_t t;
    t.ptr = (char *) malloc(val.size() + 1);
    memcpy(t.ptr, val.c_str(), val.size());
    t.ptr[val.size()] = '\0';
    this->_meta_map[name] = std::pair < MetaType_t, MetaValue_t > {STRING, t};
    return true;
}

template<>
bool MetaDataMap::set(const char *name, const char* val) {
    if (_meta_map.find(name) != _meta_map.end()) {
        return false;
    }
    MetaValue_t t;
    size_t len = strlen(val);
    t.ptr = (char *) malloc(len + 1);
    memcpy(t.ptr, val, len);
    t.ptr[len] = '\0';
    this->_meta_map[name] = std::pair < MetaType_t, MetaValue_t > {STRING, t};
    return true;
}

MetaDataMap::MetaType_t MetaDataMap::get(const char *name, MetaValue_t **t) {
    auto it = _meta_map.find(name);
    if (it == _meta_map.end()) {
        t = nullptr;
        return MetaDataMap::UNKNOW;
    }
    auto &p = it->second;
    *t = &p.second;
    return p.first;
}


// ---------------------------------------RecordMeta------------------------------
RecordMeta::RecordMeta(const std::string& identifier) {
    std::string metaFileName = getMetaFileName(identifier);
    const char *fn = metaFileName.c_str();
    FILE *file = fopen(fn, "w+");
    if (file == NULL) {
        JSI_ERROR("(id=%s): failed to open file: %s\n", identifier.c_str(), fn);
    }
    jsio metaIo;
    // jsioInitWithFile(&metaIo, file);
    // this->metaIO = metaIo;
    this->readOnly = false;
}

RecordMeta::RecordMeta(std::unique_ptr<pse::ral::DirSectionInterface> &dir, bool isRead) {
    auto stream_section = dir->openStreamSection(StaticSectionDesc::META_SEC_OFFSET, true);
    jsioInitWithSection(&this->metaIO, std::move(stream_section));
    this->readOnly = isRead;
}

RecordMeta::RecordMeta(std::unique_ptr<pse::ral::DirSectionInterface> &dir) {
    // 1. read all data from stream section
    auto stream_section = dir->openStreamSection(StaticSectionDesc::META_SEC_OFFSET, true);
    off_t size = stream_section->size();
    char* buf = new char[size];
    stream_section->read(buf, size);
    this->ptr = (void*) buf;
    
    // 2. load all data to memory,consider the data size is small
    off_t off = 0;
    while (off < size) {
        uint8_t type = *(uint8_t * )(ptr);
        ptr = (uint8_t *) ptr + 1;
        off += sizeof(uint8_t);
        switch (type) {
            case META_SECTION_NAME:
                off += readFileSection();
                break;
            case META_INFO:
                off += readFileKV();
                break;
        }
    }
    this->readOnly = true;
    delete [] buf;
}

RecordMeta::RecordMeta(const char *fn) {
    // 1. mmap reflect
    int fd = open(fn, O_RDONLY);
    off_t fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET); // reset
    void *trace_mmap = mmap(0, fsize, PROT_READ, MAP_PRIVATE/*may use MAP_SHARED?*/, fd, 0);
    this->ptr = trace_mmap;
    // 2. load all data to memory,consider the data size is small
    off_t off = 0;
    while (off < fsize) {
        uint8_t type = *(uint8_t * )(ptr);
        ptr = (uint8_t *) ptr + 1;
        off += sizeof(uint8_t);
        switch (type) {
            case META_SECTION_NAME:
                off += readFileSection();
                break;
            case META_INFO:
                off += readFileKV();
                break;
        }
    }
    close(fd);
    munmap(trace_mmap, fsize);
    this->readOnly = true;
}

RecordMeta::~RecordMeta() {
    // if(readOnly) {
    //     delete this->ptr;
    // }
}

int RecordMeta::readFileSection() {
    // 1. read section name size
    int mv = 0;
    size_t secNmSize = *(size_t * )(ptr);
    ptr = (size_t *) ptr + 1;
    mv += sizeof(size_t);
    // 2. read section name
    char *secName = (char *) malloc(secNmSize+1);
    memcpy(secName, ptr, secNmSize);
    secName[secNmSize]='\0';
    ptr = (char *) ptr + secNmSize;
    mv += secNmSize;
    // 3. init a new map
    this->metaMap[secName] = new MetaDataMap(secName);
    this->section = secName;
    free(secName);
    return mv;
}


int RecordMeta::readFileKV() {
    int mv = 0;
    // 0. read key name
    size_t size = *(size_t *) ptr;
    ptr = (size_t *) ptr + 1;
    char *name = (char *) malloc(size+1);
    memcpy(name, ptr, size);
    name[size]='\0';
    ptr = (uint8_t *) ptr + size;
    mv += sizeof(size_t) + size;
    // 1. read meta kv type
    uint8_t type = *(uint8_t * )(ptr);
    ptr = (uint8_t *) ptr + 1;
    mv += 1;
    // 2. read meta value by type
    switch (type) {
        case MetaDataMap::BOOL: {
            bool tbool = *(bool *) ptr;
            ptr = (bool *) ptr + 1;
            this->metaMap[section]->set<bool>(name, tbool);
            mv += sizeof(bool);
            break;
        }
        case MetaDataMap::INT8: {
            int8_t tInt8 = *(int8_t *) ptr;
            ptr = (int8_t *) ptr + 1;
            this->metaMap[section]->set<int8_t>(name, tInt8);
            mv += sizeof(int8_t);
            break;
        }
        case MetaDataMap::INT16: {
            int16_t tInt16 = *(int16_t *) ptr;
            ptr = (int16_t *) ptr + 1;
            this->metaMap[section]->set<int16_t>(name, tInt16);
            mv += sizeof(int16_t);
            break;
        }

        case MetaDataMap::INT32: {
            int32_t tInt32 = *(int32_t *) ptr;
            ptr = (int32_t *) ptr + 1;
            this->metaMap[section]->set<int32_t>(name, tInt32);
            mv += sizeof(int32_t);
            break;
        }

        case MetaDataMap::INT64: {
            int64_t tInt64 = *(int64_t *) ptr;
            ptr = (int64_t *) ptr + 1;
            this->metaMap[section]->set<int64_t>(name, tInt64);
            mv += sizeof(int64_t);
            break;
        }
        case MetaDataMap::SP: {
            float flt = *(float *) ptr;
            ptr = (float *) ptr + 1;
            this->metaMap[section]->set<float>(name, flt);
            mv += sizeof(float);
            break;
        }
        case MetaDataMap::DP: {
            double dp = *(float *) ptr;
            ptr = (double *) ptr + 1;
            this->metaMap[section]->set<double>(name, dp);
            mv += sizeof(double);
            break;
        }
        case MetaDataMap::RAW_PTR: {
            size = *(size_t *) ptr;
            ptr = (size_t *) ptr + 1;
            this->metaMap[section]->set(name, ptr, size);
            ptr = (uint8_t *) ptr + size;
            mv += size + sizeof(size_t);
            break;
        }
        case MetaDataMap::STRING: {
            size = *(size_t *) ptr;
            ptr = (size_t *) ptr + 1;
            this->metaMap[section]->set(name, (char *) ptr, size);
            ptr = (uint8_t *) ptr + size;
            mv += size + sizeof(size_t);
            break;
        }
    }
    free(name);
    return mv;
}


// init section  and new MetaDataMap
void RecordMeta::sectionStart(std::string name) {
    std::string nowSection = name;
    if (sectionList.empty()) {
        sectionList.push(nowSection);
    } else {
        std::string prev = sectionList.top();
        nowSection = prev + "/" + nowSection;
        sectionList.push(nowSection);
    }
    this->metaMap[nowSection] = new MetaDataMap(nowSection);
}


// pop section and flush to file
void RecordMeta::sectionEnd(std::string name) {
    if (sectionList.empty()) {
        JSI_ERROR("section is empty");
    }
    std::string endSection = sectionList.top();
    sectionList.pop();
    auto m = this->metaMap[endSection];
    // 获取整体需要分配的内存大小
    jsioWrite(&metaIO, &META_SECTION_NAME, 1);
    jsioWriteString(&metaIO, m->getSectionName());
    for (auto p: m->getMetaMap()) {
        writeFileKV(p);
    }
    this->metaMap.erase(endSection);
}

void RecordMeta::writeFileKV(std::pair <std::string, std::pair<MetaDataMap::MetaType_t, MetaDataMap::MetaValue_t>> p) {
    std::string key = p.first;
    MetaDataMap::MetaType_t vType = p.second.first;
    MetaDataMap::MetaValue_t value = p.second.second;
    int16_t valueSize = sizeof(value);
    jsioWrite(&metaIO, &META_INFO, 1);
    jsioWriteString(&metaIO, key);
    jsioWrite(&metaIO, &vType, 1);
    switch (vType) {
        case MetaDataMap::BOOL:
            jsioWrite(&metaIO, &value.bl, 1);
            break;
        case MetaDataMap::INT8:
            jsioWrite(&metaIO, &value.i8, 1);
            break;
        case MetaDataMap::INT16:
            jsioWrite(&metaIO, &value.i16, 2);
            break;
        case MetaDataMap::INT32:
            jsioWrite(&metaIO, &value.i32, 4);
            break;
        case MetaDataMap::INT64:
            jsioWrite(&metaIO, &value.i64, 8);
            break;
        case MetaDataMap::SP:
            jsioWrite(&metaIO, &value.flt, sizeof(float));
            break;
        case MetaDataMap::DP:
            jsioWrite(&metaIO, &value.dp, sizeof(double));
            break;
        case MetaDataMap::RAW_PTR:
            jsioWrite(&metaIO, &value.raw.size, sizeof(value.raw.size));
            jsioWrite(&metaIO, &value.raw.ptr, value.raw.size);
            free(value.raw.ptr);
            break;
        case MetaDataMap::STRING:
            jsioWriteString(&metaIO, value.ptr);
            free(value.ptr);
            break;
    }
}


void RecordMeta::metaRawStore(std::string name, void *p, size_t size) {
    if (sectionList.empty()) {
        JSI_ERROR("section is empty");
    }
    auto m = this->metaMap[sectionList.top()];
    if (!m->set(name.c_str(), p, size)) {
        JSI_ERROR("metaRaw Exist same key:%s", name.c_str());
    }
}

const std::unordered_map<std::string, MetaDataMap *> &RecordMeta::getMetaMap() const {
    return metaMap;
}

std::string MetaDataMap::to_string() {
    std::string buf("");
    for(const auto& pair: _meta_map) {
        buf += std::string("\t") + pair.first;
        switch(pair.second.first) {
            case _meta_type_t::BOOL:
                buf += std::string(" : ") + std::to_string(pair.second.second.bl);
                break;
            case _meta_type_t::INT8:
                buf += std::string(" : ") + std::to_string(pair.second.second.i8);
                break;
            case _meta_type_t::INT16:
                buf += std::string(" : ") + std::to_string(pair.second.second.i16);
                break;
            case _meta_type_t::INT32:
                buf += std::string(" : ") + std::to_string(pair.second.second.i32);
                break;
            case _meta_type_t::INT64:
                buf += std::string(" : ") + std::to_string(pair.second.second.i64);
                break;
            case _meta_type_t::SP:
                buf += std::string(" : ") + std::to_string(pair.second.second.flt);
                break;
            case _meta_type_t::DP:
                buf += std::string(" : ") + std::to_string(pair.second.second.dp);
                break;
            case _meta_type_t::STRING:
                buf += std::string(" : ") + std::string(pair.second.second.ptr);
                break;
            case _meta_type_t::RAW_PTR:
                buf += std::string(" : raw_ptr size=") + std::to_string(pair.second.second.raw.size);
                break;
            default:
                buf += std::string(" : Unknown");
                break;
        }
        buf += std::string("\n");
    }
    return buf;
}

std::string RecordMeta::to_string() {
    std::string buf("");
    for(const auto& pair: metaMap) {
        buf += std::string("Section ") + pair.first + std::string("\n");
        buf += pair.second->to_string();
    }
    return buf;
}