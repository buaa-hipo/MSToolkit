#include <mpi.h>
#include "variance_mpi.h"
#include <iostream>
#include "utils/cxxopts.hpp"
#include "record/record_reader.h"
#include <filesystem>
#include <stdio.h>
#include "../timeline/alignment.h"
#include <omp.h>

namespace fs = std::filesystem;

bool is_force = false;
bool enable_async_comm = true;
bool enable_heatmap = true;
bool enable_detail = false;
std::string input_dir;
std::string output_dir;
std::string ref_metric;
uint64_t resolution_ms = 100;
// MPI info
int rank;

#define EXECUTE_IF_MASTER(stat) if(rank==0) stat

void parse(int argc, char *argv[]) {
    cxxopts::Options options("variance_analysis", "Read the measurement directory generated by jsirun and generate variance analysis results.");
    options.add_options()
        ("f,force", "Force to overwrite the output file if already exists.")
        ("h,help", "Print help")
        ("i,input", "Input measurement directory generated by jsirun", cxxopts::value<std::string>())
        ("o,output", "Output directory to store the variance csv outputs.",cxxopts::value<std::string>()->default_value("variance"))
        ("m,reference-metric", "Reference metric for variance analysis (default: PAPI_TOT_INS)",cxxopts::value<std::string>()->default_value("PAPI_TOT_INS"))
        ("no-async-comm", "Disable async communication event probes")
        ("no-heatmap", "generate heatmap data")
        ("enable-detail", "Enable detailed csv without smoothing")
        ("r,resolution", "resolution (ms) to generate heatmap", cxxopts::value<uint64_t>())
        ;

    try {
        auto result = options.parse(argc, argv);
        // print help message if configured
        if (result.count("help")) {
            if (rank==0) {
                std::cout << options.help() << std::endl;
            }
            MPI_Barrier(MPI_COMM_WORLD);
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
        output_dir = result["output"].as<std::string>();
        if (result.count("force")) {
            is_force = true;
        }
        if (result.count("no-async-comm")) {
            enable_async_comm = false;
        }
        if (result.count("no-heatmap")) {
            enable_heatmap = false;
        }
        if (result.count("enable-detail")) {
            enable_detail = true;
        }
        if (result.count("resolution")) {
            resolution_ms = result["resolution"].as<uint64_t>();
        }
        if (result.count("enable-detail")) {
            enable_detail = true;
        }
        ref_metric = result["reference-metric"].as<std::string>();
        if (rank==0) {
            std::cout << "Configured: " << std::endl;
            std::cout << "\t" << "Input: " << input_dir << std::endl;
            std::cout << "\t" << "Output: " << output_dir << std::endl;
            std::cout << "\t" << "Reference metric: " << ref_metric << std::endl;
            std::cout << "\t" << "Thread Count: " << omp_get_max_threads() << std::endl;
        }
    } catch (cxxopts::exceptions::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << options.help() << std::endl;
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    // arg parse
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    parse(argc, argv);
    try {
        if(rank==0) {
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
        fs::path comm_out(output_dir);
        comm_out /= std::string("comm.") + std::to_string(rank) + ".csv";
        fs::path calc_out(output_dir);
        calc_out /= std::string("calc.") + std::to_string(rank) + ".csv";
        fs::path accl_calc_out(output_dir);
        accl_calc_out /= std::string("accl_calc") + std::to_string(rank) + ".csv";
        fs::path accl_memcpy_out(output_dir);
        accl_memcpy_out /= std::string("accl_memcpy") + std::to_string(rank) + ".csv";
        fs::path host_map(output_dir);
        host_map /= std::string("host.") + std::to_string(rank) + ".csv";
	double ts, te;
	ts = omp_get_wtime();
        std::cout << "\n" << "## Loading data as RecordReader..." << std::endl;
        ParallelRecordReaderUnordered reader(input_dir.c_str(),DATA_MODEL,nullptr,false);
        RecordTraceCollection& traces = reader.get_all_traces();
        RankMetaCollection& metas = reader.get_all_meta_maps();
        BacktraceCollection& backtraces = reader.get_all_backtraces();
        RecordTraceExtCollection& rte_collection = reader.get_all_ext_traces();
	te = omp_get_wtime();
	std::cout << "Reader Time: " << te-ts << " sec\n" << std::flush;
        // Align traces for each host nodes first
	ts = omp_get_wtime();
        BF_TimelineAlignment ta;
        ta.align(traces, metas);
	te = omp_get_wtime();
	std::cout << "Align Time: " << te-ts << " sec\n" << std::flush;
	ts = omp_get_wtime();
        ParallelVarianceMap vmap(traces, backtraces, rte_collection, ref_metric, enable_async_comm);
	te = omp_get_wtime();
	std::cout << "Variance Calculation Time: " << te-ts << " sec\n" << std::flush;
	ts = omp_get_wtime();
        MPI_Barrier(MPI_COMM_WORLD);
        if (enable_detail){
            vmap.dump_comm_csv(comm_out.c_str());
            vmap.dump_calc_csv(calc_out.c_str());
            vmap.dump_accl_calc_csv(accl_calc_out.c_str());
            vmap.dump_accl_memcpy_csv(accl_memcpy_out.c_str());
        }
        if (enable_heatmap) {
            fs::path comm_out(output_dir);
            comm_out /= std::string("comm_heat.") + std::to_string(rank) + ".csv";
            fs::path calc_out(output_dir);
            calc_out /= std::string("calc_heat.") + std::to_string(rank) + ".csv";
            fs::path accl_calc_out(output_dir);
            accl_calc_out /= std::string("accl_calc_heat.") + std::to_string(rank) + ".csv";
            fs::path accl_memcpy_out(output_dir);
            accl_memcpy_out /= std::string("accl_memcpy_heat.") + std::to_string(rank) + ".csv";
            vmap.dump_comm_heatmap(comm_out.c_str(), resolution_ms);
            vmap.dump_calc_heatmap(calc_out.c_str(), resolution_ms);
            vmap.dump_accl_calc_heatmap(accl_calc_out.c_str(), resolution_ms);
            vmap.dump_accl_memcpy_heatmap(accl_memcpy_out.c_str(), resolution_ms);
        }
        // dumping host name info
        FILE* fp = fopen(host_map.c_str(), "w");
        for (auto it = traces.begin(), ie = traces.end(); it != ie; ++it) {
            int rank = it->first;
            MetaDataMap::MetaValue_t *t;
            const std::unordered_map<std::string, MetaDataMap *> & metaMap = metas[rank]->getMetaMap();
            metaMap.at("HOST INFO")->get("HOSTNAME", &t);
            fprintf(fp, "%d,%s\n", rank, t->ptr);
        }
        fclose(fp);
	te = omp_get_wtime();
	std::cout << "Dump Time: " << te-ts << " sec\n";
    } catch (fs::filesystem_error const& e) {
        std::cout << e.what() << std::endl;
        exit(1);
    }
    MPI_Finalize();
    return 0;
}
