//
// Created by chengyanshang on 2022/10/15.
//

#include "record/record_area.h"

RecordArea::RecordArea(const std::string& identifier, int model) {
    this->used = 0;
    this->readOffset = 0;
    this->free = PREALLOCATE_BUFFSIZE;
    this->bufferSize = PREALLOCATE_BUFFSIZE;
    this->buffer[0] = new char[PREALLOCATE_BUFFSIZE];
    this->buffer[1] = new char[PREALLOCATE_BUFFSIZE];
    // Todo:Record索引文件初始化 文件名获取
    // Init AIO Config
    bzero(&this->wr, sizeof(wr));
    std::string fn = "";
    if (model == DATA_MODEL) {
        fn = getTraceFileName(identifier);
    } else if (model == COMPRESS_MODEL) {
        fn = getTraceCompressWriteFileName(identifier);
    }
    fd = open(fn.c_str(), O_RDWR | O_CREAT | O_APPEND | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        JSI_ERROR("open file failed,err :%s\n", fn.c_str());
    }
    wr.aio_fildes = fd;
    this->flushed = false;
    allocIndex = 0;
    asyncWriteIndex = -1;
    this->_model = model;
}


RecordArea::~RecordArea() {
    this->AreaFinalize();
    delete (buffer[0]);
    delete (buffer[1]);
}

char *RecordArea::AreaAllocate(int size) {
    if (size > this->free) {
        RewriteBufferToFile(used);
        this->used = 0;
        this->free = PREALLOCATE_BUFFSIZE;
    }
    char *p = this->buffer[allocIndex] + used;
    this->used += size;
    this->free -= size;
    return p;
}

void RecordArea::RewriteBufferToFile(size_t len) {
    asyncWriteIndex = allocIndex;
    // hard code
    allocIndex = 1 - allocIndex;
    while (flushed && aio_error(&wr) == EINPROGRESS) {}
    if (flushed && aio_return(&wr) < 0) {
        JSI_ERROR("AIO return Err");
    }
    if (this->_model == COMPRESS_MODEL) {
        std::string output = "";
        snappy::Compress(buffer[asyncWriteIndex], len, &output);
        // 1. 先写数据大小
        char *cache = (char *) malloc(sizeof(size_t) + output.size());
        char *tmp = cache;
        size_t data_size = output.size();
        memcpy(tmp, &data_size, sizeof(data_size));
        // 2. 再写数据本身
        tmp = tmp + sizeof data_size;
        memcpy(tmp, output.c_str(), data_size);
        this->wr.aio_buf = cache;
        this->wr.aio_nbytes = sizeof(size_t) + data_size;
    } else {
        this->wr.aio_buf = buffer[asyncWriteIndex];
        this->wr.aio_nbytes = len;
    }
    int res = aio_write(&wr);
    if (res != 0) {
        JSI_WARN("aio_write failed,res value: %s", std::to_string(res).c_str());
    }
    flushed = true;
}


void RecordArea::AreaFlush() {
    if (this->used <= 0) {
        return;
    }
    RewriteBufferToFile(used);
    this->used = 0;
    this->free = PREALLOCATE_BUFFSIZE;
}


void RecordArea::AreaFinalize() {
    this->AreaFlush();
    while (aio_error(&wr) == EINPROGRESS) {}
    close(fd);
}