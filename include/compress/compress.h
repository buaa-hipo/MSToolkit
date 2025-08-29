//
// Created by chengyanshang on 2023/1/27.
//

#ifndef JSI_TOOLKIT_COMPRESS_COMPRESS
#define JSI_TOOLKIT_COMPRESS_COMPRESS

#include <string.h>
#include<algorithm>

// #include "record/record_writer.h"
#include "record/record_area.h"
#include "record/record_reader.h"
#include "record/record_defines.h"
#include "snappy_compress.h"
#include "utils/common_util.h"



class HashCompress {
private:
    RecordReader * recordReader;

    std::vector<int16_t> AvailableCompressedList;

    std::string fileFolderName;

    std::map<std::string, int64_t> signMap;

    std::map<int64_t ,record_compress_t*> idx2CommonMap;

    std::map<std::int64_t, RecordArea *> rank2AreaMap;


    void compressRecord(int rank, record_t *record);

    void decompressRecord(int rank,record_t * record);

    std::string makeSignature(record_t *record);

    void rewriteRecord(int rank,int idx,record_t *record);

    void loadCommonStruct();

    void deleteFileBySuffix(std::string suffix);

public:
    HashCompress(std::string fileName);

    void compressFile();

    void unCompressFile();
};


#endif
