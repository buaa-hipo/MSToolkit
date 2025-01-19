//
// Created by chengyanshang on 2023/2/18.
//

#ifndef JSI_TOOLKIT_COMPRESS_SNAPPY_COMPRESS
#define JSI_TOOLKIT_COMPRESS_SNAPPY_COMPRESS

#include <string>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>

#include "utils/snappy/snappy.h"


namespace fs = std::filesystem;

#define SNAPPYCOMPRESS  1
#define SNAPPYUNCOMPRESS 2
#define SNAPPY_CACHE_SIZE 1024*1024*10

class snappy_compress_util {
private:
    std::string _init_file_name;
    std::string _target_file_name;
    int _model;

    void snappy_compress();

    void snappy_uncompress();

public:
    snappy_compress_util(std::string, std::string, int);

    void snappy_process();
};

#endif
