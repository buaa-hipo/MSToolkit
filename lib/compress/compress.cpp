//
// Created by chengyanshang on 2023/1/27.
//

#include "compress/compress.h"


HashCompress::HashCompress(std::string fileName) {
    this->fileFolderName = fileName;
    this->AvailableCompressedList = std::vector < int16_t > {
            event_MPI_Send,
            event_MPI_Recv,
            event_MPI_Isend,
            event_MPI_Irecv,
            event_MPI_Alltoall,
            event_MPI_Alltoallv,
            event_MPI_Allreduce,
            event_MPI_Reduce,
            event_MPI_Bcast
    };
}

void HashCompress::rewriteRecord(int rank, int idx, record_t *record) {
    size_t num_pmu_event_size = 0;
    if (record->MsgType != JSI_PROCESS_START && record->MsgType != JSI_PROCESS_EXIT) {
        num_pmu_event_size = sizeof(uint64_t) * 2 * this->recordReader->get_pmu_event_num_by_rank(rank);
    }
    size_t compressed_allocate_size = RecordHelper::get_compressed_record_size(record, this->recordReader->get_pmu_event_num_by_rank(rank));
    size_t init_struct_size = RecordHelper::get_record_size(record,0);
    size_t compress_struct_size = RecordHelper::get_compressed_record_size(record,0);
    uint8_t * init_start_ptr = (uint8_t *)record;
    uint8_t * compress_start_ptr = nullptr;
//    JSI_INFO("Start Find RecordType,RecordType:%d \n",record->MsgType);
//    JSI_INFO("compressed_allocate_size:%d,init_struct_size:%d,compress_struct_size:%d\n",compressed_allocate_size,init_struct_size,compress_struct_size);
    switch (record->MsgType) {
        case event_MPI_Send:
        case event_MPI_Recv: {
            record_comm_t *recordComm = (record_comm_t *) record;
            record_comm_compress_t *recordCompressed = (record_comm_compress_t * )this->rank2AreaMap[rank]->AreaAllocate(compressed_allocate_size);
            recordCompressed->record = recordComm->record;
            recordCompressed->tag = recordComm->tag;
            recordCompressed->dest = recordComm->dest;
            recordCompressed->idx = idx;
            recordCompressed->typesize = recordComm->typesize;
            compress_start_ptr = (uint8_t *)recordCompressed;
            break;
        }
        case event_MPI_Alltoall :
        case event_MPI_Alltoallv: {
            record_all2all_t *recordAll2All = (record_all2all_t *) record;
            record_all2all_compress_t *recordAll2AllCompress = (record_all2all_compress_t * )this->rank2AreaMap[rank]->AreaAllocate(compressed_allocate_size);
            recordAll2AllCompress->record = recordAll2All->record;
            recordAll2AllCompress->idx = idx;
            recordAll2AllCompress->recvcnt = recordAll2All->recvcnt;
            recordAll2AllCompress->typesize = recordAll2All->typesize;
            compress_start_ptr = (uint8_t *)recordAll2AllCompress;
            break;

        }
        case event_MPI_Allreduce: {
            record_allreduce_t *recordAllreduce = (record_allreduce_t *) record;
            record_allreduce_compress_t *recordAllreduceCompress = (record_allreduce_compress_t * )
            this->rank2AreaMap[rank]->AreaAllocate(compressed_allocate_size);
            recordAllreduceCompress->record = recordAllreduce->record;
            recordAllreduceCompress->idx = idx;
            recordAllreduceCompress->op = recordAllreduce->op;
            recordAllreduceCompress->typesize = recordAllreduce->typesize;
            compress_start_ptr = (uint8_t *)recordAllreduceCompress;
            break;

        }
        case event_MPI_Reduce: {
            record_reduce_t *recordReduce = (record_reduce_t *) record;
            record_reduce_compress_t *recordReduceCompress = (record_reduce_compress_t * )
            this->rank2AreaMap[rank]->AreaAllocate(compressed_allocate_size);
            recordReduceCompress->record = recordReduce->record;
            recordReduceCompress->idx = idx;
            recordReduceCompress->root = recordReduce->root;
            recordReduceCompress->op = recordReduce->op;
            recordReduceCompress->typesize = recordReduce->typesize;
            compress_start_ptr = (uint8_t *)recordReduceCompress;
            break;

        }
        case event_MPI_Bcast: {
            record_bcast_t *recordBcast = (record_bcast_t *) record;
            record_bcast_compress_t *recordBcastCompress = (record_bcast_compress_t * )
            this->rank2AreaMap[rank]->AreaAllocate(compressed_allocate_size);
            recordBcastCompress->record = recordBcast->record;
            recordBcastCompress->root = recordBcast->root;
            recordBcastCompress->idx = idx;
            recordBcastCompress->typesize = recordBcast->typesize;
            compress_start_ptr = (uint8_t *)(recordBcastCompress);
            break;

        }
        case event_MPI_Isend:
        case event_MPI_Irecv: {
            record_comm_async_t *recordCommAsync = (record_comm_async_t *) record;
            record_comm_async_compress_t *recordCommAsyncCompress = (record_comm_async_compress_t * )
            this->rank2AreaMap[rank]->AreaAllocate(compressed_allocate_size);
            recordCommAsyncCompress->record = recordCommAsync->record;
            recordCommAsyncCompress->idx = idx;
            recordCommAsyncCompress->dest = recordCommAsync->dest;
            recordCommAsyncCompress->tag = recordCommAsync->tag;
            recordCommAsyncCompress->request = recordCommAsync->request;
            recordCommAsyncCompress->typesize = recordCommAsync->typesize;
            compress_start_ptr = (uint8_t *)(recordCommAsyncCompress);
            break;
        }
        default: {
            record_t *new_record = (record_t * )(this->rank2AreaMap[rank]->AreaAllocate(compressed_allocate_size));
            memcpy(new_record, record, compressed_allocate_size);
            break;
        }
    }
    if (compress_start_ptr!=nullptr){
        memcpy(compress_start_ptr+compress_struct_size,init_start_ptr+init_struct_size,num_pmu_event_size);
    }
    return;
}


std::string HashCompress::makeSignature(record_t *record) {
    std::string sign;
    switch (record->MsgType) {
        case event_MPI_Send:
        case event_MPI_Recv: {
            record_comm_t *recordComm = (record_comm_t *) record;
            sign = std::to_string(recordComm->datatype) + "_" + std::to_string(recordComm->count) + "_" +
                   std::to_string(recordComm->comm);
            break;

        }
        case event_MPI_Reduce: {
            record_reduce_t *recordReduce = (record_reduce_t *) record;
            sign = std::to_string(recordReduce->datatype) + "_" + std::to_string(recordReduce->count) + "_" +
                   std::to_string(recordReduce->comm);
            break;

        }
        case event_MPI_Bcast: {
            record_bcast_t *recordBcast = (record_bcast_t *) record;
            sign = std::to_string(recordBcast->datatype) + "_" + std::to_string(recordBcast->count) + "_" +
                   std::to_string(recordBcast->comm);
            break;

        }
        case event_MPI_Alltoall:
        case event_MPI_Alltoallv: {
            record_all2all_t *recordAll2All = (record_all2all_t *) record;
            sign = std::to_string(recordAll2All->datatype) + "_" + std::to_string(recordAll2All->sendcnt) + "_" +
                   std::to_string(recordAll2All->comm);
            break;

        }
        case event_MPI_Allreduce: {
            record_allreduce_t *recordAllreduce = (record_allreduce_t *) record;
            sign = std::to_string(recordAllreduce->datatype) + "_" + std::to_string(recordAllreduce->count) + "_" +
                   std::to_string(recordAllreduce->comm);
            break;

        }
        case event_MPI_Isend:
        case event_MPI_Irecv: {
            record_comm_async_t *recordCommAsync = (record_comm_async_t *) record;
            sign = std::to_string(recordCommAsync->datatype) + "_" + std::to_string(recordCommAsync->count) + "_" +
                   std::to_string(recordCommAsync->comm);
            break;
        }
    }
    return sign;
}


void HashCompress::compressRecord(int rank, record_t *record) {
    auto iter = std::find(this->AvailableCompressedList.begin(), this->AvailableCompressedList.end(), record->MsgType);
    size_t idx = 0;
    if (iter != this->AvailableCompressedList.end()) {
        std::string sign = makeSignature(record);
        if (this->signMap.find(sign) == this->signMap.end()) {
            idx = this->signMap.size();
            this->signMap[sign] = idx;
        } else {
            idx = this->signMap[sign];
        }
    }
    rewriteRecord(rank, idx, record);
}

void HashCompress::compressFile() {
    // 0. 初始化Reader文件
    this->recordReader = new RecordReader(this->fileFolderName.c_str(), DATA_MODEL);
    // 1. 重写压缩文件
    JSI_INFO("RecordLoader Finished,Start Compress File\n");
    for (auto recordTrace: this->recordReader->get_all_traces()) {
        int rank = recordTrace.first;
        JSI_INFO("Rank %d Record Compress Start\n", rank);
        std::string identifier = std::to_string(rank);
        rank2AreaMap[rank] = new RecordArea(identifier, COMPRESS_MODEL);
        RecordTrace *trace = recordTrace.second;
        RecordTraceIterator it = trace->begin();
        while (it != trace->end()) {
            record_t *record = it.val();
            this->compressRecord(rank, record);
            it = it.next();
        }
        JSI_INFO("Rank %d Record Compress Finish\n", rank);
        rank2AreaMap[rank]->AreaFinalize();
    }
    // 2. 重写索引文件
    std::string identifier = std::to_string(COMMON_COMPRESS_RANK);
    rank2AreaMap[COMMON_COMPRESS_RANK] = new RecordArea(identifier, COMPRESS_MODEL);
    for (auto p: signMap) {
        std::string sign = p.first;
        std::vector <std::string> list = stringSplit(sign, '_');
        record_compress_t *recordCompress = (record_compress_t *) rank2AreaMap[COMMON_COMPRESS_RANK]->AreaAllocate(
                sizeof(record_compress_t));
        recordCompress->datatype = uint64_t(std::stoull(list[0].c_str()));
        recordCompress->count = std::stoull(list[1].c_str());
        recordCompress->comm = uint64_t(std::stoull(list[2].c_str()));
        recordCompress->idx = p.second;
    }
    JSI_INFO("Record Index File Rewrite Finish\n");
    // 3. 重写数据
    rank2AreaMap[COMMON_COMPRESS_RANK]->AreaFinalize();
    // 4. 删除所有标准写入文件
    deleteFileBySuffix(JSI_TRACE_FILE_EXT);
    JSI_INFO("Delete Origin Record File Finish\n");
}


void HashCompress::unCompressFile() {
    // 0. 解压所有的文件
    for (const auto &dirEntry: fs::directory_iterator(this->fileFolderName)) {
        if (dirEntry.is_regular_file()) {
            const auto &path = dirEntry.path();
            if (path.extension() == JSI_TRACE_WRITE_COMPRESS_EXT) {
                std::string rank_str(path.stem().extension().c_str() + 1);
                if (rank_str.size() > 0) {
                    std::string targetFileName = getTraceCompressReadFileName(rank_str);
                    snappy_compress_util *snappyCompressUtil = new snappy_compress_util(path.string(), targetFileName,
                                                                                        SNAPPYUNCOMPRESS);
                    snappyCompressUtil->snappy_process();
                }
            }
        }
    }
    JSI_INFO("UnCompress All File  Success!\n");
    // 0.读取Reader文件
    this->recordReader = new RecordReader(this->fileFolderName.c_str(), COMPRESS_MODEL);
    // 1.根据索引文件重建MsgType，key:index,value:Struct
    loadCommonStruct();
    JSI_INFO("loadCommonStruct Success!\n");
    // 2. 遍历具体的文件，挨个数据进行解压
    auto allTraceMap = this->recordReader->get_all_traces();
    for (auto p: allTraceMap) {
        int rank = p.first;
        std::string identifier = std::to_string(rank);
        JSI_INFO("Start Record Uncompress Rank:%d!\n",rank);
        rank2AreaMap[rank] = new RecordArea(identifier, DATA_MODEL);
        RecordTrace *trace = p.second;
        RecordTraceIterator it = trace->begin();
        RecordTraceIterator ie = trace->end();
        while (it != ie) {
            record_t *compress_t = it.val();
            decompressRecord(rank, compress_t);
            it = it.next();
        }
        JSI_INFO("Finish Record Uncompress Rank:%d!\n",rank);
        rank2AreaMap[rank]->AreaFinalize();
    }
//    // 3. 重写数据
//    for (auto p: rank2AreaMap) {
//        p.second->AreaFinalize();
//    }
    // 4. 删除所有的tmp和tar文件
    deleteFileBySuffix(JSI_TRACE_READ_COMPRESS_EXT);
    deleteFileBySuffix(JSI_TRACE_WRITE_COMPRESS_EXT);
    JSI_INFO("Delete Compress File Finished!\n");
}

void HashCompress::decompressRecord(int rank, record_t *record) {
    // JSI_INFO("COMPRESS RECORD, rank :%d ,RecordType:%d\n ",rank,record->MsgType);
    size_t num_pmu_event_size = 0;
    if (record->MsgType != JSI_PROCESS_START && record->MsgType != JSI_PROCESS_EXIT) {
        num_pmu_event_size = sizeof(uint64_t) * 2 * this->recordReader->get_pmu_event_num_by_rank(rank);
    }
    size_t init_allocate_size = RecordHelper::get_record_size(record, this->recordReader->get_pmu_event_num_by_rank(rank));
    size_t init_struct_size = RecordHelper::get_record_size(record,0);
    size_t compress_struct_size = RecordHelper::get_compressed_record_size(record,0);
    uint8_t * init_start_ptr = nullptr;
    uint8_t * compress_start_ptr = (uint8_t *)record;
    switch (record->MsgType) {
        case event_MPI_Send:
        case event_MPI_Recv: {
            record_comm_compress_t *recordCommCompress = (record_comm_compress_t *) record;
            record_comm_t *recordComm = (record_comm_t * )this->rank2AreaMap[rank]->AreaAllocate(init_allocate_size);
            recordComm->record = recordCommCompress->record;
            recordComm->tag = recordCommCompress->tag;
            recordComm->dest = recordCommCompress->dest;
            auto tmp = this->idx2CommonMap[recordCommCompress->idx];
            recordComm->datatype = tmp->datatype;
            recordComm->count = tmp->count;
            recordComm->comm = tmp->comm;
            recordComm->typesize = recordCommCompress->typesize;
            init_start_ptr = (uint8_t *) recordComm;
            break;

        }
        case event_MPI_Alltoall:
        case event_MPI_Alltoallv: {
            record_all2all_compress_t *recordAll2AllCompress1 = (record_all2all_compress_t *) record;
            record_all2all_t *recordAll2All = (record_all2all_t * )this->rank2AreaMap[rank]->AreaAllocate(init_allocate_size);
            recordAll2All->record = recordAll2AllCompress1->record;
            recordAll2All->recvcnt = recordAll2AllCompress1->recvcnt;
            recordAll2All->typesize = recordAll2AllCompress1->typesize;
            auto tmp = this->idx2CommonMap[recordAll2AllCompress1->idx];
            recordAll2All->datatype = tmp->datatype;
            recordAll2All->sendcnt = tmp->count;
            recordAll2All->comm = tmp->comm;
            init_start_ptr = (uint8_t *) recordAll2All;
            break;

        }
        case event_MPI_Allreduce: {
            record_allreduce_compress_t *recordAllreduceCompress1 = (record_allreduce_compress_t *) record;
            record_allreduce_t *recordAllreduce = (record_allreduce_t * )this->rank2AreaMap[rank]->AreaAllocate(init_allocate_size);
            recordAllreduce->record = recordAllreduceCompress1->record;
            recordAllreduce->op = recordAllreduceCompress1->op;
            recordAllreduce->typesize=recordAllreduceCompress1->typesize;
            auto tmp = this->idx2CommonMap[recordAllreduceCompress1->idx];
            recordAllreduce->datatype = tmp->datatype;
            recordAllreduce->count = tmp->count;
            recordAllreduce->comm = tmp->comm;
            init_start_ptr = (uint8_t *) recordAllreduce;
            break;

        }
        case event_MPI_Reduce: {
            record_reduce_compress_t *recordReduceCompress1 = (record_reduce_compress_t *) record;
            record_reduce_t *recordReduce = (record_reduce_t * )this->rank2AreaMap[rank]->AreaAllocate(init_allocate_size);
            recordReduce->record = recordReduceCompress1->record;
            recordReduce->root = recordReduceCompress1->root;
            recordReduce->op = recordReduceCompress1->op;
            recordReduce->typesize = recordReduceCompress1->typesize;
            auto tmp = this->idx2CommonMap[recordReduceCompress1->idx];
            recordReduce->datatype = tmp->datatype;
            recordReduce->count = tmp->count;
            recordReduce->comm = tmp->comm;
            init_start_ptr = (uint8_t *) recordReduce;
            break;

        }
        case event_MPI_Bcast: {
            record_bcast_compress_t *recordBcastCompress1 = (record_bcast_compress_t *) record;
            record_bcast_t *recordBcast = (record_bcast_t * )this->rank2AreaMap[rank]->AreaAllocate(init_allocate_size);
            recordBcast->record = recordBcastCompress1->record;
            recordBcast->root = recordBcastCompress1->root;
            recordBcast->typesize = recordBcastCompress1->typesize;
            auto tmp = this->idx2CommonMap[recordBcastCompress1->idx];
            recordBcast->datatype = tmp->datatype;
            recordBcast->count = tmp->count;
            recordBcast->comm = tmp->comm;
            init_start_ptr = (uint8_t *) recordBcast;
            break;

        }
        case event_MPI_Isend:
        case event_MPI_Irecv: {
            record_comm_async_compress_t *recordCommAsyncCompress1 = (record_comm_async_compress_t *) record;
            record_comm_async_t *recordCommAsync = (record_comm_async_t * )this->rank2AreaMap[rank]->AreaAllocate(init_allocate_size);
            recordCommAsync->record = recordCommAsyncCompress1->record;
            recordCommAsync->dest = recordCommAsyncCompress1->dest;
            recordCommAsync->tag = recordCommAsyncCompress1->tag;
            recordCommAsync->typesize = recordCommAsyncCompress1->typesize;
            recordCommAsync->request = recordCommAsyncCompress1->request;
            auto tmp = this->idx2CommonMap[recordCommAsyncCompress1->idx];
            recordCommAsync->datatype = tmp->datatype;
            recordCommAsync->count = tmp->count;
            recordCommAsync->comm = tmp->comm;
            init_start_ptr = (uint8_t *) recordCommAsync;
            break;

        }
        default: {
            record_t *new_record = (record_t * )(this->rank2AreaMap[rank]->AreaAllocate(init_allocate_size));
            memcpy(new_record, record, init_allocate_size);
            break;
        }
    }
    if (init_start_ptr!=nullptr){
        memcpy(init_start_ptr+init_struct_size,compress_start_ptr+compress_struct_size,num_pmu_event_size);
    }
    // JSI_INFO("COMPRESS RECORD Finished!! Success, rank :%d ,RecordType:%d\n ",rank,record->MsgType);
    return;
}

void HashCompress::loadCommonStruct() {
    std::string identifier = std::to_string(COMMON_COMPRESS_RANK);
    std::string common_filename = getTraceCompressReadFileName(identifier);
    int fd = open(common_filename.c_str(), O_RDONLY);
    off_t fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    void *common_file_mmap = mmap(0, fsize, PROT_READ, MAP_PRIVATE/*may use MAP_SHARED?*/, fd, 0);
    void *common_file_start = common_file_mmap;
    void *common_file_end = (void *) (((char *) common_file_start) + fsize);
    while (common_file_start != common_file_end) {
        record_compress_t *point = (record_compress_t *) malloc(sizeof(record_compress_t));
        memcpy(point, common_file_start, sizeof(record_compress_t));
        common_file_start = (void *) ((uint8_t *) common_file_start + sizeof(record_compress_t));
        this->idx2CommonMap[point->idx] = point;
    }
    munmap(common_file_mmap, fsize);
    return;
}

void HashCompress::deleteFileBySuffix(std::string suffix) {
    std::vector <std::string> waitDeleteFileList;
    for (const auto &dirEntry: fs::directory_iterator(this->fileFolderName)) {
        if (dirEntry.is_regular_file()) {
            const auto &path = dirEntry.path();
            if (path.extension() == suffix) {
                waitDeleteFileList.push_back(path.string());
            }
        }
    }
    for (std::string fileName: waitDeleteFileList) {
        remove(fileName.c_str());
    }
    return;
}