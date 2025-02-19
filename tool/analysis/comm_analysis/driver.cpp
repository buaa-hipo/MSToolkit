#include <stdio.h>
#include <iostream>
#include <filesystem>

#include "../timeline/alignment.h"
#include "comm_analysis.h"
#include "utils/cxxopts.hpp"

namespace fs = std::filesystem;

bool is_force = false;
std::string input_dir;
std::string dump_dir;
std::string output;
int topk, tv, tn, tt;
double lwt, wtv;
bool pretty_print = false;
int mode;

void parse(int argc, char* argv[]) {
    cxxopts::Options options("comm_analysis",
                             "Read the measurement directory generated by jsirun and generate comm "
                             "analysis results. Backtraces are needed for this analysis.");
    options.add_options()
            ("f,force", "Force to overwrite the output file if already exists.")
            ("p,pretty_print", "Print readable report.")
            ("h,help", "Print help")
            ("k,topk", "Set Top-k num",
                        cxxopts::value<std::string>()->default_value("50"))
            ("c,threshold_lwt", "Set threshold for long commucation time",
                        cxxopts::value<std::string>()->default_value("0.01"))
            ("w,threshold_wtv", "Set threshold for long wait time",
                        cxxopts::value<std::string>()->default_value("0.01"))
            ("v,threshold_S", "Set threshold for communication volume",
                        cxxopts::value<std::string>()->default_value("5000"))
            ("n,threshold_n", "Set threshold for communication times",
                        cxxopts::value<std::string>()->default_value("1000"))
            ("t,threshold_t", "Set threshold for minimal communication time",
                        cxxopts::value<std::string>()->default_value("1"))
            ("i,input", "Input measurement directory generated by jsirun",
                         cxxopts::value<std::string>())
            ("d,line_info_dump", "Input line info directory dumped by dwarf_line_info_dump",
                         cxxopts::value<std::string>())
            ("o,output", "Output file to store the chrome trace json outputs",
                         cxxopts::value<std::string>()->default_value("comm.report.txt"))
            ("m,mode", "Data model (0: legacy, 1: section)",
                         cxxopts::value<int>()->default_value("0"));
            ;

    try {
        auto result = options.parse(argc, argv);
        // print help message if configured
        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            exit(1);
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
        topk = std::stoi(result["topk"].as<std::string>());
        lwt = std::stod(result["threshold_lwt"].as<std::string>());
        wtv = std::stod(result["threshold_wtv"].as<std::string>());
        tv = std::stoi(result["threshold_S"].as<std::string>());
        tn = std::stoi(result["threshold_n"].as<std::string>());
        tt = std::stoi(result["threshold_t"].as<std::string>());
        if (result.count("force")) {
            is_force = true;
        }
        if (result.count("pretty_print")) {
            pretty_print = true;
        }
        if (result.count("mode")) {
            mode = result["mode"].as<int>();
        }
        std::cout << "Configured: " << std::endl;
        std::cout << "\t"
                  << "Input: " << input_dir << std::endl;
        std::cout << "\t"
                  << "Output: " << output << std::endl;
        std::cout << "\t"
                  << "Topk: " << topk << std::endl;
    } catch (const cxxopts::exceptions::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << options.help() << std::endl;
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    parse(argc, argv);
    RecordReader* _reader = nullptr;
    try {
        if (mode == 0)
        {
            _reader = new RecordReader(input_dir.c_str(),DATA_MODEL,dump_dir.empty() ? nullptr : dump_dir.c_str());
        }
        else
        {
            _reader = new RecordReader(input_dir.c_str(),SECTION_MODEL,dump_dir.empty() ? nullptr : dump_dir.c_str());
        }
        RecordReader& reader = *_reader;
        RecordTraceCollection& traces = reader.get_all_traces();
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
        BacktraceCollection& backtraces = reader.get_all_backtraces();
        RankMetaCollection& metas = reader.get_all_meta_maps();

        auto analyzer = CommunicationAnalyzer(reader, traces, &backtraces, metas, topk, lwt, wtv, tv, tn, tt, pretty_print, output.c_str());
    } catch (fs::filesystem_error const& e) {
        std::cout << e.what() << std::endl;
        exit(1);
    }

    return 0;
}
