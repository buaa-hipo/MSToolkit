#include <iostream>
#include <filesystem>
#include <stdio.h>
#include "utils/cxxopts.hpp"
#include "compress/compress.h"

namespace fs = std::filesystem;

std::string input_dir;

void parse(int argc, char *argv[]) {
    cxxopts::Options options("jsidecompress", "Decompress the mpi trace directory compressed by jsicompress.");
    options.add_options()
            ("h,help", "Print help")
            ("i,input", "Input mpi trace directory compressed by jsicompress", cxxopts::value<std::string>())
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
        std::cout << "Configured: " << std::endl;
        std::cout << "\t" << "Input: " << input_dir << std::endl;
    } catch (cxxopts::exceptions::exception e) {
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << options.help() << std::endl;
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    // arg parse
    parse(argc, argv);
    HashCompress * compressUtil = new HashCompress(input_dir);
    compressUtil->unCompressFile();
    return 0;
}