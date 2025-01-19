#pragma once

#include <dwarf.h>
#include <err.h>
#include <libdwarf.h>
#include <libelf.h>

#include <climits>
#include <cstring>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

#define DW_DLV_BADADDR (~(Dwarf_Addr) 0)

#define LINEINFO_BUFFER_RESERVE_SIZE 65536
#define LINEINFO_STRING_MAX (PATH_MAX + 64)
#define LINEINFO_STRING_BUFFER_DEFAULT_RESERVE_SIZE \
    (LINEINFO_STRING_MAX * LINEINFO_BUFFER_RESERVE_SIZE)

#define ERROR(format...) do { \
        fprintf(stderr, format); \
        exit(-1);                \
    } while (0)

#define DWARF_CHECK(stat, err) do { if((stat)!=DW_DLV_OK) { printf("[%s@%d] Fatal Error.\n", __FILE__, __LINE__); err_handler(err, NULL); } } while(0)


class Addr2LineInfo {
public:
    char* funcname;
    char* filename;
    int lineno;

    Addr2LineInfo(char* funcname, char* filename, int lineno) {
        this->funcname = funcname;
        this->filename = filename;
        this->lineno = lineno;
    }

    Addr2LineInfo() {
        this->funcname = NULL;
        this->filename = NULL;
        this->lineno = 0;
    }

    bool operator<(const Addr2LineInfo& info) const {
        return (strcmp(filename, info.filename) < 0) ||
               (strcmp(filename, info.filename) == 0 && strcmp(funcname, info.funcname) < 0) ||
               (strcmp(filename, info.filename) == 0 && strcmp(funcname, info.funcname) == 0 &&
                lineno < info.lineno);
    }
};

struct lineInfo2AddrsRange_t {
    const char* funcname;
    const char* filename;
    int lineno;
    uint64_t low_addr;
    uint64_t high_addr;
};

struct lineInfo2AddrsRange {
    int lineno;
    uint64_t low_addr;
    uint64_t high_addr;
    int64_t funcname_offset;
    int64_t filename_offset;
};

struct dwarfFuncAddrRange_t {
    std::string funcname;
    uint64_t lo_addr, hi_addr;
};

struct dwarfLineAddrRange_t {
    const char* funcname;
    std::string filename;
    int lineno;
    uint64_t lo_addr, hi_addr;
};

class LineInfoDB {
private:
    enum class DBMode {
        RECORDING,
        LOADING,
    };

    explicit LineInfoDB(DBMode mode);

public:
    ~LineInfoDB();

    static LineInfoDB* create_recording_db();

    static LineInfoDB* create_loading_db();

    void lineinfo_dump(const char* objfile, const char* dumpfile);

    void lineinfo_load(const char* dumpfile);

    void addr2line(const char* objfile, uint64_t pc, Addr2LineInfo* info);

    void traverse_funcs_cu(Dwarf_Debug dbg, Dwarf_Die cu_die);

    void traverse_lines_cu(Dwarf_Debug dbg, Dwarf_Die cu_die);

private:
    DBMode _mode;
    std::vector<char*> _loaded_string_buffer;
    std::unordered_map<std::string, std::vector<lineInfo2AddrsRange_t>> reclist;
    // info => [low, high] addr range
    std::map<Addr2LineInfo, std::pair<Dwarf_Addr, Dwarf_Addr>> info2AddrsRange;
    // modify to map: <cu_die, vector<pair> ?>
    std::map<Dwarf_Die, std::vector<std::pair<Dwarf_Addr, Dwarf_Addr>>> cu2LowHighPc;
    std::vector<std::pair<Dwarf_Addr, Dwarf_Addr>> cu_low_high_pc;

    std::unordered_map<Dwarf_Die, std::map<uint64_t, dwarfFuncAddrRange_t>> funcAddrRanges;
    std::unordered_map<Dwarf_Die, std::map<uint64_t, dwarfLineAddrRange_t>> lineAddrRanges;
};

#define MAX_PATH_SIZE 200

static void err_handler(Dwarf_Error err, Dwarf_Ptr errarg) {
    errx(EXIT_FAILURE, "libdwarf error: %lld %s", dwarf_errno(err), dwarf_errmsg(err));
}
