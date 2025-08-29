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
std::string output_dir;
bool pretty_print = false;
bool enable_leak = false;
bool enable_usage = false;
bool enable_check_source = false;
bool enable_mt = false;

bool enable_accl = false;
bool enable_general = true;

double mt_percent = 1;

int rank_offset = 0;
int rank_num = 0;

void parse(int argc, char* argv[]) {
    cxxopts::Options options("memory_analysis",
                             "memory_analysis need data collected by memory_wrapper");
    options.add_options()
            ("f,force", "Force to overwrite the output file if already exists.")
            ("p,pretty_print", "Print readable report in stdout.")
            ("h,help", "Print help")
            ("i,input", "data collected by memory_wrapper",
                         cxxopts::value<std::string>())
            ("d,line_info_dump", "Input line info directory dumped by dwarf_line_info_dump",
                         cxxopts::value<std::string>())
            ("o,output", "Output dir to store the chrome trace outputs after analysis",
                         cxxopts::value<std::string>())
            ("mult_threads", "Use percentage of system threads for analysis [0.5,1.0]",cxxopts::value<double>())
            ("leak", "Output memory leak information")
            ("usage", "Output memory usage information")
            ("check_source", "Open event source check")
            ("accl", "Enable Memory Analysis for Acceleration Domain​")
            ("disable_general", "Disable Memory Analysis for General Domain")
            ("rank_offset", "",cxxopts::value<int>())
            ("rank_num", "",cxxopts::value<int>())
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
                std::cout << "Warning: multiple line line info directory configured. Only use the last one!"
                          << std::endl;
            }
            dump_dir = result["line_info_dump"].as<std::string>();
        } else {
            std::cout << "-d or --line_info_dump did not specified!" << std::endl;
        }
        // output = result["output"].as<std::string>();
        if (result.count("output") >= 1) {
            if (result.count("output") > 2) {
                std::cout << "Warning: multiple output directory configured. Only use the last one!"
                          << std::endl;
            }
            output_dir = result["output"].as<std::string>();
        } else {
            std::cout << "-o or --output must be specified!" << std::endl;
            exit(1);
        }

        if (result.count("mult_threads")) {
            enable_mt = true;
            mt_percent = result["mult_threads"].as<double>();
            if(mt_percent > 1 || mt_percent < 0.5) {
                std::cout << "Error: mult_threads percent must be in [0.5,1]" << std::endl;
                exit(1);
            }
        }

        if (result.count("rank_offset")) {
            if (result.count("rank_offset") > 1) {
                std::cout << "Warning: multiple rank_offset. Only use the last one!"
                          << std::endl;
                exit(1);
            }
            rank_offset = result["rank_offset"].as<int>();
        }

        if (result.count("rank_num")) {
            if (result.count("rank_num") > 1) {
                std::cout << "Warning: multiple rank_num. Only use the last one!"
                          << std::endl;
                exit(1);
            }
            rank_num = result["rank_num"].as<int>();
        }

        if (result.count("force")) {
            is_force = true;
        }
        if (result.count("pretty_print")) {
            pretty_print = true;
        }
        if (result.count("leak")) {
            enable_leak = true;
        }
        if (result.count("usage")) {
            enable_usage = true;
        }
        if (result.count("check_source")) {
            enable_check_source = true;
        }
        if (result.count("accl")) {
            enable_accl = true;
        }
        if (result.count("disable_general")) {
            enable_general = false;
        }
        
        std::cout << "Configured: " << std::endl;
        std::cout << "\t"
                  << "Input: " << input_dir << std::endl;
        std::cout << "\t"
                  << "Output: " << output_dir << std::endl;
        if(enable_mt) {
            std::cout << "\t"
                      << "Multithreads: " << (int)(mt_percent*100) << "%" << std::endl;
        }
    } catch (const cxxopts::exceptions::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << options.help() << std::endl;
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    parse(argc, argv);
    try {
        if(fs::exists(output_dir)) {
            if(is_force) {
                fs::remove_all(output_dir);
            } 
            // else {
            //     std::cout << "Error: output directory (" << output_dir << ") exists!" << std::endl;
            //     exit(1);
            // }
        }
        else
        {
            if(!fs::create_directories(output_dir)) {
                std::cout << "Error: failed to create output directory (" << output_dir << ")!" << std::endl;
                exit(1);
            }
        }

        const char *dump;
        if(dump_dir.empty()) {
            dump = nullptr;
        }
        else
            dump = dump_dir.c_str();
        RecordReader reader(input_dir.c_str(),SECTION_MODEL,dump,true/*enable debug db*/,false/*not MPI-only*/,true);
        RecordTraceCollection& traces = reader.get_all_traces();
	    BacktraceCollection& backtraces = reader.get_all_backtraces();
        RankMetaCollection& metas = reader.get_all_meta_maps();
        

        // auto analyzer = MemoryAnalyzer(traces, &backtraces, metas, output_dir, pretty_print,enable_check_source);
        // if(enable_usage)
        //     analyzer.analysis_usage();
        // if(enable_leak)
        //     analyzer.analysis_memory_leak();


        printf("\n=======================================\n");
        printf("Memory analysis Start:\n");
        printf("=======================================\n\n");
        std::unique_ptr<MemoryAnalyzer> analyzer = nullptr;
        bool initialization_success = false;
        
        try {
            // 在堆上创建对象，由智能指针管理生命周期
            analyzer = std::make_unique<MemoryAnalyzer>(
                traces, &backtraces, metas, output_dir, pretty_print, enable_check_source, enable_mt, mt_percent,enable_general,enable_accl,enable_usage,enable_leak,rank_offset,rank_num
            );
            initialization_success = true;
            printf("\n=======================================\n");
            printf("Memory analysis Success!\n");
            printf("=======================================\n\n\n");
        } 
        catch (const std::exception& e) {
            int c=1;
            // printf("\n=======================================\n");
            // printf("Memory analysis initialization Fail!\n");
            // printf("=======================================\n\n\n");
        }
        
        // if (initialization_success) {
        //     if(enable_general)
        //     {
        //         if(enable_usage) {
        //             analyzer->analysis_usage("general");
        //         }
        //         if(enable_leak) {
        //             analyzer->analysis_memory_leak("general");
        //         }
        //     }
        //     if(enable_accl)
        //     {
        //         if(enable_usage) {
        //             analyzer->analysis_usage("accl");
        //         }
        //         if(enable_leak) {
        //             analyzer->analysis_memory_leak("accl");
        //         }
        //     }
        // }
	    // analyzer.dump_report(output.c_str());
    } 
    catch (fs::filesystem_error const & e) {
        std::cout << e.what() << std::endl;
        exit(1);
    }

    return 0;
}

