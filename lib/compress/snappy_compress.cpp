//
// Created by chengyanshang on 2023/1/27.
//

#include "compress/snappy_compress.h"

void snappy_compress_util::snappy_process() {
    switch (this->_model) {
        case SNAPPYCOMPRESS: {
            snappy_compress();
            break;
        }
        case SNAPPYUNCOMPRESS: {
            snappy_uncompress();
            break;
        }
    }
    return;
}

void snappy_compress_util::snappy_compress() {
    // 0. 打开等待处理的文件，使用mmap挂载
    int init_fd = open(this->_init_file_name.c_str(), O_RDONLY);
    if (init_fd < 0) {
        return;
    }
    off_t fsize = lseek(init_fd, 0, SEEK_END);
    lseek(init_fd, 0, SEEK_SET);
    void *init_mmap_file = mmap(0, fsize, PROT_READ, MAP_PRIVATE/*may use MAP_SHARED?*/, init_fd, 0);
    // 1. 打开等待写入的文件，异步写入
    int target_fd = open(this->_target_file_name.c_str(), O_WRONLY | O_CREAT, 0777);
    if (target_fd < 0) {
        return;
    }
    // 2. 申请缓存空间，进行数据读取并同步写入到磁盘中
    off_t idx = 0;
    char *localCache = (char *) malloc(1024 * 1024 * 10);
    while (idx < fsize) {
        off_t cpySize = fsize - idx >= SNAPPY_CACHE_SIZE ? SNAPPY_CACHE_SIZE : fsize - idx;
        memcpy(localCache, init_mmap_file, cpySize);
        init_mmap_file = (uint8_t *) init_mmap_file + cpySize;
        std::string output = "";
        snappy::Compress(localCache, cpySize, &output);
        size_t output_size = output.size();
        write(target_fd, &output_size, sizeof(output_size));
        write(target_fd, output.c_str(), output.size());
        idx += cpySize;
    }
    close(target_fd);
    close(init_fd);
}

void snappy_compress_util::snappy_uncompress() {
    // 0. 打开等待处理的文件，使用mmap挂载
    int init_fd = open(this->_init_file_name.c_str(), O_RDONLY);
    if (init_fd < 0) {
        return;
    }
    off_t fsize = lseek(init_fd, 0, SEEK_END);
    lseek(init_fd, 0, SEEK_SET);
    void *init_mmap_file = mmap(0, fsize, PROT_READ, MAP_PRIVATE/*may use MAP_SHARED?*/, init_fd, 0);
    // 1. 打开等待写入的文件，异步写入
    int target_fd = open(this->_target_file_name.c_str(), O_WRONLY | O_CREAT, 0777);
    if (target_fd < 0) {
        return;
    }
    // 2. 申请缓存空间，进行数据读取并同步写入到磁盘中
    off_t idx = 0;
    while (idx < fsize) {
        size_t data_size = *(size_t *) init_mmap_file;
        init_mmap_file = (uint8_t *) init_mmap_file + sizeof(size_t);
        std::string output = "";
        snappy::Uncompress((const char *)init_mmap_file, data_size, &output);
        init_mmap_file = (uint8_t *) init_mmap_file + data_size;
        write(target_fd, output.c_str(), output.size());
        idx += sizeof(size_t) + data_size;
    }
    close(target_fd);
    close(init_fd);
}

snappy_compress_util::snappy_compress_util(std::string init_file_name, std::string target_file_name, int model) {
    this->_init_file_name = init_file_name;
    this->_target_file_name = target_file_name;
    this->_model = model;
}
