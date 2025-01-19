#include "memory_analysis.h"
#include "record/record_meta.h"
#include "instrument/backtrace.h"

MemoryAnalyzer::MemoryAnalyzer(RecordTraceCollection& traces, BacktraceCollection *backtraces, RankMetaCollection& metas)
    : traces(traces), backtraces(*backtraces), metas(metas){
    this->memory_usage_list = std::vector<memory_usage_item>();

    this->alloc_times = 0;
    this->free_times = 0;

    auto traces_vec = mapToVector(traces);
    int n = traces_vec.size();
    for (int i = 0; i < n; ++i)
    {
        auto trace = traces_vec[i];
        int rank = trace.first;
        // check_meta_info(rank);
        RecordTrace& rtrace = *(trace.second);
        init_recordtrace(rtrace,rank);
    }
}

MetaDataMap::MetaValue_t* MemoryAnalyzer::get_meta_value(const char *section, const char *key, int rank)
{
    RecordMeta *meta = metas[rank];
    auto &metaMap = meta->getMetaMap();
    if(metaMap.count(section) == 0)
    {
        return nullptr;
    }
    MetaDataMap *metaDataMap = metaMap.at(section);
    if(metaDataMap == nullptr)
    {
        return nullptr;
    }
    MetaDataMap::MetaValue_t *value;
    if(metaDataMap->get(key, &value) != MetaDataMap::UNKNOW)
    {
        return value;
    }
    return nullptr;
}

bool MemoryAnalyzer::check_record_source(backtrace_context_t &ctx, BacktraceTree &bt_tree)
{
    bool result = true;
    std::vector<const char *> vec;
    bt_tree.backtrace_get_context_string_vec(ctx, -1, vec);
    
    // std::cout << *(vec.rbegin() + 1) << "re " << (strstr(*(vec.rbegin() + 1),"_start_main") == NULL) << std::endl;
    // std::cout << vec[1] << "re " << (strstr(vec[1],"_IO_file_doallocate") == NULL) << std::endl;
    // std::cout << *(vec.rbegin()) << "re " << (strstr(*(vec.rbegin()),"ld-linux-x86-64.so")== NULL) <<std::endl;
    // 接受条件
    if(strstr(*(vec.rbegin() + 1),"_start_main") == NULL)
        result = false;

    // 拒绝条件
    if(strstr(vec[1],"_IO_file_doallocate") != NULL)
        result = false;
    if(strstr(*(vec.rbegin()),"ld-linux-x86-64.so") != NULL)
        result = false;
    return result;
}

void MemoryAnalyzer::init_recordtrace(RecordTrace& rtrace,int rank)
{
    uint64_t local_alloc_size = 0;
    uint64_t local_free_size = 0;
    ptr2size = std::unordered_map<uint64_t, memory_leak_item>();

    int i = 0;
    int count = 0;
    for(auto it=rtrace.begin(), ie=rtrace.end(); it!=ie; it=it.next())
    {
        record_t* r = it.val();
        if(r->MsgType == event_Memory_Malloc || r->MsgType == event_Memory_Calloc || r->MsgType == event_Memory_Realloc || r->MsgType == event_Memory_Free)
        {
            auto ctx = (backtrace_context_t) r->ctxt;
            auto &bt_tree = *backtraces[rank];

            if(!check_record_source(ctx,bt_tree))
            {
                continue;
            }
            count += 1;
            // show_record_info(rec,ctx,bt_tree,count);
            
            memory_usage_item mu;
            memory_leak_item ml;
            uint64_t old_size;
            
            // record_memory* rec = reinterpret_cast<record_memory*>(r);
            // switch (r->MsgType)
            // {
            // case event_Memory_Malloc:
                        //     break;
            // case event_Memory_Calloc:
            if(r->MsgType == event_Memory_Malloc)
            {
                record_memory_malloc* rec = reinterpret_cast<record_memory_malloc*>(r);
                alloc_times += 1;

                ml.size = rec->size_bytes;
                ml.ctxt = rec->record.ctxt;
                ptr2size.insert(std::make_pair((uint64_t)rec->ptr, ml));

                local_alloc_size += rec->size_bytes;
                mu.alloc_size = local_alloc_size;
                mu.free_size = local_free_size;
                mu.timestamps = rec->record.timestamps;
                memory_usage_list.push_back(mu);
            }
            else if(r->MsgType == event_Memory_Calloc)
            {
                record_memory_calloc* rec = reinterpret_cast<record_memory_calloc*>(r);
                alloc_times += 1;

                ml.size = rec->size_bytes;
                ml.ctxt = rec->record.ctxt;
                ptr2size.insert(std::make_pair((uint64_t)rec->ptr, ml));

                local_alloc_size += rec->size_bytes;
                mu.alloc_size = local_alloc_size;
                mu.free_size = local_free_size;
                mu.timestamps = rec->record.timestamps;
                memory_usage_list.push_back(mu);
            }
            else if(r->MsgType == event_Memory_Realloc)
            {
                record_memory_realloc* rec = reinterpret_cast<record_memory_realloc*>(r);
                alloc_times += 1;
                free_times += 1;
                old_size = ptr2size.find((uint64_t)rec->ptr)->second.size;
                ptr2size.erase((uint64_t)rec->ptr);

                ml.size = rec->size_bytes;
                ml.ctxt = rec->record.ctxt;
                ptr2size.insert(std::make_pair((uint64_t)rec->newptr, ml));

                local_alloc_size += rec->size_bytes;
                local_free_size += old_size;
                mu.alloc_size = local_alloc_size;
                mu.free_size = local_free_size;
                mu.timestamps = rec->record.timestamps;
                memory_usage_list.push_back(mu);
            }
            else if(r->MsgType == event_Memory_Free)
            {
                record_memory_free* rec = reinterpret_cast<record_memory_free*>(r);
                if(ptr2size.find((uint64_t)rec->ptr) == ptr2size.end())
                {
                    continue;
                }
                free_times += 1;
                old_size = ptr2size.find((uint64_t)rec->ptr)->second.size;
                ptr2size.erase((uint64_t)rec->ptr);

                local_free_size += old_size;
                mu.alloc_size = local_alloc_size;
                mu.free_size = local_free_size;
                mu.timestamps = rec->record.timestamps;
                memory_usage_list.push_back(mu);
            }
            else
            {
                printf("Unknown record type: %d\n", r->MsgType);
                exit(1);
            }
        }
    }
    analysis_usage(rank);
    analysis_memory_leak(rank);
}

void MemoryAnalyzer::analysis_usage(int rank)
{
    printf("Memory usage analysis:\n");
    int i = 0;
    int64_t memory_total =  get_meta_value("MEMORY_INFO","memory_total",rank)->i64;
    printf("    memory_total: %ld Bytes\n", memory_total*1000);
    for (auto mu : memory_usage_list)
    {
        i += 1;
        printf("    timestamps: %lu, alloc_size: %lu, free_size: %lu, memory_usage: %lu, usage_ratio: %lf\n", (mu.timestamps.enter+mu.timestamps.exit)/2, mu.alloc_size, mu.free_size, mu.alloc_size-mu.free_size, (mu.alloc_size-mu.free_size)*1.0/(memory_total*1000));
    }
    printf("\n");
}

void MemoryAnalyzer::analysis_memory_leak(int rank)
{
    printf("Memory leak analysis:\n\n");

    uint64_t leak_size = 0;
    int i = 0;
    int n = ptr2size.size();
    auto &bt_tree = *backtraces[rank];
    std::vector<const char *> vec;
    for (auto it : ptr2size)
    {
        i += 1;
        leak_size += it.second.size;
        printf("leak %lu bytes in 1 block  %d of %d\n", it.second.size, i, n);
        printf("    at context:\n");
        vec.clear();
        bt_tree.backtrace_get_context_string_vec(it.second.ctxt, -1, vec);
        for(auto it=vec.begin(); it!=vec.end(); it++)
        {
            printf("        %s\n", *it);
        }
        printf("\n");
    }

    printf("LEAK SUMMARY:");
    printf("    in use at exit: %lu bytes in %d blocks\n", leak_size, n);
    if(memory_usage_list.size() == 0)
        printf("    no heap usage info\n");
    else
        printf("    total heap usage: %lu allocs, %lu frees, %lu bytes allocated\n", alloc_times, free_times, memory_usage_list.rbegin()->alloc_size);
}


MemoryAnalyzer::~MemoryAnalyzer() {

}


void MemoryAnalyzer::check_meta_info(int rank)
{
    // metas
    printf("metas map size: %ld\n", metas.size());
    RecordMeta *meta = metas[rank];
    auto &metaMap = meta->getMetaMap();
    printf("metaMap size: %ld\n", metaMap.size());
    for(auto it : metaMap)
    {
        std::cout << "meta key: " << it.first << std::endl;
    }

    printf("=========meta info=========\n"); 
    std::cout << get_meta_value("MEMORY_INFO","rank",rank) << std::endl;
    printf("rank: %d\n", get_meta_value("MEMORY_INFO","rank",rank)->i32);
    printf("memory_total: %ld\n", get_meta_value("MEMORY_INFO","memory_total",rank)->i64);
    printf("memory_free: %ld\n", get_meta_value("MEMORY_INFO","memory_free",rank)->i64);
    printf("memory_available: %ld\n", get_meta_value("MEMORY_INFO","memory_available",rank)->i64);
    printf("allocate_time: %d\n", get_meta_value("MEMORY_INFO_END","allocate_time",rank)->i32);
    printf("=========meta info end=========\n");

    // MetaDataMap::MetaValue_t *item;
    // auto &metaDataMap = metaMap.at("MEMORY_INFO");
    // if(metaDataMap != nullptr)
    // {
    //     printf("=========meta info=========\n");
    //     metaDataMap->get("rank", &item);    
    //     printf("rank: %d\n", item->i32);
    //     metaDataMap->get("memory_total", &item);
    //     printf("memory_total: %ld\n", item->i64);
    //     metaDataMap->get("memory_free", &item);
    //     printf("memory_free: %ld\n", item->i64);
    //     metaDataMap->get("memory_available", &item);
    //     printf("memory_available: %ld\n", item->i64);
    //     printf("=========meta info end=========\n");
    // }
    // auto &metaDataMap2 = metaMap.at("MEMORY_INFO_END");
    // if(metaDataMap2 != nullptr)
    // {
    //     printf("=========meta info=========\n");
    //     metaDataMap2->get("allocate_time", &item);    
    //     printf("allocate_time: %d\n", item->i32);
    //     printf("=========meta info end=========\n");
    // }
}

// void show_record_info(record_memory* rec, backtrace_context_t &ctx, BacktraceTree &bt_tree,int count)
// {
//     std::cout << "count: " << count << "    Type: " << rec->type << std::endl;

//     if(rec->type == record_memory::MALLOC)
//     {
//         std::cout << "MALLOC ptr : " << rec->param.malloc.ptr << std::endl;
//         std::cout << "MALLOC size : " << rec->param.malloc.size_bytes << std::endl;
//     }
//     if(rec->type == record_memory::CALLOC)
//     {
//         std::cout << "CALLOC ptr : " << rec->param.calloc.ptr << std::endl;
//         std::cout << "CALLOC size : " << rec->param.calloc.size_bytes << std::endl;
//     }
//     if(rec->type == record_memory::REALLOC)
//     {
//         std::cout << "REALLOC ptr : " << rec->param.realloc.ptr << std::endl;
//         std::cout << "REALLOC newptr : " << rec->param.realloc.newptr << std::endl;
//         std::cout << "REALLOC size : " << rec->param.realloc.size_bytes << std::endl;
//     }
//     if(rec->type == record_memory::FREE)
//     {
//         std::cout << "FREE ptr : " << rec->param.free.ptr << std::endl;
//     }

//     // std::cout << "ctx: " << bt_tree.backtrace_get_context_string(ctx) << std::endl;
//     // std::cout << "node ctx: " << bt_tree.backtrace_get_node_string(ctx) << std::endl;
//     // auto parent_ctx = bt_tree.get_parent(ctx);
//     // std::cout << "parent ctx: " << bt_tree.backtrace_get_context_string(parent_ctx) << std::endl;

//     std::vector<const char *> vec;
//     bt_tree.backtrace_get_context_string_vec(ctx, -1, vec);
//     for(auto it=vec.begin(); it!=vec.end(); it++)
//     {
//         std::cout << "vec: " << *it << std::endl;
//     }
//     std::cout << std::endl;
// }