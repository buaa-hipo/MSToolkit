#pragma once

#include <dwarf.h>
#include <err.h>
#include <fcntl.h>
#include <libdwarf.h>
#include <libelf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils/dwarf_line_info.h"

namespace {

#define MAX_PATH_SIZE 200

static bool pc_in_die(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Addr pc) {
    int ret;
    Dwarf_Addr cu_lowpc = DW_DLV_BADADDR, cu_highpc;
    enum Dwarf_Form_Class highpc_cls;
    ret = dwarf_lowpc(die, &cu_lowpc, NULL);
    if (ret == DW_DLV_OK) {
        if (pc == cu_lowpc) {
            return true;
        }
        ret = dwarf_highpc_b(die, &cu_highpc, NULL, &highpc_cls, NULL);
        if (ret == DW_DLV_OK) {
            if (highpc_cls == DW_FORM_CLASS_CONSTANT) {
                cu_highpc += cu_lowpc;
            }
            if (pc >= cu_lowpc && pc < cu_highpc) {
                return true;
            }
        }
    }
    Dwarf_Attribute attr;
    if (dwarf_attr(die, DW_AT_ranges, &attr, NULL) == DW_DLV_OK) {
        Dwarf_Unsigned offset;
        if (dwarf_global_formref(attr, &offset, NULL) == DW_DLV_OK) {
            Dwarf_Signed count = 0;
            Dwarf_Ranges* ranges = 0;
            Dwarf_Addr baseaddr = 0;
            if (cu_lowpc != DW_DLV_BADADDR) {
                baseaddr = cu_lowpc;
            }
            ret = dwarf_get_ranges_b(dbg, offset, die, NULL, &ranges, &count, NULL, NULL);
            for (int i = 0; i < count; i++) {
                Dwarf_Ranges* cur = ranges + i;
                if (cur->dwr_type == DW_RANGES_ENTRY) {
                    Dwarf_Addr rng_lowpc, rng_highpc;
                    rng_lowpc = baseaddr + cur->dwr_addr1;
                    rng_highpc = baseaddr + cur->dwr_addr2;
                    if (pc >= rng_lowpc && pc < rng_highpc) {
                        dwarf_dealloc_ranges(dbg, ranges, count);
                        dwarf_dealloc(dbg, attr, DW_DLA_ATTR);
                        return true;
                    }
                } else if (cur->dwr_type == DW_RANGES_ADDRESS_SELECTION) {
                    baseaddr = cur->dwr_addr2;
                } else {// DW_RANGES_END
                    baseaddr = cu_lowpc;
                }
            }
            dwarf_dealloc_ranges(dbg, ranges, count);
        }
        dwarf_dealloc(dbg, attr, DW_DLA_ATTR);
    }
    return false;
}

static void print_line(Dwarf_Debug dbg, Dwarf_Line line, Dwarf_Addr pc, Addr2LineInfo* info) {
    Dwarf_Unsigned lineno;
    if (line) {
        char* linesrc;
        dwarf_linesrc(line, &linesrc, nullptr);
        dwarf_lineno(line, &lineno, nullptr);
        strncpy(info->filename, linesrc, MAX_PATH_SIZE);
        dwarf_dealloc(dbg, linesrc, DW_DLA_STRING);
    } else {
        strncpy(info->funcname, "??", MAX_PATH_SIZE);
        strncpy(info->filename, "??", MAX_PATH_SIZE);
        lineno = 0;
    }
    info->lineno = lineno;
}

static bool lookup_func_pc_cu(Dwarf_Debug dbg, Dwarf_Addr pc, Dwarf_Die cu_die, Addr2LineInfo* info,
                              int in_level) {
    int ret;
    Dwarf_Error err;
    Dwarf_Die sib_die;
    Dwarf_Die cur_die = cu_die;
    for (;;) {
        Dwarf_Half tag;
        ret = dwarf_tag(cur_die, &tag, &err);
        if (ret == DW_DLV_ERROR) {
            err_handler(err, NULL);
        }

        if (tag == DW_TAG_subprogram) {
            char* name;
            Dwarf_Attribute t_attr;
            Dwarf_Addr high_pc, low_pc;
            enum Dwarf_Form_Class cls = DW_FORM_CLASS_UNKNOWN;
            Dwarf_Half form = 0;
            ret = dwarf_diename(cur_die, &name, &err);
            if (ret == DW_DLV_OK) {
                ret = dwarf_attr(cur_die, DW_AT_inline, &t_attr, &err);
                if (ret == DW_DLV_OK) {
                    // inline function has no pc ranges, so do nothing
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
                    if (pc >= low_pc && pc <= high_pc) {
                        strncpy(info->funcname, name, MAX_PATH_SIZE);
                        dwarf_dealloc(dbg, name, DW_DLA_STRING);
                        return true;
                    }
                }
            func_notfound:
                dwarf_dealloc(dbg, name, DW_DLA_STRING);
            }
        }

        if (tag == DW_TAG_compile_unit) {
            Dwarf_Die child;
            DWARF_CHECK(dwarf_child(cur_die, &child, &err), err);
            if (lookup_func_pc_cu(dbg, pc, child, info, in_level + 1)) {
                return true;
            }
        }

        ret = dwarf_siblingof_b(dbg, cur_die, true, &sib_die, &err);
        if (ret == DW_DLV_ERROR) {
            err_handler(err, NULL);
        }
        if (ret == DW_DLV_NO_ENTRY) {
            break;
        }
        if (cur_die != cu_die) {
            dwarf_dealloc(dbg, cur_die, DW_DLA_DIE);
        }
        cur_die = sib_die;
    }
    strncpy(info->funcname, "??", MAX_PATH_SIZE);
    return false;
}

static bool lookup_pc_cu(Dwarf_Debug dbg, Dwarf_Addr pc, Dwarf_Die cu_die, Addr2LineInfo* info) {
    int ret;
    Dwarf_Unsigned version;
    Dwarf_Small table_count;
    Dwarf_Line_Context ctxt;
    lookup_func_pc_cu(dbg, pc, cu_die, info, 0);
    ret = dwarf_srclines_b(cu_die, &version, &table_count, &ctxt, NULL);
    if (ret == DW_DLV_NO_ENTRY) {
        // printf("[addr2line] No source line info\n");
        return false;
    }
    bool is_found = false;
    if (table_count == 1) {
        Dwarf_Line* linebuf = 0;
        Dwarf_Signed linecount = 0;
        Dwarf_Error err;
        ret = dwarf_srclines_from_linecontext(ctxt, &linebuf, &linecount, &err);
        if (ret == DW_DLV_ERROR) {
            dwarf_srclines_dealloc_b(ctxt);
            err_handler(err, NULL);
        }
        Dwarf_Addr prev_lineaddr;
        Dwarf_Line prev_line = 0;
        for (int i = 0; i < linecount; i++) {
            Dwarf_Line line = linebuf[i];
            Dwarf_Addr lineaddr;
            dwarf_lineaddr(line, &lineaddr, NULL);
            if (pc == lineaddr) {
                /* Print the last line entry containing current pc. */
                Dwarf_Line last_pc_line = line;
                for (int j = i + 1; j < linecount; j++) {
                    Dwarf_Line j_line = linebuf[j];
                    dwarf_lineaddr(j_line, &lineaddr, NULL);
                    if (pc == lineaddr) {
                        last_pc_line = j_line;
                    }
                }
                is_found = true;
                print_line(dbg, last_pc_line, pc, info);
                break;
            } else if (prev_line && pc > prev_lineaddr && pc < lineaddr) {
                is_found = true;
                print_line(dbg, prev_line, pc, info);
                break;
            }
            Dwarf_Bool is_lne;
            dwarf_lineendsequence(line, &is_lne, NULL);
            if (is_lne) {
                prev_line = 0;
            } else {
                prev_lineaddr = lineaddr;
                prev_line = line;
            }
        }
    }
    dwarf_srclines_dealloc_b(ctxt);
    return is_found;
}

static bool lookup_pc(Dwarf_Debug dbg, Dwarf_Addr pc, Addr2LineInfo* info) {
    Dwarf_Bool is_info = true;
    Dwarf_Unsigned next_cu_header;
    Dwarf_Half header_cu_type;
    int ret;
    int cu_i;
    for (cu_i = 0;; cu_i++) {
        ret = dwarf_next_cu_header_d(dbg, is_info, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                     &next_cu_header, &header_cu_type, NULL);
        if (ret == DW_DLV_NO_ENTRY) {
            break;
        }
        Dwarf_Die cu_die = 0;
        ret = dwarf_siblingof_b(dbg, nullptr, is_info, &cu_die, nullptr);
        if (ret == DW_DLV_OK) {
            if (pc_in_die(dbg, cu_die, pc)) {
                bool lookup_ret = lookup_pc_cu(dbg, pc, cu_die, info);
                dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
                while (dwarf_next_cu_header_d(dbg, is_info, NULL, NULL, NULL, NULL, NULL, NULL,
                                              NULL, NULL, &next_cu_header, &header_cu_type,
                                              NULL) != DW_DLV_NO_ENTRY) {
                }
                return lookup_ret;
            } else {
                dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
                cu_die = 0;
            }
        }
    }
    return false;
}

void addr2line(const char* objfile, uint64_t pc, Addr2LineInfo* info) {
    int ret;
    Dwarf_Error err;
    Dwarf_Debug dbg;
    info->filename = new char[MAX_PATH_SIZE];
    info->funcname = new char[MAX_PATH_SIZE];
    int fd = open(objfile, O_RDONLY);
    if (fd<0) {
        strncpy(info->funcname, "??", MAX_PATH_SIZE);
        strncpy(info->filename, "??", MAX_PATH_SIZE);
        info->lineno = 0;
	return ;
    }
    ret = dwarf_init_b(fd, DW_GROUPNUMBER_ANY, err_handler, nullptr, &dbg, &err);
    if (ret == DW_DLV_NO_ENTRY) {
        errx(EXIT_FAILURE, "%s not found", objfile);
    }

    bool is_found = lookup_pc(dbg, pc, info);
    if (!is_found) {
        print_line(dbg, nullptr, pc, info);
    }

    dwarf_finish(dbg);
    close(fd);
}

}
