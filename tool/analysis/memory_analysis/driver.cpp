#include <stdio.h>
#include <iostream>
#include <filesystem>

#include "../timeline/alignment.h"
#include "memory_analysis.h"
#include "utils/cxxopts.hpp"

namespace fs = std::filesystem;

bool is_force = false;
std::string input_dir;
std::string dump_dir;
std::string output;
bool pretty_print = false;

void parse(int argc, char* argv[]) {
    cxxopts::Options options("memory_analysis",
                             "memory_analysis need data collected by memory_wrapper");
    options.add_options()
            ("f,force", "Force to overwrite the output file if already exists.")
            ("p,pretty_print", "Print readable report.")
            ("h,help", "Print help")
            ("i,input", "data collected by memory_wraaper",
                         cxxopts::value<std::string>())
            ("d,line_info_dump", "Input line info directory dumped by dwarf_line_info_dump",
                         cxxopts::value<std::string>())
            ("o,output", "Output file to store the chrome trace json outputs",
                         cxxopts::value<std::string>()->default_value("memory_report.txt"))
            ;

    try {
        auto result = options.parse(argc, argv);
        // print help message if configured
        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            exit(0);
        }
        if (result.count("input") >= 1) {
            if (result.count("input") > 2) {
                std::cout << "Warning: multiple input directory configured. Only use the last one!"
                          << std::endl;
            }
            input_dir = result["input"].as<std::string>();
        } else {
            std::cout << "-i or --input must be specified!" << std::endl;
            exit(1);
        }
        if (result.count("line_info_dump") >= 1) {
            if (result.count("line_info_dump") > 2) {
                std::cout << "Warning: multiple input line info directory configured. Only use the last one!"
                          << std::endl;
            }
            dump_dir = result["line_info_dump"].as<std::string>();
        } else {
            std::cout << "-d or --line_info_dump did not specified!" << std::endl;
        }
        output = result["output"].as<std::string>();
        if (result.count("force")) {
            is_force = true;
        }
        if (result.count("pretty_print")) {
            pretty_print = true;
        }
        std::cout << "Configured: " << std::endl;
        std::cout << "\t"
                  << "Input: " << input_dir << std::endl;
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
        if(fs::exists(output)) {
            if(is_force) {
                fs::remove_all(output);
            } else {
                std::cout << "Error: output directory (" << output << ") exists!" << std::endl;
                exit(1);
            }
        }
        if(!fs::create_directories(output)) {
            std::cout << "Error: failed to create output directory (" << output << ")!" << std::endl;
            exit(1);
        }
        const char *dump;
        if(dump_dir.empty()) {
            dump = nullptr;
        }
        else
            dump = dump_dir.c_str();
        RecordReader reader(input_dir.c_str(),SECTION_MODEL,dump,true/*enable debug db*/,false/*not MPI-only*/);
        RecordTraceCollection& traces = reader.get_all_traces();
	    BacktraceCollection& backtraces = reader.get_all_backtraces();
        RankMetaCollection& metas = reader.get_all_meta_maps();
        

        auto analyzer = MemoryAnalyzer(traces, &backtraces, metas);
	    // analyzer.dump_report(output.c_str());
    } 
    catch (fs::filesystem_error const & e) {
        std::cout << e.what() << std::endl;
        exit(1);
    }

    return 0;
}
