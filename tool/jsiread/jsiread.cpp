#include <iostream>
#include "utils/cxxopts.hpp"
#include "record/record_reader.h"
#include <filesystem>
#include <stdio.h>

namespace fs = std::filesystem;

bool is_dump = false;
bool is_force = false;
bool backtrace_enabled = false;
int size = -1;
std::string dump_dir;
std::string input_dir;
std::string output_dir;
int mode = 1;

void parse(int argc, char *argv[]) {
    cxxopts::Options options("jsiread", "Read the measurement directory generated by jsirun.");
    options.add_options()
        ("b,backtrace", "Read backtrace information")
        ("d,dump", "Dump readable outputs")
        ("f,force", "Force to remove the output directory if already exists.")
        ("h,help", "Print help")
        ("l,line_info_dump", "Input line info directory dumped by dwarf_line_info_dump",
                         cxxopts::value<std::string>())
        ("i,input", "Input measurement directory generated by jsirun", cxxopts::value<std::string>())
        ("o,output", "Output directory to store the readable outputs",cxxopts::value<std::string>()->default_value("readable"))
        ("s,size", "Maximum number of output trace event (normal). (-1 for all events <default>)",cxxopts::value<int>())
        ("m,mode", "Data model (0: legacy, 1: section)",cxxopts::value<int>()->default_value("1"));
        ;

    try {
        auto result = options.parse(argc, argv);
        // print help message if configured
        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            exit(1);
        }
        if (result.count("input")>=1) {
            if(result.count("input")>2) {
                std::cout << "Warning: multiple input directory configured. Only use the last one!" << std::endl;
            }
            input_dir = result["input"].as<std::string>();
        } else {
            std::cout << "-i or --input must be specified!" << std::endl;
            exit(1);
        }
        if (result.count("size")) {
            size = result["size"].as<int>();
        }
        if (result.count("line_info_dump") >= 1) {
            if (result.count("line_info_dump") > 2) {
                std::cout << "Warning: multiple input line info directory configured. Only use the last one!"
                          << std::endl;
            }
            dump_dir = result["line_info_dump"].as<std::string>();
        } else {
            std::cout << "-l or --line_info_dump not specified! Runtime dwarf addr2line will be used, which will cause a great slowdown!" << std::endl;
        }
        if (result.count("output") || result.count("dump")) {
            is_dump = true;
        }
        if (is_dump) {
            output_dir = result["output"].as<std::string>();
        }
        if (result.count("force")) {
            is_force = true;
        }
        if (result.count("backtrace")) {
            backtrace_enabled = true;
        }
        if (result.count("mode")) {
            mode = result["mode"].as<int>();
        }
        std::cout << "Configured: " << std::endl;
        std::cout << "\t" << "Input: " << input_dir << std::endl;
        std::cout << "\t" << "Output: " << (is_dump ? output_dir : std::string("<stdout>")) << std::endl;
    } catch (cxxopts::exceptions::exception e) {
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << options.help() << std::endl;
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    // arg parse
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
        if (is_dump) {
            if(fs::exists(output_dir)) {
                if(is_force) {
                    fs::remove_all(output_dir);
                } else {
                    std::cout << "Error: output directory (" << output_dir << ") exists!" << std::endl;
                    exit(1);
                }
            }
            if(!fs::create_directories(output_dir)) {
                std::cout << "Error: failed to create output directory (" << output_dir << ")!" << std::endl;
                exit(1);
            }
        }
        // dump meta info first
        RankMetaCollection& metas = reader.get_all_meta_maps();
        for (const auto &pair: metas) {
            FILE* fp;
            int rank = pair.first;
            auto& data = pair.second;
            if (is_dump) {
                std::string fn;
                if (rank>=0) {
                    fn = output_dir + std::string("/meta.") + std::to_string(rank);
                    fp = fopen(fn.c_str(), "w");
                } else {
                    fn = output_dir + std::string("/meta.pid") + std::to_string(rank);
                    fp = fopen(fn.c_str(), "w");
                }
                if (fp == NULL) {
                    std::cout << "Error: failed to open output file " << fn << std::endl;
                    exit(1);
                }
            } else {
                fp = stdout;
            }
            std::cout << "## Meta for Rank " << rank << std::endl;
            fprintf(fp, "%s", data->to_string().c_str());
        }
        // dump traces
        BacktraceCollection& backtraces = reader.get_all_backtraces();
        std::vector<RecordTrace *> rt_list;
        std::vector<int> rank_list;
        for (auto it = traces.begin(); it != traces.end(); ++it) {
            rt_list.push_back(it->second);
            rank_list.push_back(it->first);
        }
// #pragma omp parallel for
        for(int i = 0; i < rt_list.size(); i++) {
            FILE* fp;
            int rank = rank_list[i];
            if (is_dump) {
                std::string fn;
                if (rank>=0) {
                    fn = output_dir + std::string("/result.") + std::to_string(rank);
                    fp = fopen(fn.c_str(), "w");
                } else {
                    fn = output_dir + std::string("/result.pid") + std::to_string(rank);
                    fp = fopen(fn.c_str(), "w");
                }
                if (fp == NULL) {
                    std::cout << "Error: failed to open output file " << fn << std::endl;
                    exit(1);
                }
            } else {
                fp = stdout;
            }
            std::cout << "## Rank " << rank << std::endl;
            RecordTrace* rt = rt_list[i];
            auto num_events = rt->num_pmu_events();
            auto& event_list = rt->pmu_event_list();
            int j = 1;
            for (RecordTraceIterator it = rt->begin(); it != rt->end(); it = it.next(), ++j) {
                if (size>0 && j>size) {
                    fprintf(fp, "## Early stop as the output length exceeds given size: %d ##\n", size);
                    break;
                }
                fprintf(fp, "=== Event %d ===\n", j);
                // Write trace data and backtraces (if exist)
                fprintf(fp, "%s", RecordHelper::dump_string(it.val(), backtraces[rank]).c_str());
                // Write PMU event counter values
                for (int eidx = 0; eidx < num_events; eidx++) {
                    auto& event_name = event_list[eidx];
                    const auto enter_val =
                            RecordHelper::counter_val_enter(it.val(), eidx, num_events);
                    const auto exit_val =
                            RecordHelper::counter_val_exit(it.val(), eidx, num_events);
                    fprintf(fp, "PMU Event Counter %d(%s): (%lu, %lu, %lu)\n", eidx,
                            event_name.c_str(), enter_val, exit_val, exit_val - enter_val);
                }
            }
            if (is_dump) {
                fclose(fp);
            }
        }

        RecordTraceExtCollection& rte_collection = reader.get_all_ext_traces();
        for (auto it = rte_collection.begin(); it != rte_collection.end(); ++it) {
            FILE* fp;
            int pid = it->first;
            RecordTraceExt* rte = it->second;
            if (is_dump) {
                std::string fn = output_dir + std::string("/result_ext.") + std::to_string(pid);
                fp = fopen(fn.c_str(), "w");
                if (fp == NULL) {
                    std::cout << "Error: failed to open output file " << fn << std::endl;
                    exit(1);
                }
            } else {
                fp = stdout;
            }

            std::cout << "## EXT TRACE PID " << pid << std::endl;
            fprintf(fp, "%s\n", rte->dump().c_str());

            if (is_dump) {
                fclose(fp);
            }
        }

    } catch (fs::filesystem_error const& e) {
        std::cout << e.what() << std::endl;
        delete _reader;
        exit(1);
    }
    delete _reader;
    return 0;
}
