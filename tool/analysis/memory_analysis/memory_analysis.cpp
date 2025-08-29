#include "memory_analysis.h"
#include "record/record_meta.h"
#include "instrument/backtrace.h"

MemoryAnalyzer::MemoryAnalyzer(RecordTraceCollection& traces, BacktraceCollection *backtraces, RankMetaCollection& metas, std::string output_dir, bool pretty_print, bool enable_check_source, bool enable_mt, double mt_percent,bool enable_general,bool enable_accl,bool enable_usage,bool enable_leak,int rank_offset,int rank_num)
    : traces(traces), backtraces(*backtraces), metas(metas), output_dir(output_dir), pretty_print(pretty_print), enable_check_source(enable_check_source), enable_mt(enable_mt), mt_percent(mt_percent), enable_general(enable_general), enable_accl(enable_accl), enable_usage(enable_usage), enable_leak(enable_leak),rank_offset(rank_offset),rank_num(rank_num){
    
    // for (auto* ptr : this->trace_data_list) {
    //     delete ptr;
    // }
    // this->trace_data_list.clear();
    // this->trace_data_list = std::vector<MemoryAnalyzerTraceData*>();
    // this->trace_data_list_accl = std::vector<MemoryAnalyzerTraceData*>();
    this->trace_data_list = std::vector<std::unique_ptr<MemoryAnalyzerTraceData>>();
    this->trace_data_list_accl = std::vector<std::unique_ptr<MemoryAnalyzerTraceData>>();

    std::vector<RecordTrace *> all_rt_list;
    std::vector<int> all_rank_list;
    std::vector<std::string> all_id_list;
    for (auto it = traces.begin(); it != traces.end(); ++it) 
    {
        all_rt_list.push_back(it->second);
        all_rank_list.push_back(it->second->rank());
        all_id_list.push_back(it->first);
    }
    if(rank_num == 0)
        rank_num = all_rank_list.size();

    int round_num = (all_rt_list.size() + max_round_size - 1)/ max_round_size;
    int round_index = 0;
    for(round_index = 0; round_index < round_num; ++round_index)
    {
        int start = round_index * max_round_size;
        int end = std::min(start + max_round_size, (int)all_rt_list.size());   
        current_round_size = end - start;
        round_completed_size = start;
        const auto rt_list = std::vector<RecordTrace*>(
            all_rt_list.begin() + start,
            all_rt_list.begin() + end
        );
        const auto rank_list = std::vector<int>(
            all_rank_list.begin() + start,
            all_rank_list.begin() + end
        );
        const auto id_list = std::vector<std::string>(
            all_id_list.begin() + start,
            all_id_list.begin() + end
        );
        //清理上一轮内存
        // for (auto& ptr : this->trace_data_list) {
        //     delete ptr.get();
        // }
        this->trace_data_list.clear();
        this->trace_data_list.shrink_to_fit();
        // for (auto& ptr : this->trace_data_list_accl) {
        //     delete ptr.get();
        // }
        this->trace_data_list_accl.clear();
        this->trace_data_list_accl.shrink_to_fit();


        if(!enable_mt)
        {
            for(int i = 0; i < rt_list.size(); ++i)
            {
                RecordTrace* rt = rt_list[i];
                int rank = rank_list[i];
                std::string id = id_list[i];
                // printf("InitRecordTrace rank%4d, %4d / %d\n",rank,i + 1 + round_completed_size,(int)all_rt_list.size());
                printf("InitRecordTrace rank%4d, %4d / %d\n",rank_offset + rank,rank_offset + i + 1 + round_completed_size,rank_num);
                // MemoryAnalyzerTraceData* trace_data = init_recordtrace(*rt,rank,id);
                // this->trace_data_list.push_back(trace_data);
                // std::map<std::string,MemoryAnalyzerTraceData*> trace_data_map = init_recordtrace(*rt,rank,id);
                std::map<std::string,std::unique_ptr<MemoryAnalyzerTraceData>> trace_data_map =  std::move(init_recordtrace(*rt,rank,id));
                // this->trace_data_list.push_back(trace_data_map["general"]);
                // this->trace_data_list_accl.push_back(trace_data_map["accl"]);
                this->trace_data_list.push_back(std::move(trace_data_map["general"]));
                this->trace_data_list_accl.push_back(std::move(trace_data_map["accl"]));
            }
        }
        else
        {
            // 多线程处理部分
            int total = rt_list.size();
            int num_threads = std::max(1, (int)(static_cast<double>(std::thread::hardware_concurrency()) * mt_percent));
            int chunk_size = (total + num_threads - 1) / num_threads; // 向上取整
            std::vector<std::thread> threads;
            // std::vector<std::vector<MemoryAnalyzerTraceData*>> local_results(num_threads);
            // std::vector<std::vector<MemoryAnalyzerTraceData*>> local_results_accl(num_threads);
            std::vector<std::vector<std::unique_ptr<MemoryAnalyzerTraceData>>> local_results(num_threads);
            std::vector<std::vector<std::unique_ptr<MemoryAnalyzerTraceData>>> local_results_accl(num_threads);
            std::mutex print_mutex;
            std::atomic<int> exit_index(1);
    
            for (int t = 0; t < num_threads; ++t) {
                int start = t * chunk_size;
                int end = std::min(start + chunk_size, total);
                int all_size = all_rt_list.size();
                threads.emplace_back([this, start, end, t, &rt_list, &rank_list, &id_list, total, &local_results, &local_results_accl,&print_mutex,&exit_index,rank_offset,rank_num]() {
                    // std::vector<MemoryAnalyzerTraceData*> local_vec;
                    // std::vector<MemoryAnalyzerTraceData*> local_vec_accl;
                    std::vector<std::unique_ptr<MemoryAnalyzerTraceData>> local_vec;
                    std::vector<std::unique_ptr<MemoryAnalyzerTraceData>> local_vec_accl;
                    for (int i = start; i < end; ++i) {
                        RecordTrace* rt = rt_list[i];
                        int rank = rank_list[i];
                        std::string id = id_list[i];
                        {
                            // 保护printf输出
                            std::lock_guard<std::mutex> lock(print_mutex);
                            printf("InitRecordTrace rank%4d\n", rank + rank_offset);
                        }
                        // std::map<std::string,MemoryAnalyzerTraceData*> trace_data_map = init_recordtrace(*rt, rank, id);
                        std::map<std::string,std::unique_ptr<MemoryAnalyzerTraceData>> trace_data_map = init_recordtrace(*rt, rank, id);
                        // local_vec.push_back(trace_data_map["general"]);
                        // local_vec_accl.push_back(trace_data_map["accl"]);
                        local_vec.push_back(std::move(trace_data_map["general"]));
                        local_vec_accl.push_back(std::move(trace_data_map["accl"]));
                        {
                            // 保护printf输出
                            std::lock_guard<std::mutex> lock(print_mutex);
                            printf("InitRecordTrace end rank%4d, %4d / %d\n", rank + rank_offset, exit_index.load() + round_completed_size + rank_offset, rank_num);
                            exit_index.store(exit_index.load() + 1);
                        }
                    }
                    local_results[t] = std::move(local_vec);
                    local_results_accl[t] = std::move(local_vec_accl);
                });
            }
    
            // 等待所有线程完成
            for (auto& thread : threads) {
                thread.join();
            }
    
            // // 合并结果到trace_data_list
            // for (auto& vec : local_results) {
            //     this->trace_data_list.insert(this->trace_data_list.end(), vec.begin(), vec.end());
            // }
            // for (auto& vec : local_results_accl) {
            //     this->trace_data_list_accl.insert(this->trace_data_list_accl.end(),vec.begin(),vec.end());
            // }
            // 合并 general 数据
            for (auto& vec : local_results) {
                this->trace_data_list.insert(
                    this->trace_data_list.end(),
                    std::make_move_iterator(vec.begin()),
                    std::make_move_iterator(vec.end())
                );
                vec.clear();  // 必须清空
                vec.shrink_to_fit();
            }

            // 合并 accl 数据
            for (auto& vec : local_results_accl) {
                this->trace_data_list_accl.insert(
                    this->trace_data_list_accl.end(),
                    std::make_move_iterator(vec.begin()),
                    std::make_move_iterator(vec.end())
                );
                vec.clear();
                vec.shrink_to_fit();
            }

            local_results.clear();
            local_results.shrink_to_fit();
            local_results_accl.clear();
            local_results_accl.shrink_to_fit();
        }


        if(enable_general)
        {
            if(enable_usage) {
                this->analysis_usage("general");
            }
            if(enable_leak) {
                this->analysis_memory_leak("general");
            }
        }
        if(enable_accl)
        {
            if(enable_usage) {
                this->analysis_usage("accl");
            }
            if(enable_leak) {
                this->analysis_memory_leak("accl");
            }
        }
    }

    // std::vector<RecordTrace *> rt_list;
    // std::vector<int> rank_list;
    // std::vector<std::string> id_list;
    // for (auto it = traces.begin(); it != traces.end(); ++it) 
    // {
    //     rt_list.push_back(it->second);
    //     rank_list.push_back(it->second->rank());
    //     id_list.push_back(it->first);
    // }
    // record_trace_num = rt_list.size();//


    // if(!enable_mt)
    // {
    //     for(int i = 0; i < rt_list.size(); ++i)
    //     {
    //         RecordTrace* rt = rt_list[i];
    //         int rank = rank_list[i];
    //         std::string id = id_list[i];
    //         printf("InitRecordTrace rank%4d, %4d / %d\n",rank,i + 1,(int)rt_list.size());
    //         // MemoryAnalyzerTraceData* trace_data = init_recordtrace(*rt,rank,id);
    //         // this->trace_data_list.push_back(trace_data);
    //         std::map<std::string,MemoryAnalyzerTraceData*> trace_data_map = init_recordtrace(*rt,rank,id);
    //         this->trace_data_list.push_back(trace_data_map["general"]);
    //         this->trace_data_list_accl.push_back(trace_data_map["accl"]);
    //     }
    // }
    // else
    // {
    //     // 多线程处理部分
    //     int total = rt_list.size();
    //     int num_threads = std::max(1, (int)(static_cast<double>(std::thread::hardware_concurrency()) * mt_percent));
    //     int chunk_size = (total + num_threads - 1) / num_threads; // 向上取整
    //     std::vector<std::thread> threads;
    //     std::vector<std::vector<MemoryAnalyzerTraceData*>> local_results(num_threads);
    //     std::vector<std::vector<MemoryAnalyzerTraceData*>> local_results_accl(num_threads);
    //     std::mutex print_mutex;
    //     std::atomic<int> exit_index(1);

    //     for (int t = 0; t < num_threads; ++t) {
    //         int start = t * chunk_size;
    //         int end = std::min(start + chunk_size, total);
    //         threads.emplace_back([this, start, end, t, &rt_list, &rank_list, &id_list, total, &local_results, &local_results_accl,&print_mutex,&exit_index]() {
    //             std::vector<MemoryAnalyzerTraceData*> local_vec;
    //             std::vector<MemoryAnalyzerTraceData*> local_vec_accl;
    //             for (int i = start; i < end; ++i) {
    //                 RecordTrace* rt = rt_list[i];
    //                 int rank = rank_list[i];
    //                 std::string id = id_list[i];
    //                 {
    //                     // 保护printf输出
    //                     std::lock_guard<std::mutex> lock(print_mutex);
    //                     printf("InitRecordTrace rank%4d\n", rank);
    //                 }
    //                 std::map<std::string,MemoryAnalyzerTraceData*> trace_data_map = init_recordtrace(*rt, rank, id);
    //                 local_vec.push_back(trace_data_map["general"]);
    //                 local_vec_accl.push_back(trace_data_map["accl"]);
    //                 {
    //                     // 保护printf输出
    //                     std::lock_guard<std::mutex> lock(print_mutex);
    //                     printf("InitRecordTrace end rank%4d, %4d / %d\n", rank, exit_index.load(), total);
    //                     exit_index.store(exit_index.load() + 1);
    //                 }
    //             }
    //             local_results[t] = std::move(local_vec);
    //             local_results_accl[t] = std::move(local_vec_accl);
    //         });
    //     }

    //     // 等待所有线程完成
    //     for (auto& thread : threads) {
    //         thread.join();
    //     }

    //     // 合并结果到trace_data_list
    //     for (auto& vec : local_results) {
    //         this->trace_data_list.insert(this->trace_data_list.end(), vec.begin(), vec.end());
    //     }
    //     for (auto& vec : local_results_accl) {
    //         this->trace_data_list_accl.insert(this->trace_data_list_accl.end(),vec.begin(),vec.end());
    //     }
    // }


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

bool MemoryAnalyzer::accept_record_source(backtrace_context_t &ctx, BacktraceTree &bt_tree)
{
    bool result = true;
    std::vector<const char *> vec;
    bt_tree.backtrace_get_context_string_vec(ctx, -1, vec);
    
    // std::cout << *(vec.rbegin() + 1) << "re " << (strstr(*(vec.rbegin() + 1),"_start_main") == NULL) << std::endl;
    // std::cout << vec[1] << "re " << (strstr(vec[1],"_IO_file_doallocate") == NULL) << std::endl;
    // std::cout << *(vec.rbegin()) << "re " << (strstr(*(vec.rbegin()),"ld-linux-x86-64.so")== NULL) <<std::endl;
    // 拒绝 not found
    if(strstr(*(vec.rbegin() + 1),"_start_main") == NULL)
        result = false;

    // 拒绝 found
    for(auto str : vec)
    {
        if(strstr(str,"libmpi_wrapper.so") != NULL)
        {
            result = false;
            return result;
        }
    }
    if(strstr(vec[1],"_IO_file_doallocate") != NULL)
        result = false;
    if(strstr(*(vec.rbegin()),"ld-linux-x86-64.so") != NULL)
        result = false;

    return result;
}

// std::map<std::string,MemoryAnalyzerTraceData*>  MemoryAnalyzer::init_recordtrace(RecordTrace& rtrace,int rank,std::string id)
std::map<std::string, std::unique_ptr<MemoryAnalyzerTraceData>> MemoryAnalyzer::init_recordtrace(RecordTrace& rtrace,int rank,std::string id)
{
    // MemoryAnalyzerTraceData * trace_data = new MemoryAnalyzerTraceData(rank,id);
    // MemoryAnalyzerTraceData * trace_data_accl = new MemoryAnalyzerTraceData(rank,id);
    auto trace_data = std::make_unique<MemoryAnalyzerTraceData>(rank, id);
    auto trace_data_accl = std::make_unique<MemoryAnalyzerTraceData>(rank, id);

    int i = 0;
    uint64_t old_size;

    for(auto it=rtrace.begin(), ie=rtrace.end(); it!=ie; it=it.next())
    {
        record_t* r = it.val();
        if(enable_general && (r->MsgType == event_Memory_Malloc || r->MsgType == event_Memory_Calloc || r->MsgType == event_Memory_Realloc || r->MsgType == event_Memory_Free))
        {
            auto& ptr2size = trace_data->ptr2size;
            auto& memory_usage_list = trace_data->memory_usage_list;
            auto& alloc_times = trace_data->alloc_times;
            auto& free_times = trace_data->alloc_times;
            auto& count = trace_data->event_num;
            auto& all_count = trace_data->all_event_num;
            uint64_t local_alloc_size = memory_usage_list.size() > 0 ? memory_usage_list.back().alloc_size : 0;
            uint64_t local_free_size = memory_usage_list.size() > 0 ? memory_usage_list.back().free_size : 0;


            all_count += 1;
            auto ctx = (backtrace_context_t) r->ctxt;
            auto &bt_tree = *backtraces[id];
            
            if(enable_check_source)
            {
                if(!accept_record_source(ctx,bt_tree))
                {
                    continue;
                }
            }

            count += 1;
            // show_record_info(rec,ctx,bt_tree,count);
            
            MemoryAnalyzerTraceData::memory_usage_item mu;
            MemoryAnalyzerTraceData::memory_leak_item ml;
            if(r->MsgType == event_Memory_Malloc)
            {
                record_memory_malloc* rec = reinterpret_cast<record_memory_malloc*>(r);

                //illegal check
                if(ptr2size.find((uint64_t)rec->ptr) != ptr2size.end())
                {
                    // printf("ptr: %p has been allocated\n", rec->ptr);
                    continue;
                }
                //illegal check end

                // printf("malloc size: %lu ptr: %p\n", rec->size_bytes, rec->ptr);
                alloc_times += 1;

                // mem leak
                ml.size = rec->size_bytes;
                ml.ctxt = rec->record.ctxt;
                ptr2size.insert(std::make_pair((uint64_t)rec->ptr, ml));

                //mem usage
                local_alloc_size += rec->size_bytes;
                mu.alloc_size = local_alloc_size;
                mu.free_size = local_free_size;
                mu.timestamps = rec->record.timestamps;
                memory_usage_list.push_back(mu);
            }
            else if(r->MsgType == event_Memory_Calloc)
            {
                record_memory_calloc* rec = reinterpret_cast<record_memory_calloc*>(r);

                //illegal check
                if(ptr2size.find((uint64_t)rec->ptr) != ptr2size.end())
                {
                    // printf("ptr: %p has been allocated\n", rec->ptr);
                    continue;
                }
                //illegal check end

                alloc_times += 1;
                // printf("calloc size: %lu ptr: %p\n", rec->size_bytes, rec->ptr);

                // mem leak
                ml.size = rec->size_bytes;
                ml.ctxt = rec->record.ctxt;
                ptr2size.insert(std::make_pair((uint64_t)rec->ptr, ml));

                //mem usage
                local_alloc_size += rec->size_bytes;
                mu.alloc_size = local_alloc_size;
                mu.free_size = local_free_size;
                mu.timestamps = rec->record.timestamps;
                memory_usage_list.push_back(mu);
            }
            else if(r->MsgType == event_Memory_Realloc)
            {
                record_memory_realloc* rec = reinterpret_cast<record_memory_realloc*>(r);
                // printf("realloc size: %lu ptr: %p newptr: %p\n", rec->size_bytes, rec->ptr, rec->newptr);

                //illegal check
                if(rec->ptr != NULL && ptr2size.find((uint64_t)rec->ptr) == ptr2size.end())
                {
                    continue;
                }
                //illegal check end

                if(rec->ptr == NULL)
                {
                    alloc_times += 1;

                    //mem leak
                    ml.size = rec->size_bytes;
                    ml.ctxt = rec->record.ctxt;
                    ptr2size.insert(std::make_pair((uint64_t)rec->newptr, ml));

                    //mem usage
                    local_alloc_size += rec->size_bytes;
                    mu.alloc_size = local_alloc_size;
                    mu.free_size = local_free_size;
                    mu.timestamps = rec->record.timestamps;
                    memory_usage_list.push_back(mu);
                }
                else
                {
                    alloc_times += 1;
                    free_times += 1;
                    old_size = ptr2size.find((uint64_t)rec->ptr)->second.size;
                    ptr2size.erase((uint64_t)rec->ptr);

                    // mem leak
                    ml.size = rec->size_bytes;
                    ml.ctxt = rec->record.ctxt;
                    ptr2size.insert(std::make_pair((uint64_t)rec->newptr, ml));

                    //mem usage
                    local_alloc_size += rec->size_bytes;
                    local_free_size += old_size;
                    mu.alloc_size = local_alloc_size;
                    mu.free_size = local_free_size;
                    mu.timestamps = rec->record.timestamps;
                    memory_usage_list.push_back(mu);
                }

            }
            else if(r->MsgType == event_Memory_Free)
            {
                record_memory_free* rec = reinterpret_cast<record_memory_free*>(r);
                // printf("free ptr: %p\n", rec->ptr);
                
                //illegal check
                if(rec->ptr == NULL)
                {
                    continue;
                }
                if(ptr2size.find((uint64_t)rec->ptr) == ptr2size.end())
                {
                    continue;
                }
                //illegal check end

                // mem leak
                free_times += 1;
                old_size = ptr2size.find((uint64_t)rec->ptr)->second.size;
                ptr2size.erase((uint64_t)rec->ptr);

                //mem usage
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

        //accl
        if(enable_accl && (r->MsgType == event_ACCL_API_malloc || r->MsgType == event_ACCL_API_free))
        {
            auto& ptr2size = trace_data_accl->ptr2size;
            auto& memory_usage_list = trace_data_accl->memory_usage_list;
            auto& alloc_times = trace_data_accl->alloc_times;
            auto& free_times = trace_data_accl->alloc_times;
            auto& count = trace_data_accl->event_num;
            auto& all_count = trace_data_accl->all_event_num;
            uint64_t local_alloc_size = memory_usage_list.size() > 0 ? memory_usage_list.back().alloc_size : 0;
            uint64_t local_free_size = memory_usage_list.size() > 0 ? memory_usage_list.back().free_size : 0;


            all_count += 1;
            auto ctx = (backtrace_context_t) r->ctxt;
            auto &bt_tree = *backtraces[id];
            
            if(enable_check_source)
            {
                if(!accept_record_source(ctx,bt_tree))
                {
                    continue;
                }
            }

            count += 1;
            // show_record_info(rec,ctx,bt_tree,count);
            
            MemoryAnalyzerTraceData::memory_usage_item mu;
            MemoryAnalyzerTraceData::memory_leak_item ml;

            if(r->MsgType == event_ACCL_API_malloc)
            {
                mt_record_malloc_t * rec = reinterpret_cast<mt_record_malloc_t*>(r);

                //illegal check
                if(ptr2size.count((uint64_t)rec->address) != 0)
                {
                    // printf("ptr: %p has been allocated\n", rec->ptr);
                    continue;
                }
                //illegal check end

                // printf("malloc size: %lu ptr: %p\n", rec->size_bytes, rec->ptr);
                alloc_times += 1;

                // mem leak
                ml.size = (uint64_t)rec->bytes;
                ml.ctxt = rec->record.ctxt;
                ptr2size[(uint64_t)rec->address] = ml;

                //mem usage
                local_alloc_size += (uint64_t)rec->bytes;
                mu.alloc_size = local_alloc_size;
                mu.free_size = local_free_size;
                mu.timestamps = rec->record.timestamps;
                memory_usage_list.push_back(mu);
            }
            else if(r->MsgType == event_ACCL_API_free)
            {
                mt_record_free_t *rec = reinterpret_cast<mt_record_free_t*>(r);
                // printf("free ptr: %p\n", rec->ptr);
                
                //illegal check
                if(rec->address == NULL)
                {
                    continue;
                }
                if(ptr2size.count((uint64_t)rec->address) == 0)
                {
                    continue;
                }
                //illegal check end

                // mem leak
                free_times += 1;
                old_size = ptr2size[((uint64_t)rec->address)].size;
                ptr2size.erase((uint64_t)rec->address);

                //mem usage
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

    // return std::map<std::string,MemoryAnalyzerTraceData*>{{"general",trace_data},{"accl",trace_data_accl}};
    // return {{"general", std::move(trace_data)}, {"accl", std::move(trace_data_accl)}};
    std::map<std::string, std::unique_ptr<MemoryAnalyzerTraceData>> result;
    result.emplace("general", std::move(trace_data));
    result.emplace("accl", std::move(trace_data_accl));
    return result;
    // analysis_usage(rank,count);
    // analysis_memory_leak(rank,count);
}

void MemoryAnalyzer::analysis_usage(std::string trace_data_name)
{
    // std::vector<MemoryAnalyzerTraceData*> &local_trace_data_list = (trace_data_name == "general") ? this->trace_data_list : this->trace_data_list_accl;
    auto &local_trace_data_list = (trace_data_name == "general") ? this->trace_data_list : this->trace_data_list_accl;

    printf("\n========================================================\n");
    if(trace_data_name == "general")
        printf("Memory usage analysis Start:\n");
    else
        printf("Memory usage analysis for Acceleration Domain Start:\n");
    printf("========================================================\n\n");
    try{
        std::mutex print_mutex;

        if(!enable_mt)
        {
            for(auto &it : local_trace_data_list)
            {
                analysis_usage_trace(it.get(),print_mutex,trace_data_name);
            }
        }
        else
        {
            // 多线程处理部分
            // int total = record_trace_num;
            int total = current_round_size;
            int num_threads = std::max(1, (int)(static_cast<double>(std::thread::hardware_concurrency()) * mt_percent));
            int chunk_size = (total + num_threads - 1) / num_threads; // 向上取整
            std::vector<std::thread> threads;
            std::atomic<int> exit_index(1);
    
            for (int t = 0; t < num_threads; ++t) {
                int start = t * chunk_size;
                int end = std::min(start + chunk_size, total);
                threads.emplace_back([this, &local_trace_data_list,start, end ,&print_mutex,&exit_index,trace_data_name]() {
                    for (int i = start; i < end; ++i) {
                        analysis_usage_trace(local_trace_data_list[i].get(),print_mutex,trace_data_name);
                    }
                });
            }
            // 等待所有线程完成
            for (auto& thread : threads) {
                thread.join();
            }
        }

        printf("\n========================================================\n");
        if(trace_data_name == "general")
            printf("Memory usage analysis Success!\n");
        else
            printf("Memory usage analysis for Acceleration Domain Success!\n");
        printf("========================================================\n\n\n");
    }
    catch (const std::exception& e){
        printf("\n========================================================\n");
        if(trace_data_name == "general")
            printf("Memory usage analysis Fail!\n");
        else
            printf("Memory usage analysis for Acceleration Domain Fail!\n");
        printf("========================================================\n\n\n");
    }
}

void MemoryAnalyzer::analysis_usage_trace_print(MemoryAnalyzerTraceData* trace_data,std::string trace_data_name)
// void MemoryAnalyzer::analysis_usage_trace_print(std::unique_ptr<MemoryAnalyzerTraceData> trace_data,std::string trace_data_name)
{
    int rank = trace_data->rank;
    std::string id = trace_data->id;

    const auto& ptr2size = trace_data->ptr2size;
    const auto& memory_usage_list = trace_data->memory_usage_list;
    const auto& alloc_times = trace_data->alloc_times;
    const auto& free_times = trace_data->alloc_times;
    const auto& count = trace_data->event_num;
    const auto& all_count = trace_data->all_event_num;

    printf("\nMemory usage analysis for Rank%d:\n\n",rank + rank_offset);
    if(count == 0)
    {
        printf("    no memory usage info\n\n");
        return;
    }
    std::string filename;
    if(trace_data_name == "general")
        filename = output_dir + "/memory_usage_rank" + std::to_string(rank + rank_offset) + ".csv";
    else
        filename = output_dir + "/accl_memory_usage_rank" + std::to_string(rank + rank_offset) + ".csv";
    printf("    output_dir: %s\n", output_dir.c_str());
    printf("    memory_usage file: %s\n\n", filename.c_str());

    int i = 0;
    int64_t memory_total;
    if(trace_data_name == "general")
        memory_total =  get_meta_value("MEMORY_INFO","memory_total",rank)->i64;
    else
        memory_total = 32*1024*1024;

    printf("    memory_total: %ld Bytes\n", memory_total*1000);
    auto mu = *(memory_usage_list.rbegin());
    printf("    last alloc -- timestamps: %lu, alloc_size: %lu Bytes, free_size: %lu Bytes, memory_usage: %lu Bytes, usage_ratio: %lf\n\n", (mu.timestamps.enter+mu.timestamps.exit)/2, mu.alloc_size, mu.free_size, mu.alloc_size-mu.free_size, (mu.alloc_size-mu.free_size)*1.0/(memory_total*1000));
}

void MemoryAnalyzer::analysis_usage_trace(MemoryAnalyzerTraceData* trace_data,std::mutex &print_mutex,std::string trace_data_name)
// void MemoryAnalyzer::analysis_usage_trace(std::unique_ptr<MemoryAnalyzerTraceData> trace_data,std::mutex &print_mutex,std::string trace_data_name)
{
    int rank = trace_data->rank;
    std::string id = trace_data->id;

    const auto& ptr2size = trace_data->ptr2size;
    const auto& memory_usage_list = trace_data->memory_usage_list;
    const auto& alloc_times = trace_data->alloc_times;
    const auto& free_times = trace_data->alloc_times;
    const auto& count = trace_data->event_num;
    const auto& all_count = trace_data->all_event_num;

    if(count == 0)
    {
        printf("No usage count");
        return;
    }
    std::string filename;
    if(trace_data_name == "general")
        filename = output_dir + "/memory_usage_rank" + std::to_string(rank + rank_offset) + ".csv";
    else
        filename = output_dir + "/accl_memory_usage_rank" + std::to_string(rank + rank_offset) + ".csv";
    int i = 0;

    int64_t memory_total;
    if(trace_data_name == "general")
        memory_total =  get_meta_value("MEMORY_INFO","memory_total",rank)->i64;
    else
        memory_total = 32*1024*1024;

    auto mu = *(memory_usage_list.rbegin());

    //write to file
    std::ofstream ofs(filename, std::ios::trunc);
    if (!ofs.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    ofs << "timestamp,alloc_size (Bytes),free_size (Bytes),memory_usage (Bytes),usage_ratio\n";

    uint64_t base_ts = 0;
    for (auto &mu : memory_usage_list)
    {
        if(i == 0)
            base_ts = (mu.timestamps.enter + mu.timestamps.exit) / 2;

        i += 1;

        uint64_t ts = (mu.timestamps.enter + mu.timestamps.exit) / 2 - base_ts;
        uint64_t usage = mu.alloc_size - mu.free_size;
        double ratio = (usage * 1.0) / (memory_total * 1000);

        // printf("    timestamps: %lu, alloc_size: %lu, free_size: %lu, memory_usage: %lu, usage_ratio: %lf\n",
        //        ts, mu.alloc_size, mu.free_size, usage, ratio);

        ofs << ts << ","
            << mu.alloc_size << ","
            << mu.free_size << ","
            << usage << ","
            << ratio << "\n";
    }

    ofs.close();  // 关闭文件

    //print std
    if(enable_mt)
    {
        std::lock_guard<std::mutex> lock(print_mutex);
        analysis_usage_trace_print(trace_data,trace_data_name);
    }
    else
    {
        analysis_usage_trace_print(trace_data,trace_data_name);
    }
}

void MemoryAnalyzer::analysis_memory_leak(std::string trace_data_name)
{
    // std::vector<MemoryAnalyzerTraceData*> &local_trace_data_list = (trace_data_name == "general") ? this->trace_data_list : this->trace_data_list_accl;
    std::vector<std::unique_ptr<MemoryAnalyzerTraceData>> &local_trace_data_list = (trace_data_name == "general") ? this->trace_data_list : this->trace_data_list_accl;

    printf("\n========================================================\n");
    if(trace_data_name == "general")
        printf("Memory leak analysis Start:\n");
    else
        printf("Memory leak analysis for Acceleration Domain Start:\n");
    printf("========================================================\n\n");
    try{

        std::mutex print_mutex;

        if(!enable_mt)
        {
            for(auto &it : local_trace_data_list)
            {
                analysis_memory_leak_trace(it.get(),print_mutex,trace_data_name);
            }
        }
        else
        {
            // 多线程处理部分
            // int total = record_trace_num;
            int total = current_round_size;
            int num_threads = std::max(1, (int)(static_cast<double>(std::thread::hardware_concurrency()) * mt_percent));
            int chunk_size = (total + num_threads - 1) / num_threads; // 向上取整
            std::vector<std::thread> threads;
            std::atomic<int> exit_index(1);
    
            for (int t = 0; t < num_threads; ++t) {
                int start = t * chunk_size;
                int end = std::min(start + chunk_size, total);
                threads.emplace_back([this, &local_trace_data_list,start, end ,&print_mutex,&exit_index,trace_data_name]() {
                    for (int i = start; i < end; ++i) {
                        analysis_memory_leak_trace(local_trace_data_list[i].get(),print_mutex,trace_data_name);
                    }
                });
            }
            // 等待所有线程完成
            for (auto& thread : threads) {
                thread.join();
            }
        }

        printf("\n========================================================\n");
        if(trace_data_name == "general")
            printf("Memory leak analysis Success!\n");
        else
            printf("Memory leak analysis for Acceleration Domain Success!\n");
        printf("========================================================\n\n\n");
    }
    catch (const std::exception& e){
        printf("\n========================================================\n");
        if(trace_data_name == "general")
            printf("Memory leak analysis Fail!\n");
        else
            printf("Memory leak analysis for Acceleration Domain Fail!\n");
        printf("========================================================\n\n\n");
    }

}

void MemoryAnalyzer::analysis_memory_leak_trace_print(MemoryAnalyzerTraceData* trace_data,int leak_size,int n,std::string trace_data_name)
// void MemoryAnalyzer::analysis_memory_leak_trace_print(std::unique_ptr<MemoryAnalyzerTraceData> trace_data,int leak_size,int n,std::string trace_data_name)
{
    int rank = trace_data->rank;
    std::string id = trace_data->id;

    const auto& ptr2size = trace_data->ptr2size;
    const auto& memory_usage_list = trace_data->memory_usage_list;
    const auto& alloc_times = trace_data->alloc_times;
    const auto& free_times = trace_data->alloc_times;
    const auto& count = trace_data->event_num;
    const auto& all_count = trace_data->all_event_num;

    printf("Memory leak analysis for Rank%d:\n\n",rank + rank_offset);
    if(count == 0)
    {
        printf("    no memory leak info\n\n");
    }
    std::string filename;
    if(trace_data_name == "general")
        filename = output_dir + "/memory_leak_rank" + std::to_string(rank + rank_offset) + ".txt";
    else
        filename = output_dir + "/accl_memory_leak_rank" + std::to_string(rank + rank_offset) + ".txt";
    printf("    output_dir: %s\n", output_dir.c_str());
    printf("    memory_usage file: %s\n\n", filename.c_str());



    printf("    LEAK SUMMARY:");
    printf("    in use at exit: %lu bytes in %d blocks\n", leak_size, n);
    if(memory_usage_list.size() == 0)
        printf("        no heap usage info\n");
    else
        printf("        total heap usage: %lu allocs, %lu frees, %lu bytes allocated\n\n", alloc_times, free_times, memory_usage_list.rbegin()->alloc_size);
}

void MemoryAnalyzer::analysis_memory_leak_trace(MemoryAnalyzerTraceData* trace_data,std::mutex &print_mutex,std::string trace_data_name)
// void MemoryAnalyzer::analysis_memory_leak_trace(std::unique_ptr<MemoryAnalyzerTraceData> trace_data,std::mutex &print_mutex,std::string trace_data_name)
{
    int rank = trace_data->rank;
    std::string id = trace_data->id;

    const auto& ptr2size = trace_data->ptr2size;
    const auto& memory_usage_list = trace_data->memory_usage_list;
    const auto& alloc_times = trace_data->alloc_times;
    const auto& free_times = trace_data->alloc_times;
    const auto& count = trace_data->event_num;
    const auto& all_count = trace_data->all_event_num;

    if(count == 0)
    {
        printf("No leak count");
        return ;
    }

    std::string filename;
    if(trace_data_name == "general")
        filename = output_dir + "/memory_leak_rank" + std::to_string(rank + rank_offset) + ".txt";
    else
        filename = output_dir + "/accl_memory_leak_rank" + std::to_string(rank + rank_offset) + ".txt";

    std::ofstream ofs(filename, std::ios::trunc);
    if(!ofs.is_open())
    {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    ofs << "Memory leak analysis:\n\n";
    if(count == 0)
    {
        ofs << "    no memory leak info\n\n";
        ofs.close();
        return;
    }

    uint64_t leak_size = 0;
    int i = 0;
    int n = ptr2size.size();
    auto &bt_tree = *backtraces[id];
    std::vector<const char *> vec;

    for (auto it : ptr2size)
    {
        i += 1;
        leak_size += it.second.size;

        // if(pretty_print)
        // {
        //     printf("leak %lu bytes in 1 block  %d of %d\n", it.second.size, i, n);
        //     printf("    at context:\n");
        // }

        ofs << "leak " << it.second.size 
            << " bytes in 1 block  " << i << " of " << n << "\n"
            << "    at context:\n";

        vec.clear();
        bt_tree.backtrace_get_context_string_vec(it.second.ctxt, -1, vec);
        for(auto it=vec.begin(); it!=vec.end(); it++)
        {
            // if(pretty_print)
            //     printf("        %s\n", *it);
            ofs << "        " << *it << "\n";
        }
        // if(pretty_print)
        //     printf("\n");
        ofs << "\n";
    }

    ofs << "LEAK SUMMARY:\n";
    ofs << "    in use at exit: " << leak_size << " bytes in " << n << " blocks\n";
    if(memory_usage_list.size() == 0)
        ofs << "    no heap usage info\n";
    else
        ofs << "    total heap usage: " << alloc_times << " allocs, "
            << free_times << " frees, "
            << memory_usage_list.rbegin()->alloc_size << " bytes allocated\n";
    ofs.close();  // 关闭文件

    if(enable_mt)
    {
        std::lock_guard<std::mutex> lock(print_mutex);
        analysis_memory_leak_trace_print(trace_data, leak_size, n,trace_data_name);
    }
    else
    {
        analysis_memory_leak_trace_print(trace_data, leak_size, n,trace_data_name);
    }
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
    // printf("allocate_time: %d\n", get_meta_value("MEMORY_INFO_END","allocate_time",rank)->i32);
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
