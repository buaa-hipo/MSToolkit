#ifndef __JSI_BACKTRACE_H__
#define __JSI_BACKTRACE_H__

#include <dlfcn.h>
#include <execinfo.h>

#include <cinttypes>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "utils/jsi_log.h"
#include "utils/dwarf_line_info.h"
#include "utils/addr2line.hpp"
#include "record/record_defines.h"
#include "ral/backend.h"
#include "ral/section.h"

#define BACKTRACE_UNKNOWN_NODE (-1)
#define BACKTRACE_ROOT_NODE (0)

#define BACKTRACE_BUFFER_RESERVE_SIZE 65536
#define BACKTRACE_STRING_MAX (PATH_MAX + 64)
#define BACKTRACE_STRING_BUFFER_DEFAULT_RESERVE_SIZE \
    (BACKTRACE_STRING_MAX * BACKTRACE_BUFFER_RESERVE_SIZE)

/**
 * levels of function calls inside libjsi_record while calling backtrace_context_get,
 * currently equals 1 (1 for backtrace_context_get itself and 0 for BacktraceTree::
 * backtrace_context_get since it is inlined)
 */
#define BACKTRACE_LIBJSI_RECORD_LEVELS 1

typedef int32_t backtrace_context_t;

namespace jsi { namespace toolkit {

struct backtrace_node_t {
    backtrace_context_t parent_context = BACKTRACE_UNKNOWN_NODE;
    std::unordered_map<void *, backtrace_context_t> children_contexts{};

    // Instruction pointer
    void *ip = nullptr;

    // Binary (dynamic library or the executable) information
    // obtained by `dladdr`
    char *bin_filename = nullptr;
    void *bin_base_addr = nullptr;
    char *bin_symbol_name = nullptr;

    // Source code attributions
    char* src_funcname = nullptr;
    char* src_filename = nullptr;
    int32_t lineno = 0;

    // Backtrace string of ONLY this node (no ancestors')
    char *bt_node_string = nullptr;
    // Backtrace context string from this node (including ancestors)
    std::unordered_map<int, char *> bt_context_string{};
};

struct backtrace_node_record_t {
    uint64_t ip;
    int64_t parent_context;
    uint64_t faddr;
    int64_t fname_offset;
    int64_t sname_offset;
    int64_t sfilename_offset;
    int64_t sfuncname_offset;
    int lineno;
};

class BacktraceTree {
private:
    enum class BacktraceMode {
        RECORDING,
        LOADING,
    };

    explicit BacktraceTree(int max_bt_size, BacktraceMode mode);


public:
    ~BacktraceTree();

    static BacktraceTree *create_recording_bt_tree(int max_bt_size);

    static BacktraceTree *create_loading_bt_tree();

    inline __attribute__((always_inline)) backtrace_context_t backtrace_context_get(int omit_level=0);

    void backtrace_db_dump(FILE *file);
    void backtrace_db_dump(std::unique_ptr<pse::ral::DirSectionInterface> &dir);

    void backtrace_db_load(FILE *file, const char* dwarf_dir = nullptr, bool enable_dbinfo = true);
    void backtrace_db_load(std::unique_ptr<pse::ral::DirSectionInterface> &dir, const char* dwarf_dir = nullptr, bool enable_dbinfo = true);

    backtrace_context_t get_parent(backtrace_context_t ctx) const;

    void backtrace_get_context_string_vec(backtrace_context_t ctx, int size,
                                          std::vector<const char *> &vec);

    const char *backtrace_get_context_string(backtrace_context_t ctx);

    const char *backtrace_get_context_string(backtrace_context_t ctx, int size);

    const char *backtrace_get_node_string(backtrace_context_t ctx);

    static bool backtrace_is_equal(backtrace_context_t ctx1, const BacktraceTree &tree1,
                                   backtrace_context_t ctx2, const BacktraceTree &tree2);

    void *backtrace_context_ip(backtrace_context_t ctx) const;

    // uint64_t backtrace_get_callsite();

private:
    BacktraceMode _mode;
    char *_loaded_string_buffer = nullptr;
    std::vector<void *> _backtrace_buffer;
    std::vector<backtrace_node_t> _bt_nodes;
    int _max_bt_size;
};

}}// namespace jsi::toolkit

void backtrace_init_recording(int max_bt_size);
void backtrace_finalize();
backtrace_context_t backtrace_context_get(int omit_level=0);

/**
 * WARNING: The backtrace will be invalid after dumping.
 */
void backtrace_db_dump(FILE *file);
void backtrace_db_dump(std::unique_ptr<pse::ral::DirSectionInterface> &dir);

backtrace_context_t backtrace_context_get_parent(backtrace_context_t bt_ctxt);
void backtrace_context_print(backtrace_context_t bt_ctxt, int size);
const char *backtrace_context_string(backtrace_context_t bt_ctxt, int size);
bool backtrace_is_same(backtrace_context_t c1, backtrace_context_t c2);
bool backtrace_is_same_parent(backtrace_context_t c1, backtrace_context_t c2);
void *backtrace_context_get_ip(backtrace_context_t bt_ctxt);
uint64_t backtrace_get_callsite();

#endif
