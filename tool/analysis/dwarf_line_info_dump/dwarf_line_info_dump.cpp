#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "utils/cxxopts.hpp"
#include "utils/dwarf_line_info.h"

#include <filesystem>
namespace fs = std::filesystem;

bool is_force = false;
std::vector<std::string> objfiles;
std::string output;

void parse(int argc, char* argv[]) {
    cxxopts::Options options("dwarf_line_info_dump", "Dump line info from objfile using dwarf. ");
    options.add_options()
            ("f,force", "Force to overwrite the output file if already exists.")
            ("h,help", "Print help")
            ("o,output", "Output file to store the line infos",
                         cxxopts::value<std::string>()->default_value("dwarf.report.txt"))
            ("i,input", "Input objfile(s) to process",
                         cxxopts::value<std::vector<std::string>>())
            ;

    options.parse_positional({"input"});
    try {
        auto result = options.parse(argc, argv);
        // print help message if configured
        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            exit(1);
        }
        if (result.count("input") >= 1) {
            // std::cout << result["input"].as<std::vector<std::string>>()[0] << std::endl;
            auto& tmp = result["input"].as<std::vector<std::string>>();
            for (auto& file: tmp) {
                objfiles.push_back(file);
            }
            // input_dir = result["input"].as<std::string>();
        } else {
            std::cout << "-i or --input must be specified!" << std::endl;
            exit(1);
        }
        output = result["output"].as<std::string>();

        if (result.count("force")) {
            is_force = true;
        }
        std::cout << "Configured: " << std::endl;
        for (auto& objfile: objfiles)
            std::cout << "\t"
                      << "Input: " << objfile << std::endl;
        std::cout << "\t"
                  << "Output: " << output << std::endl;
    } catch (const cxxopts::exceptions::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << options.help() << std::endl;
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    parse(argc, argv);
    try {
        if (fs::exists(output)) {
            if (is_force) {
                fs::remove_all(output);
            } else {
                std::cout << "Error: output directory (" << output << ") exists!" << std::endl;
                exit(1);
            }
        }
        if (!fs::create_directories(output)) {
            std::cout << "Error: failed to create output directory (" << output << ")!"
                      << std::endl;
            exit(1);
        }
        for (auto& file: objfiles) {
            // std::cout << "objfile: " << file << ", " << file.substr(file.find_last_of('/')+1) << std::endl;
            Dwarf_Debug dbg;
            Dwarf_Error de;
            int ret;

            const char* objfile = file.c_str();
            std::filesystem::path objfile_rel = objfile;
            int fd = open(objfile, O_RDONLY);
            ret = dwarf_init_b(fd, DW_GROUPNUMBER_ANY, err_handler, NULL, &dbg, &de);

            Dwarf_Bool is_info = true;
            Dwarf_Unsigned next_cu_header;
            Dwarf_Half header_cu_type;
            int cu_i;

            auto lineinfo_db = LineInfoDB::create_recording_db();

            for (int cu_i = 0;; cu_i++) {
                ret = dwarf_next_cu_header_d(dbg, is_info, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                             NULL, &next_cu_header, &header_cu_type, NULL);
                if (ret == DW_DLV_NO_ENTRY) {
                    break;
                }
                Dwarf_Die cu_die = 0;
                ret = dwarf_siblingof_b(dbg, 0, is_info, &cu_die, NULL);
                if (ret == DW_DLV_OK) {
                    lineinfo_db->traverse_funcs_cu(dbg, cu_die);
                    lineinfo_db->traverse_lines_cu(dbg, cu_die);
                    dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
                    cu_die = 0;
                }
            }

            //TODO: read a files list and dump to a dir together
            std::string tmp = output + std::string("/") + file.substr(file.find_last_of('/') + 1) +
                              std::string(".dump");
            const char* outfile = tmp.c_str();
            // std::cout << tmp << std::endl;
            char* objfilename = new char[MAX_PATH_SIZE];
            strcpy(objfilename, std::filesystem::absolute(objfile_rel).string().c_str());
            lineinfo_db->lineinfo_dump(objfilename, outfile);
            // printf("%s\n", objfilename);

            dwarf_finish(dbg);
            close(fd);
            delete lineinfo_db;
        }

        /*
        auto lineinfo_db_load = LineInfoDB::create_loading_db();
	lineinfo_db_load->lineinfo_load(output.c_str());
        Addr2LineInfo* info = new Addr2LineInfo();
        info->filename = new char[MAX_PATH_SIZE];
        info->funcname = new char[MAX_PATH_SIZE];
	std::cout << objfiles[0].c_str() << objfiles[1].c_str() << std::endl;
        lineinfo_db_load->addr2line(objfiles[0].c_str(), 9234, info);
        printf("%s, %s:%d\n", info->filename, info->funcname, info->lineno);
        lineinfo_db_load->addr2line(objfiles[1].c_str(), 13333, info);
        printf("%s, %s:%d\n", info->filename, info->funcname, info->lineno);
	    */
    } catch (fs::filesystem_error const& e) {
        std::cout << e.what() << std::endl;
        exit(1);
    }


    return 0;
}
