//
// Created by chengyanshang on 2022/10/15.
//

#ifndef JSI_TOOLKIT_RECORD_AREA_H
#define JSI_TOOLKIT_RECORD_AREA_H


// #define PREALLOCATE_BUFFSIZE 4096
// 2MB buffer size
#define PREALLOCATE_BUFFSIZE 2097152

#include<unistd.h>
#include<sys/types.h>
#include <map>
#include <string.h>
#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<aio.h>

#include "utils/jsi_log.h"
#include "utils/jsi_io.h"
#include "utils/snappy/snappy.h"
#include "record/record_type.h"
#include "record/record_defines.h"

class RecordArea {
private:
    // cache used Info
    unsigned long used, free;
    unsigned long bufferSize;

    // readOffset
    off_t readOffset;

    // Cache
    char *buffer[2];


    struct aiocb wr;

    int fd;

    bool flushed;

    int allocIndex;

    int asyncWriteIndex;

    int _model;

public:
    // Init the class with process rank
    RecordArea(const std::string& identifier,int model);

    ~RecordArea();

    // area flush
    void AreaFlush();


    // allocate a block to caller with size
    char *AreaAllocate(int size);

    void AreaFinalize();


    void RewriteBufferToFile(size_t len);

};


#endif //JSI_TOOLKIT_RECORD_AREA_H
