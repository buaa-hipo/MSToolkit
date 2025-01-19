#include "instrument/backtrace.h"

#include <dlfcn.h>

#include <climits>
#include <cstring>
#include <unordered_map>
#include <vector>

#ifdef USE_LIBUNWIND
/* Exciplitly use libunwind interface */
#include <libunwind.h>
#endif

#define MAX_FILE_PATH (200)

namespace {

class StringBuffer {
public:
    explicit StringBuffer(size_t size = BACKTRACE_STRING_BUFFER_DEFAULT_RESERVE_SIZE) {
        _capacity = std::max(size, (size_t) BACKTRACE_STRING_MAX);
        _data = new char[_capacity];
        _size = 0;
    }

    ~StringBuffer() {
        delete[] _data;
    }

    size_t append(const char *str, size_t len) {
        auto start_offset = _size;
        memcpy(&_data[_size], str, len);
        _data[_size + len] = '\0';
        _size += len + 1;
        return start_offset;
    }

    inline size_t can_append(size_t len) const {
        return len + 1 + _size <= _capacity;
    }

    inline const char *data() {
        return _data;
    }

    inline size_t size() const {
        return _size;
    }

    inline void clear() {
        _size = 0;
    }

private:
    char *_data;
    size_t _capacity;
    size_t _size;
};

size_t flush_on_excess(size_t len, StringBuffer &buf, FILE *file, off64_t &foff_buf) {
    if (buf.can_append(len)) {
        return 0;
    }

    int ret;
    auto foff = ftello64(file);
    if ((ret = fseeko64(file, foff_buf, SEEK_SET)) != 0) {
        JSI_ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
    }
    fwrite(buf.data(), buf.size(), 1, file);
    foff_buf = ftello64(file);
    if ((ret = fseeko64(file, foff, SEEK_SET)) != 0) {
        JSI_ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
    }

    auto flushed_size = buf.size();
    buf.clear();
    return flushed_size;
}

jsi::toolkit::backtrace_node_t create_backtrace_node(void *ip, backtrace_context_t p_ctx) {
    jsi::toolkit::backtrace_node_t bt_node;
    bt_node.parent_context = p_ctx;
    bt_node.ip = ip;

    Dl_info info;
    if (dladdr(ip, &info) == 0) {
        JSI_ERROR("Invalid instruction pointer address in backtrace info\n");
    }

    // bt_node.bin_filename = info.dli_fname;
    // bt_node.bin_base_addr = info.dli_fbase;
    // bt_node.bin_symbol_name = info.dli_sname;
    if(info.dli_fname) {
        size_t fname_len = strlen(info.dli_fname);
        // printf("fname size = %d\n", fname_len); fflush(stdout);
        bt_node.bin_filename = new char[fname_len+1];
        memcpy(bt_node.bin_filename, info.dli_fname, fname_len);
        bt_node.bin_filename[fname_len] = '\0';
    } else {
        bt_node.bin_filename = nullptr;
    }
    bt_node.bin_base_addr = info.dli_fbase;
    if(info.dli_sname) {
        size_t sname_len = strlen(info.dli_sname);
        // printf("sname size = %d\n", sname_len); fflush(stdout);
        bt_node.bin_symbol_name = new char[sname_len+1];
        memcpy(bt_node.bin_symbol_name, info.dli_sname, sname_len);
        bt_node.bin_symbol_name[sname_len] = '\0';
    } else {
        bt_node.bin_symbol_name = nullptr;
    }

    return bt_node;
}

jsi::toolkit::BacktraceTree *bt_tree = nullptr;

}// namespace

namespace jsi { namespace toolkit {

BacktraceTree::BacktraceTree(int max_bt_size, BacktraceTree::BacktraceMode mode) {
    _mode = mode;
    _max_bt_size = max_bt_size;
    _backtrace_buffer.resize(_max_bt_size);

    _bt_nodes.reserve(BACKTRACE_BUFFER_RESERVE_SIZE);
    _bt_nodes.push_back(backtrace_node_t{});// add root backtrace node
}

BacktraceTree::~BacktraceTree() {
    for (auto &node: _bt_nodes) {
        delete[] node.bt_node_string;
        for (auto &it: node.bt_context_string) {
            delete[] it.second;
        }
        if (_mode == BacktraceMode::RECORDING) {
            if(node.bin_filename) {
                delete[] node.bin_filename;
            }
            if(node.bin_symbol_name) {
                delete[] node.bin_symbol_name;
            }
            delete[] node.src_filename;
            delete[] node.src_funcname;
        }
        if (_mode == BacktraceMode::LOADING) {
            delete[] node.src_filename;
            delete[] node.src_funcname;
        }
    }
    delete[] _loaded_string_buffer;
}

BacktraceTree *BacktraceTree::create_recording_bt_tree(int max_bt_size) {
    return new BacktraceTree(max_bt_size, BacktraceMode::RECORDING);
}

BacktraceTree *BacktraceTree::create_loading_bt_tree() {
    return new BacktraceTree(0, BacktraceMode::LOADING);
}

backtrace_context_t BacktraceTree::backtrace_context_get(int omit_level) {
    if (_mode != BacktraceMode::RECORDING) {
        JSI_ERROR("Trying to get context in non-recording mode, aborting...\n");
    }

    int _max_bt_size_act = _max_bt_size+omit_level+BACKTRACE_LIBJSI_RECORD_LEVELS;
    _backtrace_buffer.resize(_max_bt_size_act);
#ifdef USE_LIBUNWIND
    int bt_size = unw_backtrace(_backtrace_buffer.data(), _max_bt_size_act);
#else
    int bt_size = backtrace(_backtrace_buffer.data(), _max_bt_size_act);
#endif

    backtrace_context_t ctx = BACKTRACE_ROOT_NODE;
    int bt_ptr;

    // walk through existing backtrace _nodes
    int _inner_level = BACKTRACE_LIBJSI_RECORD_LEVELS + omit_level;
    for (bt_ptr = bt_size - 1; bt_ptr >= _inner_level; bt_ptr--) {
        void *ip = _backtrace_buffer[bt_ptr];
#ifdef __aarch64__
        ip = reinterpret_cast<char *>(ip) - 4;
#endif
        auto it = _bt_nodes[ctx].children_contexts.find(ip);
        if (it == _bt_nodes[ctx].children_contexts.end()) {
            break;
        }
        ctx = it->second;
    }

    // create new _nodes if not exist
    for (; bt_ptr >= _inner_level; bt_ptr--) {
        void *ip = _backtrace_buffer[bt_ptr];
#ifdef __aarch64__
        ip = reinterpret_cast<char *>(ip) - 4;
#endif
        auto bt_node = create_backtrace_node(ip, ctx);
        _bt_nodes.push_back(bt_node);
        auto child_ctx = (int32_t) _bt_nodes.size() - 1;
        _bt_nodes[ctx].children_contexts[ip] = child_ctx;
        ctx = child_ctx;
    }

    return ctx;
}

void BacktraceTree::backtrace_db_dump(std::unique_ptr<pse::ral::DirSectionInterface> &dir) {
    // if (_mode != BacktraceMode::RECORDING) {
    //     JSI_WARN("Trying to dump backtrace db in loading mode, ignoring...\n");
    //     return;
    // }

    uint64_t len = _bt_nodes.size() - 1;// Fixed 64-bit
    // Open a data section for node data
    auto node_data_sec = dir->openDataSection<backtrace_node_record_t>(StaticSectionDesc::BACKTRACE_NODE_SEC, true, 0, 0);
    // Open a string section for string data
    auto string_sec = dir->openStringSection(StaticSectionDesc::BACKTRACE_STRING_SEC, true);
    
    for (backtrace_context_t i = 1; i <= len; i++) {
        auto &node = _bt_nodes[i];

        backtrace_node_record_t record;
        record.ip = reinterpret_cast<uint64_t>(node.ip);
        record.parent_context = static_cast<int64_t>(node.parent_context);
        record.faddr = reinterpret_cast<uint64_t>(node.bin_base_addr);

        auto fname = node.bin_filename;
        if (fname != nullptr) {
            auto offset = string_sec->write(fname);
            record.fname_offset = offset;
        } else {
            record.fname_offset = -1;
        }

        auto sname = node.bin_symbol_name;
        if (sname != nullptr) {
            auto offset = string_sec->write(sname);
            record.sname_offset = offset;
        } else {
            record.sname_offset = -1;
        }

        auto sfilename = node.src_filename;
        if (sfilename != nullptr) {
            auto offset = string_sec->write(sfilename);
            record.sfilename_offset = offset;
        } else {
            record.sfilename_offset = -1;
        }

        auto sfuncname = node.src_funcname;
        if (sfuncname != nullptr) {
            auto offset = string_sec->write(sfuncname);
            record.sfuncname_offset = offset;
        } else {
            record.sfuncname_offset = -1;
        }

        record.lineno = node.lineno;

        node_data_sec->write(&record);
    }
}

void BacktraceTree::backtrace_db_dump(FILE *file) {
    if (_mode != BacktraceMode::RECORDING) {
        JSI_WARN("Trying to dump backtrace db in loading mode, ignoring...\n");
        return;
    }

    int ret;
    StringBuffer buf;
    size_t offset = 0;
    uint64_t len = _bt_nodes.size() - 1;// Fixed 64-bit
    fwrite(&len, sizeof(uint64_t), 1, file);

    auto foff_records = ftello64(file);
    auto record_size = sizeof(backtrace_node_record_t) * (_bt_nodes.size() - 1);
    if ((ret = fseeko64(file, record_size + sizeof(uint64_t), SEEK_CUR)) !=
        0) {// uint64_t for string buf size
        JSI_ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
    }
    auto foff_buf = ftello64(file);

    if ((ret = fseeko64(file, foff_records, SEEK_SET)) != 0) {
        JSI_ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
    }
    for (backtrace_context_t i = 1; i <= len; i++) {
        auto &node = _bt_nodes[i];

        backtrace_node_record_t record;
        record.ip = reinterpret_cast<uint64_t>(node.ip);
        record.parent_context = static_cast<int64_t>(node.parent_context);
        record.faddr = reinterpret_cast<uint64_t>(node.bin_base_addr);

        auto fname = node.bin_filename;
        if (fname != nullptr) {
            const auto fname_len = strlen(fname);
            offset += flush_on_excess(fname_len, buf, file, foff_buf);
            record.fname_offset = offset + buf.append(fname, fname_len);
        } else {
            record.fname_offset = -1;
        }

        auto sname = node.bin_symbol_name;
        if (sname != nullptr) {
            const auto sname_len = strlen(sname);
            offset += flush_on_excess(sname_len, buf, file, foff_buf);
            record.sname_offset = offset + buf.append(sname, sname_len);
        } else {
            record.sname_offset = -1;
        }

        auto sfilename = node.src_filename;
        if (sfilename != nullptr) {
            const auto sfilename_len = strlen(sfilename);
            offset += flush_on_excess(sfilename_len, buf, file, foff_buf);
            record.sfilename_offset = offset + buf.append(sfilename, sfilename_len);
        } else {
            record.sfilename_offset = -1;
        }

        auto sfuncname = node.src_funcname;
        if (sfuncname != nullptr) {
            const auto sfuncname_len = strlen(sfuncname);
            offset += flush_on_excess(sfuncname_len, buf, file, foff_buf);
            record.sfuncname_offset = offset + buf.append(sfuncname, sfuncname_len);
        } else {
            record.sfuncname_offset = -1;
        }

        record.lineno = node.lineno;

        fwrite(&record, sizeof(backtrace_node_record_t), 1, file);
    }

    // Write string buf size
    uint64_t buf_size = offset + buf.size();
    fwrite(&buf_size, sizeof(uint64_t), 1, file);

    // Write remaining string buf
    fseeko64(file, foff_buf, SEEK_SET);
    fwrite(buf.data(), buf.size(), 1, file);
}

void BacktraceTree::backtrace_db_load(std::unique_ptr<pse::ral::DirSectionInterface> &dir, const char *dwarf_dir, bool enable_dbinfo) {
    if (_mode != BacktraceMode::LOADING) {
        JSI_WARN("Trying to load backtrace db in recording mode, ignoring...\n");
        return;
    }
    
    auto lineinfo_db_load = LineInfoDB::create_loading_db();
    if (dwarf_dir) {
        lineinfo_db_load->lineinfo_load(dwarf_dir);
    }

    auto node_data_sec = dir->openDataSection<backtrace_node_record_t>(StaticSectionDesc::BACKTRACE_NODE_SEC, false, 0, 0);
    auto len = node_data_sec->size();
    auto string_sec = dir->openStringSection(StaticSectionDesc::BACKTRACE_STRING_SEC, false);
    _bt_nodes.resize(len + 1);
    delete[] _loaded_string_buffer;
    auto total_length = string_sec->total_length();
    _loaded_string_buffer = new char[total_length];
    int buf_offset = 0;

    // Root node
    _bt_nodes[0].parent_context = BACKTRACE_UNKNOWN_NODE;

    if (dwarf_dir) {
        std::unordered_map<std::string, std::unordered_map<uint64_t, Addr2LineInfo>>
                addr2line_info_cache;
        for (backtrace_context_t i = 1; i <= len; i++) {
            backtrace_node_record_t record;
            node_data_sec->read(&record, i - 1);
            
            auto &node = _bt_nodes[i];
            node.ip = reinterpret_cast<void *>(record.ip);
            node.bin_base_addr = reinterpret_cast<void *>(record.faddr);

            node.parent_context = static_cast<backtrace_context_t>(record.parent_context);

            if (record.fname_offset == -1) {
                node.bin_filename = nullptr;
            } else {
                auto ret_size = string_sec->read(_loaded_string_buffer + buf_offset, record.fname_offset, total_length - buf_offset);
                _loaded_string_buffer[buf_offset + ret_size] = '\0';
                node.bin_filename = _loaded_string_buffer + buf_offset;
                buf_offset += ret_size + 1;
            }
            if (record.sname_offset== -1) {
                node.bin_symbol_name = nullptr;
            } else {
                auto ret_size = string_sec->read(_loaded_string_buffer + buf_offset, record.sname_offset, total_length - buf_offset);
                _loaded_string_buffer[buf_offset + ret_size] = '\0';
                node.bin_symbol_name = _loaded_string_buffer + buf_offset;
                buf_offset += ret_size + 1;
            }
#ifdef KYLIN
            uint64_t pc = (uint64_t) node.ip - (uint64_t) node.bin_base_addr + 0x400000;
#else
            uint64_t pc = (uint64_t) node.ip - (uint64_t) node.bin_base_addr;
#endif
            if (enable_dbinfo) {
                Addr2LineInfo *info = &addr2line_info_cache[node.bin_filename][pc];
                if (info->filename == nullptr) {
                    info = new Addr2LineInfo();
                    info->filename = new char[MAX_PATH_SIZE];
                    info->funcname = new char[MAX_PATH_SIZE];
                    lineinfo_db_load->addr2line(node.bin_filename, pc, info);
                }
                node.src_filename = info->filename;
                node.src_funcname = info->funcname;
                node.lineno = info->lineno;
            }
        }
    } else {
        std::unordered_map<std::string, std::unordered_map<uint64_t, Addr2LineInfo>>
                addr2line_info_cache;
        for (backtrace_context_t i = 1; i <= len; i++) {
            backtrace_node_record_t record;
            node_data_sec->read(&record, i - 1);

            auto &node = _bt_nodes[i];
            node.ip = reinterpret_cast<void *>(record.ip);
            node.bin_base_addr = reinterpret_cast<void *>(record.faddr);

            node.parent_context = static_cast<backtrace_context_t>(record.parent_context);

            if (record.fname_offset == -1) {
                node.bin_filename = nullptr;
            } else {
                auto ret_size = string_sec->read(_loaded_string_buffer + buf_offset, record.fname_offset, total_length - buf_offset);
                _loaded_string_buffer[buf_offset + ret_size] = '\0';
                node.bin_filename = _loaded_string_buffer + buf_offset;
                buf_offset += ret_size + 1;
            }
            if (record.sname_offset== -1) {
                node.bin_symbol_name= nullptr;
            } else {
                auto ret_size = string_sec->read(_loaded_string_buffer + buf_offset, record.sname_offset, total_length - buf_offset);
                _loaded_string_buffer[buf_offset + ret_size] = '\0';
                node.bin_symbol_name= _loaded_string_buffer + buf_offset;
                buf_offset += ret_size + 1;
            }

#ifdef KYLIN
            uint64_t pc = (uint64_t) node.ip - (uint64_t) node.bin_base_addr + 0x400000;
#else
            uint64_t pc = (uint64_t) node.ip - (uint64_t) node.bin_base_addr;
#endif
            if (enable_dbinfo) {
                Addr2LineInfo *info = &addr2line_info_cache[node.bin_filename][pc];
                if (info->filename == nullptr) {
                    //info->filename = new char[MAX_PATH_SIZE];
                    //info->funcname = new char[MAX_PATH_SIZE];
                    addr2line(node.bin_filename, pc, info);
                }

                node.src_filename = info->filename;
                node.src_funcname = info->funcname;
                node.lineno = info->lineno;
            }
        }
    }

    delete lineinfo_db_load;
}

void BacktraceTree::backtrace_db_load(FILE *file, const char *dwarf_dir, bool enable_dbinfo) {
    if (_mode != BacktraceMode::LOADING) {
        JSI_WARN("Trying to load backtrace db in recording mode, ignoring...\n");
        return;
    }
    
    auto lineinfo_db_load = LineInfoDB::create_loading_db();
    if (dwarf_dir) {
        lineinfo_db_load->lineinfo_load(dwarf_dir);
    }

    int ret;
    uint64_t len;
    if (fread(&len, sizeof(uint64_t), 1, file) != 1) {
        JSI_ERROR("Error loading backtrace database: the db size cannot be determined\n");
    }
    _bt_nodes.resize(len + 1);

    auto foff_records = ftello64(file);
    auto record_size = sizeof(backtrace_node_record_t) * len;
    if ((ret = fseeko64(file, record_size, SEEK_CUR)) != 0) {
        JSI_ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
    }

    uint64_t buf_len;
    if (fread(&buf_len, sizeof(uint64_t), 1, file) != 1) {
        JSI_ERROR(
                "Error loading backtrace database: the string buf size cannot be "
                "determined.\n");
    }

    delete[] _loaded_string_buffer;
    _loaded_string_buffer = new char[buf_len];
    if (fread(_loaded_string_buffer, 1, buf_len, file) != buf_len) {
        JSI_ERROR("Error loading backtrace database: string buf truncated.\n");
    }

    if ((ret = fseeko64(file, foff_records, SEEK_SET)) != 0) {
        JSI_ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
    }

    // Root node
    _bt_nodes[0].parent_context = BACKTRACE_UNKNOWN_NODE;

    if (dwarf_dir) {
        std::unordered_map<std::string, std::unordered_map<uint64_t, Addr2LineInfo>>
                addr2line_info_cache;
        for (backtrace_context_t i = 1; i <= len; i++) {
            backtrace_node_record_t record;
            if (fread(&record, sizeof(backtrace_node_record_t), 1, file) != 1) {
                JSI_ERROR("Error loading backtrace database: record array truncated.\n");
            }
            auto &node = _bt_nodes[i];
            node.ip = reinterpret_cast<void *>(record.ip);
            node.bin_base_addr = reinterpret_cast<void *>(record.faddr);

            node.parent_context = static_cast<backtrace_context_t>(record.parent_context);

            node.bin_filename = (record.fname_offset == -1)
                                        ? nullptr
                                        : _loaded_string_buffer + record.fname_offset;
            node.bin_symbol_name = (record.sname_offset == -1)
                                           ? nullptr
                                           : _loaded_string_buffer + record.sname_offset;
#ifdef KYLIN
            uint64_t pc = (uint64_t) node.ip - (uint64_t) node.bin_base_addr + 0x400000;
#else
            uint64_t pc = (uint64_t) node.ip - (uint64_t) node.bin_base_addr;
#endif
            if (enable_dbinfo) {
                Addr2LineInfo *info = &addr2line_info_cache[node.bin_filename][pc];
                if (info->filename == nullptr) {
                    info = new Addr2LineInfo();
                    info->filename = new char[MAX_PATH_SIZE];
                    info->funcname = new char[MAX_PATH_SIZE];
                    lineinfo_db_load->addr2line(node.bin_filename, pc, info);
                }
                node.src_filename = info->filename;
                node.src_funcname = info->funcname;
                node.lineno = info->lineno;
            }
        }
    } else {
        std::unordered_map<std::string, std::unordered_map<uint64_t, Addr2LineInfo>>
                addr2line_info_cache;
        for (backtrace_context_t i = 1; i <= len; i++) {
            backtrace_node_record_t record;
            if (fread(&record, sizeof(backtrace_node_record_t), 1, file) != 1) {
                JSI_ERROR("Error loading backtrace database: record array truncated.\n");
            }
            auto &node = _bt_nodes[i];
            node.ip = reinterpret_cast<void *>(record.ip);
            node.bin_base_addr = reinterpret_cast<void *>(record.faddr);

            node.parent_context = static_cast<backtrace_context_t>(record.parent_context);

            node.bin_filename = (record.fname_offset == -1)
                                        ? nullptr
                                        : _loaded_string_buffer + record.fname_offset;
            node.bin_symbol_name = (record.sname_offset == -1)
                                           ? nullptr
                                           : _loaded_string_buffer + record.sname_offset;

#ifdef KYLIN
            uint64_t pc = (uint64_t) node.ip - (uint64_t) node.bin_base_addr + 0x400000;
#else
            uint64_t pc = (uint64_t) node.ip - (uint64_t) node.bin_base_addr;
#endif
            if (enable_dbinfo) {
                Addr2LineInfo *info = &addr2line_info_cache[node.bin_filename][pc];
                if (info->filename == nullptr) {
                    //info->filename = new char[MAX_PATH_SIZE];
                    //info->funcname = new char[MAX_PATH_SIZE];
                    addr2line(node.bin_filename, pc, info);
                }

                node.src_filename = info->filename;
                node.src_funcname = info->funcname;
                node.lineno = info->lineno;
            }
        }
    }
    delete lineinfo_db_load;
}

backtrace_context_t BacktraceTree::get_parent(backtrace_context_t ctx) const {
    if (ctx < 0 || ctx >= _bt_nodes.size()) {
        return BACKTRACE_UNKNOWN_NODE;
    }
    return _bt_nodes[ctx].parent_context;
}

void BacktraceTree::backtrace_get_context_string_vec(backtrace_context_t ctx, int size,
                                                     std::vector<const char *> &vec) {
    for (int i = 0; (size == -1 || i < size) && ctx > 0; i++, ctx = _bt_nodes[ctx].parent_context) {
        auto *&str = _bt_nodes[ctx].bt_node_string;
        if (str == nullptr) {
            str = new char[BACKTRACE_STRING_MAX];
            auto &node = _bt_nodes[ctx];
            auto ret =
                    // snprintf(str, BACKTRACE_STRING_MAX, "%s(+%" PRIxPTR ") [%p]", node.bin_filename,
                    //          reinterpret_cast<uintptr_t>(node.ip) -
                    //                  reinterpret_cast<uintptr_t>(node.bin_base_addr),
                    //          node.ip);
                    snprintf(str, BACKTRACE_STRING_MAX, "%s(+%" PRIxPTR ") (%p):%s [%s:%s:%d]",
                             node.bin_filename,
                             reinterpret_cast<uintptr_t>(node.ip) -
                                     reinterpret_cast<uintptr_t>(node.bin_base_addr),
                             node.ip, node.bin_symbol_name,
                             node.src_funcname ? node.src_funcname : "??",
                             node.src_filename ? node.src_filename : "??", node.lineno);
            if (ret < 0) {
                JSI_ERROR("Encoding error.\n");
            } else if (ret >= BACKTRACE_STRING_MAX) {
                JSI_WARN(
                        "Unexpected too long backtrace context string, truncated to "
                        "the "
                        "first "
                        "%d characters.\n",
                        BACKTRACE_STRING_MAX - 1);
            }
        }

        vec.push_back(str);
    }
}

const char *BacktraceTree::backtrace_get_context_string(backtrace_context_t ctx) {
    return backtrace_get_context_string(ctx, -1);
}

const char *BacktraceTree::backtrace_get_context_string(backtrace_context_t ctx, int size) {
    if (ctx >= _bt_nodes.size()) {
        JSI_ERROR("Invalid backtrace context: %d >= %ld\n", ctx, _bt_nodes.size());
    }
    auto it = _bt_nodes[ctx].bt_context_string.find(size);
    auto ie = _bt_nodes[ctx].bt_context_string.end();
    if (it == ie) {
        std::vector<const char *> vec;
        backtrace_get_context_string_vec(ctx, size, vec);
        size_t len = 1;
        for (const auto &node_str: vec) {
            len += strlen(node_str) + 1;
        }
        char *p = new char[len];
        char *tmp = p;
        for (const auto &node_str: vec) {
            size_t level_str_len = strlen(node_str);
            memcpy(p, node_str, level_str_len);
            p += level_str_len;
            *p = '\n';
            p += 1;
        }
        *p = '\0';
        _bt_nodes[ctx].bt_context_string[size] = tmp;
        return tmp;
    }
    return it->second;
}

const char *BacktraceTree::backtrace_get_node_string(backtrace_context_t ctx) {
    auto *&str = _bt_nodes[ctx].bt_node_string;
    if (str == nullptr) {
        str = new char[BACKTRACE_STRING_MAX];
        auto &node = _bt_nodes[ctx];
        auto ret =
                // snprintf(str, BACKTRACE_STRING_MAX, "%s(+%" PRIxPTR ") [%p]", node.bin_filename,
                //          reinterpret_cast<uintptr_t>(node.ip) -
                //                  reinterpret_cast<uintptr_t>(node.bin_base_addr),
                //          node.ip);
                snprintf(str, BACKTRACE_STRING_MAX, "%s(+%" PRIxPTR ") (%p):%s [%s:%s:%d]", node.bin_filename,
                         reinterpret_cast<uintptr_t>(node.ip) -
                                 reinterpret_cast<uintptr_t>(node.bin_base_addr),
                         node.ip,
                         node.bin_symbol_name,
                         node.src_funcname?node.src_funcname:"??",
                         node.src_filename?node.src_filename:"??",
                         node.lineno);
        if (ret < 0) {
            JSI_ERROR("Encoding error.\n");
        } else if (ret >= BACKTRACE_STRING_MAX) {
            JSI_WARN(
                    "Unexpected too long backtrace context string, truncated to "
                    "the "
                    "first "
                    "%d characters.\n",
                    BACKTRACE_STRING_MAX - 1);
        }
    }
    return str;
}

bool BacktraceTree::backtrace_is_equal(backtrace_context_t ctx1, const BacktraceTree &tree1,
                                       backtrace_context_t ctx2, const BacktraceTree &tree2) {
    while (ctx1 != BACKTRACE_ROOT_NODE && ctx2 != BACKTRACE_ROOT_NODE) {
        const auto &node1 = tree1._bt_nodes[ctx1];
        const auto &node2 = tree1._bt_nodes[ctx2];
        if (node1.ip != node2.ip) {
            return false;
        }
        ctx1 = node1.parent_context;
        ctx2 = node2.parent_context;
    }
    return (ctx1 == ctx2);// ctx1 == ctx2 == BACKTRACE_ROOT_NODE
}

void *BacktraceTree::backtrace_context_ip(backtrace_context_t ctx) const {
    return _bt_nodes[ctx].ip;
}

}}// namespace jsi::toolkit

void backtrace_init_recording(int max_bt_size) {
    if (bt_tree != nullptr) {
        JSI_WARN("Backtrace is already initialized. This initialization will be ignored.");
        return;
    }
    bt_tree = jsi::toolkit::BacktraceTree::create_recording_bt_tree(max_bt_size);
}

void backtrace_finalize() {
    if (bt_tree != nullptr) {
        delete bt_tree;
        bt_tree = nullptr;
    }
}

backtrace_context_t backtrace_context_get(int omit_level) {
    if (bt_tree == nullptr) {
        JSI_ERROR("Backtrace must be initialized by calling backtrace_init first.");
    }
    return bt_tree->backtrace_context_get(omit_level);
}

void backtrace_db_dump(FILE *file) {
    if (bt_tree == nullptr) {
        JSI_ERROR("Backtrace must be initialized by calling backtrace_init first.");
    }
    bt_tree->backtrace_db_dump(file);
}

void backtrace_db_dump(std::unique_ptr<pse::ral::DirSectionInterface> &dir) {
    if (bt_tree == nullptr) {
        JSI_ERROR("Backtrace must be initialized by calling backtrace_init first.");
    }
    bt_tree->backtrace_db_dump(dir);
}

backtrace_context_t backtrace_context_get_parent(backtrace_context_t bt_ctxt) {
    if (bt_tree == nullptr) {
        JSI_ERROR("Backtrace must be initialized by calling backtrace_init first.");
    }
    return bt_tree->get_parent(bt_ctxt);
}

void backtrace_context_print(backtrace_context_t bt_ctxt, int size) {
    if (bt_tree == nullptr) {
        JSI_ERROR("Backtrace must be initialized by calling backtrace_init first.");
    }
    puts(backtrace_context_string(bt_ctxt, size));
}

const char *backtrace_context_string(backtrace_context_t bt_ctxt, int size) {
    if (bt_tree == nullptr) {
        JSI_ERROR("Backtrace must be initialized by calling backtrace_init first.");
    }
    return bt_tree->backtrace_get_context_string(bt_ctxt, size);
}

bool backtrace_is_same(backtrace_context_t c1, backtrace_context_t c2) {
    return c1 == c2;
}

bool backtrace_is_same_parent(backtrace_context_t c1, backtrace_context_t c2) {
    if (backtrace_context_get_parent(c1) == BACKTRACE_UNKNOWN_NODE ||
        backtrace_context_get_parent(c2) == BACKTRACE_UNKNOWN_NODE) {
        return false;
    }
    return backtrace_context_get_parent(c1) == backtrace_context_get_parent(c2);
}

void *backtrace_context_get_ip(backtrace_context_t bt_ctxt) {
    if (bt_tree == nullptr) {
        JSI_ERROR("Backtrace must be initialized by calling backtrace_init first.");
    }
    return bt_tree->backtrace_context_ip(bt_ctxt);
}

// no need to track CCT with backtrace when only requried for callsite info.
uint64_t backtrace_get_callsite() {
    void* buff[BACKTRACE_LIBJSI_RECORD_LEVELS+1];
#ifdef USE_LIBUNWIND
    int bt_size = unw_backtrace(buff, BACKTRACE_LIBJSI_RECORD_LEVELS+1);
#else
    int bt_size = backtrace(buff, BACKTRACE_LIBJSI_RECORD_LEVELS+1);
#endif

    return  reinterpret_cast<uint64_t>(buff[BACKTRACE_LIBJSI_RECORD_LEVELS]);
}