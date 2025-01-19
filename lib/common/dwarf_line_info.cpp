#include "utils/dwarf_line_info.h"

#include <filesystem>
namespace fs = std::filesystem;

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void LineInfoDB::addr2line(const char *objfile, uint64_t pc, Addr2LineInfo *info) {
    if (_mode != DBMode::LOADING) {
        ERROR("Trying to load line info db in recording mode, ignoring...\n");
        return;
    }

    bool is_found = false;
    bool found = (reclist.find(objfile) != reclist.end());
    if (!found) {
        for (auto &entry: reclist) {
            fs::path p1 = entry.first.c_str();
            fs::path p2 = objfile;
            if (entry.first.c_str() == objfile || (fs::exists(p1) && fs::equivalent(p1, p2))) {
                found = true;
                reclist[std::string(objfile)] = entry.second;
                break;
            }
        }
    }
    if (found) {
        for (auto &rec: reclist[objfile]) {
            if ((rec.low_addr <= pc && pc < rec.high_addr) || (rec.low_addr <= pc+0x400000 && pc+0x400000 < rec.high_addr)) {

                // if(strcmp(rec.funcname,"??") == 0) { printf("%s\n", rec.funcname); continue; }
                // if(rec.high_addr - rec.low_addr > min_gap) continue;
                // min_gap = rec.high_addr - rec.low_addr;
                // printf("pc = %lx, low_addr = %lx, high_addr = %lx\n", pc + 0x400000, rec.low_addr,rec.high_addr);
                is_found = true;
                strncpy(info->funcname, rec.funcname ? rec.funcname : "??", MAX_PATH_SIZE);
                strncpy(info->filename, rec.filename ? rec.filename : "??", MAX_PATH_SIZE);
                info->lineno = rec.lineno;
                // return;
            }
        }
    }
    if (!is_found) {
        strncpy(info->funcname, "??", MAX_PATH_SIZE);
        strncpy(info->filename, "??", MAX_PATH_SIZE);
        info->lineno = 0;
    }
}

class StringBuffer {
public:
    explicit StringBuffer(size_t size = LINEINFO_STRING_BUFFER_DEFAULT_RESERVE_SIZE) {
        _capacity = std::max(size, (size_t) LINEINFO_STRING_MAX);
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
        ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
    }
    fwrite(buf.data(), buf.size(), 1, file);
    foff_buf = ftello64(file);
    if ((ret = fseeko64(file, foff, SEEK_SET)) != 0) {
        ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
    }

    auto flushed_size = buf.size();
    buf.clear();
    return flushed_size;
}

LineInfoDB::LineInfoDB(LineInfoDB::DBMode mode) {
    _mode = mode;
    reclist.reserve(LINEINFO_BUFFER_RESERVE_SIZE);
}

LineInfoDB::~LineInfoDB() {
    for (auto &buf: _loaded_string_buffer) delete[] buf;
}

// TODO: move dwarf operation in main to here.
LineInfoDB *LineInfoDB::create_recording_db() {
    return new LineInfoDB(DBMode::RECORDING);
}

LineInfoDB *LineInfoDB::create_loading_db() {
    return new LineInfoDB(DBMode::LOADING);
}

void LineInfoDB::lineinfo_dump(const char *objfile, const char *outfile) {
    if (_mode != DBMode::RECORDING) {
        ERROR("Trying to get line info in non-recording mode, aborting...\n");
    }
    FILE *file = fopen(outfile, "wb");
    // std::cout << outfile << std::endl;

    fwrite(objfile, MAX_PATH_SIZE, 1, file);
    int ret;
    StringBuffer buf;
    size_t offset = 0;
    //    uint64_t len = info2AddrsRange.size();// Fixed 64-bit
    uint64_t len = 0;
    for (const auto &it: lineAddrRanges) {
        len += it.second.size();
    }

    fwrite(&len, sizeof(uint64_t), 1, file);

    auto foff_records = ftello64(file);
//    auto record_size = sizeof(lineInfo2AddrsRange) * len;
    auto record_size = sizeof(lineInfo2AddrsRange) * len;
    if ((ret = fseeko64(file, record_size + sizeof(uint64_t), SEEK_CUR)) !=
        0) {// uint64_t for string buf size
        ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
    }
    auto foff_buf = ftello64(file);

    if ((ret = fseeko64(file, foff_records, SEEK_SET)) != 0) {
        ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
    }

    for (const auto &it: lineAddrRanges) {
        for (const auto &it2 : it.second) {
            auto &lar = it2.second;
            lineInfo2AddrsRange record;

            record.lineno = lar.lineno;
            record.low_addr = lar.lo_addr;
            record.high_addr = lar.hi_addr;

            const auto filename_len = lar.filename.size();
            offset += flush_on_excess(filename_len, buf, file, foff_buf);
            record.filename_offset = offset + buf.append(lar.filename.c_str(), filename_len);

            auto *funcname = lar.funcname;
            if (funcname != nullptr) {
                const auto funcname_len = strlen(funcname);
                offset += flush_on_excess(funcname_len, buf, file, foff_buf);
                record.funcname_offset = offset + buf.append(funcname, funcname_len);
            } else {
                record.funcname_offset = -1;
            }
            fwrite(&record, sizeof(lineInfo2AddrsRange), 1, file);
        }

    }

    // Write string buf size
    uint64_t buf_size = offset + buf.size();
    fwrite(&buf_size, sizeof(uint64_t), 1, file);

    // Write remaining string buf
    fseeko64(file, foff_buf, SEEK_SET);
    fwrite(buf.data(), buf.size(), 1, file);
    fclose(file);
}

//TODO: load dir
void LineInfoDB::lineinfo_load(const char *dump_dir) {
    if (_mode != DBMode::LOADING) {
        ERROR("Trying to load line info db in recording mode, ignoring...\n");
        return;
    }
    for (const auto &dirEntry: fs::directory_iterator(dump_dir)) {
        if (dirEntry.is_regular_file()) {
            const auto &path = dirEntry.path();
            // std::cout << path << std::endl;
            FILE *file = fopen(path.c_str(), "rb");
            int ret;

            char *objfile = new char[MAX_PATH_SIZE];
            if (fread(objfile, MAX_PATH_SIZE, 1, file) != 1) {
                ERROR("Error loading line info database: the db size cannot be determined\n");
            }
            // printf("%s\n", objfile);

            uint64_t len;
            if (fread(&len, sizeof(uint64_t), 1, file) != 1) {
                ERROR("Error loading line info database: the db size cannot be determined\n");
            }
            reclist[objfile].resize(len);

            auto foff_records = ftello64(file);
            auto record_size = sizeof(lineInfo2AddrsRange) * len;
            if ((ret = fseeko64(file, record_size, SEEK_CUR)) != 0) {
                ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
            }

            uint64_t buf_len;
            if (fread(&buf_len, sizeof(uint64_t), 1, file) != 1) {
                ERROR("Error loading line info database: the string buf size cannot be "
                      "determined.\n");
            }

            char *loaded_string_buffer = new char[buf_len];
            _loaded_string_buffer.push_back(loaded_string_buffer);
            if (fread(loaded_string_buffer, 1, buf_len, file) != buf_len) {
                ERROR("Error loading line info database: string buf truncated.\n");
            }

            if ((ret = fseeko64(file, foff_records, SEEK_SET)) != 0) {
                ERROR("Dump error with fseeko64 returning %d. Aborting...\n", ret);
            }

            for (uint64_t i = 0; i < len; i++) {
                lineInfo2AddrsRange record;
                if (fread(&record, sizeof(lineInfo2AddrsRange), 1, file) != 1) {
                    ERROR("Error loading line info database: record array truncated.\n");
                }
                auto &node = reclist[objfile][i];
                node.lineno = record.lineno;
                node.low_addr = record.low_addr;
                node.high_addr = record.high_addr;

                node.filename = (record.filename_offset == -1)
                                        ? nullptr
                                        : loaded_string_buffer + record.filename_offset;
                node.funcname = (record.funcname_offset == -1)
                                        ? nullptr
                                        : loaded_string_buffer + record.funcname_offset;
                // printf("low_addr = %lx, high_addr = %lx, filename = %s, funcname = %s, lineno = %d\n",node.low_addr,node.high_addr,node.filename,node.funcname,node.lineno);
            }
            delete[] objfile;
            fclose(file);
        }
    }
}

static void traverse_funcs_cu_helper(Dwarf_Debug dbg, Dwarf_Die cu_die, std::map<uint64_t, dwarfFuncAddrRange_t> *funcAddrRanges) {
    int ret;
    Dwarf_Error err;
    Dwarf_Die sib_die;
    Dwarf_Die cur_die = cu_die;
    for (;;) {
        Dwarf_Half tag;
        ret = dwarf_tag(cur_die, &tag, &err);
        if (ret == DW_DLV_ERROR) {
            err_handler(err, nullptr);
        }

        if (tag == DW_TAG_subprogram) {
            char *name;
            Dwarf_Attribute t_attr;
            Dwarf_Addr high_pc, low_pc;
            enum Dwarf_Form_Class cls = DW_FORM_CLASS_UNKNOWN;
            Dwarf_Half form = 0;
            bool name_found = false;

            ret = dwarf_diename(cur_die, &name, &err);
            if (ret == DW_DLV_OK) {
                name_found = true;
            } else {
                ret = dwarf_attr(cur_die, DW_AT_linkage_name, &t_attr, &err);
                if (ret == DW_DLV_OK) {
                    dwarf_dealloc_attribute(t_attr);
                    dwarf_formstring(t_attr, &name, &err);
                    name_found = true;
                }
            }

            if (name_found) {
                ret = dwarf_attr(cur_die, DW_AT_inline, &t_attr, &err);
                if (ret == DW_DLV_OK) {
                    // inline function has no pc ranges, so do nothing
                    dwarf_dealloc_attribute(t_attr);
                } else {
                    // make sure the function is not inlined
                    ret = dwarf_lowpc(cur_die, &low_pc, &err);
                    if (ret != DW_DLV_OK) {
                        goto func_notfound;
                    }
                    ret = dwarf_highpc_b(cur_die, &high_pc, &form, &cls, &err);
                    if (ret != DW_DLV_OK) {
                        goto func_notfound;
                    }
                    high_pc += low_pc;
                    // printf("*** [SEARCH FUNC low & high %p] %s: %p ~ %p\n", (void*)pc, name, (void*)low_pc, (void*)high_pc);
                    (*funcAddrRanges)[low_pc] = {name, low_pc, high_pc};
                }
            func_notfound:
                dwarf_dealloc(dbg, name, DW_DLA_STRING);
            }
        }

        if (tag == DW_TAG_compile_unit || tag == DW_TAG_namespace || tag == DW_TAG_class_type || tag == DW_TAG_structure_type) {
            Dwarf_Die child;
            ret = dwarf_child(cur_die, &child, &err);
            if (ret == DW_DLV_OK) { // child exists
                traverse_funcs_cu_helper(dbg, child, funcAddrRanges);
            }
        }

        ret = dwarf_siblingof_b(dbg, cur_die, true, &sib_die, &err);
        if (ret == DW_DLV_ERROR) {
            err_handler(err, nullptr);
        }
        if (ret == DW_DLV_NO_ENTRY) {
            break;
        }
        if (cur_die != cu_die) {
            dwarf_dealloc(dbg, cur_die, DW_DLA_DIE);
        }
        cur_die = sib_die;
    }
}

void LineInfoDB::traverse_funcs_cu(Dwarf_Debug dbg, Dwarf_Die cu_die) {
    traverse_funcs_cu_helper(dbg, cu_die, &funcAddrRanges[cu_die]);
    printf("Func die: %p\n", cu_die);
    for (auto &it : funcAddrRanges[cu_die]) {
        printf("Func: %s (%lx,%lx)\n", it.second.funcname.c_str(), it.second.lo_addr, it.second.hi_addr);
    }
}

static const char *get_funcname(uint64_t pc,
                                const std::map<uint64_t, dwarfFuncAddrRange_t> &funcAddrRanges) {
    if (funcAddrRanges.empty()) {
        return nullptr;
    }
    // Find the first range whose lo_addr is no less than `pc`
    auto func_it = funcAddrRanges.lower_bound(pc);

    if (func_it == funcAddrRanges.end()) {
        func_it--;
    } else if (func_it->first != pc) {
        // lo_addr is greater than `pc`, try to go to the prev one
        if (func_it == funcAddrRanges.begin()) {
            // is already the first one, not found
            return nullptr;
        }
        func_it--;
    }

    if (pc <= func_it->second.hi_addr) { // ensure that pc falls in the [lo, hi] range. (THE PC RANGES OF FUNCS ARE CLOSED INTERVALS)
        return func_it->second.funcname.c_str();
    } else {
        return nullptr;
    }
}

void LineInfoDB::traverse_lines_cu(Dwarf_Debug dbg, Dwarf_Die cu_die) {
    int ret;
    Dwarf_Unsigned version;
    Dwarf_Small table_count;
    Dwarf_Line_Context ctxt;
    ret = dwarf_srclines_b(cu_die, &version, &table_count, &ctxt, nullptr);
    if (ret == DW_DLV_NO_ENTRY) {
        // printf("[addr2line] No source line info\n");
        return;
    }
    if (table_count == 1) {
        Dwarf_Line *linebuf = nullptr;
        Dwarf_Signed linecount = 0;
        Dwarf_Error err;
        ret = dwarf_srclines_from_linecontext(ctxt, &linebuf, &linecount, &err);
        if (ret == DW_DLV_ERROR) {
            dwarf_srclines_dealloc_b(ctxt);
            err_handler(err, nullptr);
        }

        Dwarf_Line prev_line = nullptr;
        Dwarf_Unsigned prev_lineno;
        Dwarf_Addr prev_lineaddr;
        char *prev_linesrc;
        for (int i = 0; i < linecount; i++) {
            Dwarf_Line line = linebuf[i];
            Dwarf_Unsigned lineno;
            Dwarf_Addr lineaddr;
            char *linesrc;

            dwarf_lineaddr(line, &lineaddr, nullptr);
            dwarf_lineno(line, &lineno, nullptr);
            dwarf_linesrc(line, &linesrc, nullptr);

            if (prev_line) {
                const char *funcname = get_funcname(prev_lineaddr, funcAddrRanges[cu_die]);
                lineAddrRanges[cu_die][prev_lineaddr] = {funcname, prev_linesrc,
                                                         static_cast<int>(prev_lineno),
                                                         prev_lineaddr, lineaddr};
            }

            Dwarf_Bool is_lne;
            dwarf_lineendsequence(line, &is_lne, nullptr);
            if (is_lne) {
                const char *funcname = get_funcname(lineaddr, funcAddrRanges[cu_die]);
                lineAddrRanges[cu_die][lineaddr] = {funcname, linesrc, static_cast<int>(lineno),
                                                    lineaddr, lineaddr};
                prev_line = nullptr;
            } else {
                prev_line = line;
                prev_lineaddr = lineaddr;
                prev_lineno = lineno;
            }
            dwarf_dealloc(dbg, prev_linesrc, DW_DLA_STRING);
            prev_linesrc = linesrc;
        }
        dwarf_dealloc(dbg, prev_linesrc, DW_DLA_STRING);
    }
    dwarf_srclines_dealloc_b(ctxt);
    printf("Line die: %p\n", cu_die);
    for (auto &it : lineAddrRanges[cu_die]) {
        printf("Line: %s:%d (%lx,%lx)\n", it.second.filename.c_str(), it.second.lineno, it.second.lo_addr, it.second.hi_addr);
    }
}
