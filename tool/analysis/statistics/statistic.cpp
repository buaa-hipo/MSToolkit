#include "statistic.h"
#include "topdown_tree.h"
#include "bottomup_tree.h"

#include <cmath>
#include <iostream>
#include <fstream>

#define MAX_THREAD 128

Statistics::Statistics(RecordTraceCollection &record_collection,
                       RankMetaCollection &meta_collection,
                       BacktraceCollection &backtrace_collection) {
    _record_collection = record_collection;
    _meta_collection = meta_collection;
    _backtrace_collection = backtrace_collection;
    _topdown_overview = new TopDownTree(-1);
}

Statistics::~Statistics() {
    _record_collection.clear();
    _meta_collection.clear();
    _backtrace_collection.clear();
}

void Statistics::dump_top_down_trees(std::string file_dir) {
    std::string dump_path;
    std::ofstream dump_file;
    
    for (TopDownTree tree: _top_down_tree_list) {
        int rank = tree.get_rank();
        dump_path = file_dir + "/top-down-" + std::to_string(rank);
        dump_file.open(dump_path);
        dump_file << tree.dump();
        dump_file.close();
    }
    dump_path = file_dir + "/top-down-overview";
    dump_file.open(dump_path);
    std::string overview = _topdown_overview->dump();
    if (overview.max_size() < overview.size()) {
        std::cout << "overview too long! " << std::endl;
    }
    dump_file << overview;
    dump_file.close();
}

void Statistics::dump_bottom_up_trees(std::string file_dir) {
    std::string dump_path;
    std::ofstream dump_file;
    for (BottomUpForest forest: _bottom_up_forest_list) {
        int rank = forest.forest_get_rank();
        dump_path = file_dir + "/bottom-up-" + std::to_string(rank);
        dump_file.open(dump_path);
        dump_file << forest.forest_dump();
        dump_file.close();
    }
}

void update_node_list_thread(std::unordered_map<void*, TopDownNode>& map, void *ip, uint64_t ctxt, std::string name, uint64_t cycle, bool is_child) {
    if (map.find(ip) == map.end()) {
        TopDownNode node;
        node.name = name;
        node.ip = ip;
        node.ctxt = ctxt;
        node.stat.tot_time.cycle = cycle;
        node.stat.avg_time.cycle = cycle;
        node.stat.square_tot.cycle = cycle * cycle;
        node.stat.square_avg.cycle = cycle * cycle;
        node.stat.std_time.cycle = 0;
        node.stat.n_call = 1;
        node.is_leaf = true;
        map.insert(std::pair<void*, TopDownNode>(ip, node));
    }
    else {
        TopDownNode& node = map[ip];
        if (is_child) {
            node.stat.n_call += 1;
            node.is_leaf = true;
            node.stat.square_tot.cycle += cycle * cycle;
            node.stat.square_avg.cycle = node.stat.square_tot.cycle / (double)node.stat.n_call;
            node.stat.std_time.cycle = sqrt(node.stat.square_avg.cycle - node.stat.avg_time.cycle * node.stat.avg_time.cycle);
        }
        node.stat.tot_time.cycle += cycle;
        node.stat.avg_time.cycle = node.stat.tot_time.cycle / (double)node.stat.n_call;
    }
}

void Statistics::iter_record(Statistics::time_t& total_time, Statistics::FuncStatMap& funcStatMap, std::vector<double>& comm_brkdwn_dist, Statistics::DeriveMode mode) {
    if (mode == Statistics::FLAT) {
        std::vector<Statistics::FuncStatMap> funcMap_thread(omp_get_max_threads());
        std::vector<RecordTrace *> rt_list;
        std::vector<int> rank_list;
        int thread_count = omp_get_max_threads();
        // std::cout << "Thread max: " << omp_get_max_threads() << std::endl;
        for (auto it = _record_collection.begin(); it != _record_collection.end(); ++it) {
            rt_list.push_back(it->second);
            rank_list.push_back(it->first);
        }

#pragma omp parallel for 
        for (int i = 0; i < rt_list.size(); i++) {
            // std::cout << "Thread count: " << omp_get_thread_num() << std::endl;
            int rank = rank_list[i];
            // std::cout << "Rank " << rank << std::endl;
            RecordTrace &rt = *(rt_list[i]);
            comm_brkdwn_t brkwnt_t;
            uint64_t sync_cycle = 0;
            uint64_t comm_cycle = 0;
            RecordHelper helper;

            if (_record_collection.find(rank) != _record_collection.end()) {
                /*Get total time of each rank*/
                time_t rank_total_time;
                uint64_t start_time;
                uint64_t exit_time;

                for (RecordTraceIterator rti = rt.begin(), rte = rt.end(); rti != rte; rti = rti.next()) {
                    record_t &record = *(rti.val());

                    if (helper.is_process_start(&record)) {
                        start_time = rti.val()->timestamps.enter;
                    }
                    if (helper.is_process_exit(&record)) {
                        exit_time = rti.val()->timestamps.exit;
                    }
                    
                    
                    std::string name = helper.get_record_name(&record);
                    if (helper.is_function(&record)) {
                        name = "Function";
                    }
                    uint64_t curCycle = record.timestamps.exit - record.timestamps.enter;

                    Statistics::FuncStatMap& func_this_thread = funcMap_thread[omp_get_thread_num()];
                    if (func_this_thread.find(name) == func_this_thread.end()) {
                            func_this_thread[name].tot_time.cycle = 0;
                            func_this_thread[name].n_call = 0;
                            func_this_thread[name].avg_time.cycle = 0;
                            func_this_thread[name].square_tot.cycle = 0;
                            func_this_thread[name].square_avg.cycle = 0;
                            func_this_thread[name].std_time.cycle = 0;
                    }
                    else {
                        func_this_thread[name].n_call += 1;
                        func_this_thread[name].tot_time.cycle += curCycle;
                        func_this_thread[name].square_tot.cycle += curCycle * curCycle;
                        func_this_thread[name].avg_time.cycle = func_this_thread[name].tot_time.cycle / (double)func_this_thread[name].n_call;
                        func_this_thread[name].square_avg.cycle = func_this_thread[name].square_tot.cycle / (double)func_this_thread[name].n_call;
                        func_this_thread[name].std_time.cycle = sqrt(func_this_thread[name].square_avg.cycle - func_this_thread[name].avg_time.cycle * func_this_thread[name].avg_time.cycle);
                    }

                    if (helper.is_event(&record, event_MPI_Barrier)) {
                        sync_cycle += record.timestamps.exit - record.timestamps.enter;
                    }
                    if (helper.is_mpi(&record) && !(helper.is_event(&record, event_MPI_Barrier))) {
                        comm_cycle += record.timestamps.exit - record.timestamps.enter;
                    } 
                }

                rank_total_time.cycle = exit_time - start_time;
                #pragma omp critical 
                {
                    _rank_total_time_map[rank] = rank_total_time;
                    _rank_total_time_list.push_back(rank_total_time.cycle);
                    brkwnt_t.sync = (double) sync_cycle / (double) rank_total_time.cycle;
                    brkwnt_t.comm = (double) comm_cycle / (double) rank_total_time.cycle;
                    comm_brkdwn_dist.push_back(brkwnt_t.comm + brkwnt_t.sync); 
                }             
            }
        }

        for (int i = 0; i < thread_count; i++) {
            // std::cout << "i: " << i << std::endl;
            for (auto & it : funcMap_thread[i]) {
                std::string name = it.first;
                func_stat_t stat = it.second;
                if (funcStatMap.find(name) == funcStatMap.end()) {
                    funcStatMap[name] = stat;
                }
                else {
                    funcStatMap[name].n_call += stat.n_call;
                    funcStatMap[name].tot_time.cycle += stat.tot_time.cycle;
                    funcStatMap[name].square_tot.cycle += stat.square_tot.cycle;
                    funcStatMap[name].avg_time.cycle = funcStatMap[name].tot_time.cycle / (double)funcStatMap[name].n_call;
                    funcStatMap[name].square_avg.cycle = funcStatMap[name].square_tot.cycle / (double)funcStatMap[name].n_call;
                    funcStatMap[name].std_time.cycle = sqrt(funcStatMap[name].square_avg.cycle - funcStatMap[name].avg_time.cycle * funcStatMap[name].avg_time.cycle);
                }
            }
        }
        
        
        for (int i = 0; i < rt_list.size(); i++) {
            if (_rank_total_time_list[i] > total_time.cycle) {
                total_time.cycle = _rank_total_time_list[i];
            }
        }

    }
    else if (mode == Statistics::TOP_DOWN) {
        std::vector<RecordTrace *> rt_list;
        std::vector<int> rank_list;
        std::vector<std::unordered_map<void*, TopDownNode> > nodes_list_thread(omp_get_max_threads());
        int thread_count = omp_get_num_procs();

        for (auto it = _record_collection.begin(); it != _record_collection.end(); ++it) {
            rt_list.push_back(it->second);
            rank_list.push_back(it->first);
        }

        #pragma omp parallel for 
        for (int i = 0; i < rt_list.size(); i++) {
            int rank = rank_list[i];
            RecordHelper helper;
            time_t rank_total_time;

            if (_record_collection.find(rank) != _record_collection.end()) {
                TopDownTree tree(rank);
                // std::cout << "## Rank " << rank << std::endl;
                RecordTrace &rt = *(_record_collection[rank]);
                uint64_t start_time;
                uint64_t exit_time;

                for (RecordTraceIterator rti = rt.begin(), rte = rt.end(); rti != rte; rti = rti.next()) {
                    record_t &record = *(rti.val());
                    if (helper.is_process_start(&record)) {
                        start_time = rti.val()->timestamps.enter;
                        continue;
                    }
                    if (helper.is_process_exit(&record)) {
                        exit_time = rti.val()->timestamps.exit;
                        continue;
                    }
                    uint64_t ctxt = record.ctxt;
                    std::string name = helper.get_record_name(&record);
                    uint64_t curCycle = record.timestamps.exit - record.timestamps.enter;
                    
                    if (_backtrace_collection.find(rank) != _backtrace_collection.end()) {
                        jsi::toolkit::BacktraceTree* backtrace_tree = _backtrace_collection[rank];
                        void *ip = backtrace_tree->backtrace_context_ip(ctxt);
                        tree.updateTree(ip, ctxt, name, curCycle, true);
                        update_node_list_thread(nodes_list_thread[omp_get_thread_num()], ip, ctxt, name, curCycle, true);

                        while (ctxt != BACKTRACE_ROOT_NODE) {
                            backtrace_context_t parent_ctxt = backtrace_tree->get_parent(ctxt);
                            void *parent_ip = backtrace_tree->backtrace_context_ip(parent_ctxt);
                            std::string parent_name = backtrace_tree->backtrace_get_node_string(parent_ctxt);
                            tree.updateTree(parent_ip, parent_ctxt, parent_name, curCycle, false);
                            tree.addChild(parent_ip, ip);
                            update_node_list_thread(nodes_list_thread[omp_get_thread_num()], parent_ip, parent_ctxt, parent_name, curCycle, false);
                            TopDownNode& node = nodes_list_thread[omp_get_thread_num()][parent_ip];
                            node.children.emplace(ip);
                            ctxt = parent_ctxt;
                            ip = parent_ip;
                        }
                    }
                }
                rank_total_time.cycle = exit_time - start_time;
                tree.set_total_time(rank_total_time);
                #pragma omp critical
                {
                    _top_down_tree_list.push_back(tree);
                    _rank_total_time_list.push_back(rank_total_time.cycle);
                }   
            }
        }

        for (int i = 0; i < thread_count; i++) {
            for (auto it : nodes_list_thread[i]) {
                auto ip = it.first;
                auto& node = it.second;
                if (_topdown_overview->get_nodes_list().find(ip) == _topdown_overview->get_nodes_list().end()) {
                    _topdown_overview->get_nodes_list().insert(it);
                    if (node.ctxt == BACKTRACE_ROOT_NODE) {
                        _topdown_overview->set_head(node);
                    }
                    
                }
                else {
                    auto& cur_node = _topdown_overview->get_nodes_list()[ip];
                    if (node.is_leaf) {
                        cur_node.stat.n_call += node.stat.n_call;
                        cur_node.stat.square_tot.cycle += node.stat.square_tot.cycle;
                        cur_node.stat.square_avg.cycle = cur_node.stat.square_tot.cycle / (double)cur_node.stat.n_call;
                        cur_node.stat.std_time.cycle = sqrt(cur_node.stat.square_avg.cycle - cur_node.stat.avg_time.cycle * cur_node.stat.avg_time.cycle);
                    }
                    cur_node.stat.tot_time.cycle += node.stat.tot_time.cycle;
                    cur_node.stat.avg_time.cycle = cur_node.stat.tot_time.cycle / (double)cur_node.stat.n_call;
                    if (node.ctxt == BACKTRACE_ROOT_NODE) {
                        _topdown_overview->set_head(cur_node);
                    }
                }
            }
        }
        
        // std::cout << "size of _nodes_list: " << _topdown_overview->get_nodes_list().size() << std::endl;
        // for (auto pair : _topdown_overview->get_nodes_list()) {
        //     auto ip = pair.first;
        //     auto node = pair.second;
        //     std::cout << "========" << node.name << "=======" << std::endl;
        //     std::cout << "ip: " << node.ip << std::endl;
        //     std::cout << "ctxt: " << node.ctxt << std::endl;
        //     std::cout << "children size: " << node.children.size() << std::endl;
        //     for (auto child_ip : node.children) {
        //         std::cout << "children is: " << child_ip << std::endl;
        //         std::cout << "\t child: " << _topdown_overview->get_nodes_list()[child_ip].name << std::endl;
        //     }
        //     std::cout << std::endl;
        // }
        time_t max_time;
        max_time.cycle = 0;
        
        for (int i = 0; i < rt_list.size(); i++) {
            if (_rank_total_time_list[i] > max_time.cycle) {
                max_time.cycle = _rank_total_time_list[i];
            }
        }
        _topdown_overview->set_total_time(max_time);
    }
    else if (mode == Statistics::BOTTOM_UP) {
        time_t max_time;
        max_time.cycle = 0;

        std::vector<RecordTrace *> rt_list;
        std::vector<int> rank_list;
        for (auto it = _record_collection.begin(); it != _record_collection.end(); ++it) {
            rt_list.push_back(it->second);
            rank_list.push_back(it->first);
        }

#pragma omp parallel for 
        for (int i = 0; i < rt_list.size(); i++) {
            int rank = rank_list[i];
            // std::cout << "## Rank " << rank << std::endl;
            RecordHelper helper;
            time_t rank_total_time;

            if (_record_collection.find(rank) != _record_collection.end()) {
                BottomUpForest forest(rank);
                // std::cout << "rank" << rank << std::endl;
                RecordTrace &rt = *(rt_list[i]);
                uint64_t start_time;
                uint64_t exit_time;

                for (RecordTraceIterator rti = rt.begin(), rte = rt.end(); rti != rte; rti = rti.next()) {
                    record_t &record = *(rti.val());
                    if (helper.is_process_start(&record)) {
                        start_time = rti.val()->timestamps.enter;
                        continue;
                    }
                    if (helper.is_process_exit(&record)) {
                        exit_time = rti.val()->timestamps.exit;
                        continue;
                    }
                    uint64_t ctxt = record.ctxt;
                    std::string tree_name = helper.get_record_name(&record);
                    uint64_t curCycle = record.timestamps.exit - record.timestamps.enter;
                    // std::cout << "tree_name: "  << tree_name << std::endl;
                    if (_backtrace_collection.find(rank) != _backtrace_collection.end()) {
                        jsi::toolkit::BacktraceTree* backtrace_tree = _backtrace_collection[rank];
                        void *src_ip = backtrace_tree->backtrace_context_ip(ctxt);
                        void *ip = src_ip;
                        std::string name = tree_name;
                        forest.updateForest(rank, tree_name, src_ip, ip, ctxt, tree_name, curCycle, true);
                        
                        while (ctxt != BACKTRACE_ROOT_NODE) {
                            backtrace_context_t parent_ctxt = backtrace_tree->get_parent(ctxt);
                            // std::cout << "ctxt(" << ctxt << ")'s parent is " << parent_ctxt << std::endl;
                            void *parent_ip = backtrace_tree->backtrace_context_ip(parent_ctxt);
                            std::string parent_name = backtrace_tree->backtrace_get_node_string(parent_ctxt);
                            // std::cout << "parent_name" << parent_name << std::endl;
                            forest.updateForest(rank, tree_name, src_ip, parent_ip, parent_ctxt, parent_name, curCycle, false);
                            forest.forestAddChild(tree_name, parent_name, name, src_ip);
                            name = parent_name;
                            ctxt = parent_ctxt;
                            ip = parent_ip;
                        }
                    }
                }

                #pragma omp critical 
                {
                    _bottom_up_forest_list.push_back(forest);
                }
               
            }
        }
    }

}

Statistics::time_t Statistics::get_total_time() {
    time_t totalTime;
    uint64_t max_cycle = 0;

    for (auto it = _record_collection.begin(), ie = _record_collection.end(); it != ie; ++it) {
        int rank = it->first;
        time_t curTime = get_total_time_with_rank(rank);

        if (curTime.cycle >= max_cycle) {
            max_cycle = curTime.cycle;
        }
    }
    totalTime.cycle = max_cycle;
    return totalTime;
}

Statistics::time_t Statistics::get_total_time_with_rank(int rank) {
    if (_rank_total_time_map.find(rank) != _rank_total_time_map.end()) {
        return _rank_total_time_map[rank];
    }
    if (_record_collection.find(rank) != _record_collection.end()) {
        time_t totalTime;
        RecordTrace &rt = *(_record_collection[rank]);
        uint64_t start_time;
        uint64_t exit_time;

        
        RecordTraceIterator rti = rt.find(JSI_PROCESS_START, true /*ignore zoom*/);
        if (RecordTraceIterator::is_invalid(rti)) {
            JSI_ERROR("Error: Could not find PROCESS_START event in Rank %d\n", rank);
        }
        start_time = rti.val()->timestamps.enter;

        rti = rt.find(JSI_PROCESS_EXIT, true);
        if (RecordTraceIterator::is_invalid(rti)) {
            JSI_ERROR("Error: Could not find PROCESS_EXIT event in Rank %d\n", rank);
        }
        exit_time = rti.val()->timestamps.exit;
        totalTime.cycle = exit_time - start_time;
        _rank_total_time_map[rank] = totalTime;
        return totalTime;
    } else {
        JSI_ERROR("Error: Could find record trace in Rank %d\n", rank);
    }
}

Statistics::FuncStatMap *Statistics::get_all_stat(DeriveMode mode) {
    FuncStatMap *mpiStatMap = get_MPI_stat(mode);
    FuncStatMap *funcStatMap = get_Func_stat(mode);
    mpiStatMap->insert(funcStatMap->begin(), funcStatMap->end());
    return mpiStatMap;
}

Statistics::FuncStatMap *Statistics::get_MPI_stat(DeriveMode mode) {
    FuncStatMap *funcStatMap = new FuncStatMap();
    std::unordered_map <std::string, std::vector<uint64_t>> everyFuncTime;

    for (auto it = _record_collection.begin(), ie = _record_collection.end(); it != ie; ++it) {
        int rank = it->first;
        RecordTrace &rt = *(it->second);
        RecordHelper helper;

        for (RecordTraceIterator rti = rt.begin(), rte = rt.end(); rti != rte; rti = rti.next()) {
            record_t &record = *(rti.val());

            if (helper.is_mpi(&record)) {
                std::string name = helper.get_record_name(&record);
                uint64_t curCycle = record.timestamps.exit - record.timestamps.enter;

                everyFuncTime[name].push_back(curCycle);
                if ((*funcStatMap).find(name) == funcStatMap->end()) {//  initialize the funcStatMap
                    (*funcStatMap)[name].tot_time.cycle = 0;
                    (*funcStatMap)[name].n_call = 0;
                }
                (*funcStatMap)[name].tot_time.cycle += curCycle;
                (*funcStatMap)[name].n_call += 1;
            }
        }
    }

    for (auto it = (*funcStatMap).begin(); it != (*funcStatMap).end(); ++it) {
        std::string name = it->first;
        func_stat_t *stat = &(it->second);
        // calculate avg
        stat->avg_time.cycle = stat->tot_time.cycle / stat->n_call;
        // calculate std
        uint64_t tmp = 0;
        for (int i = 0; i < everyFuncTime[name].size(); i++) {
            tmp += (everyFuncTime[name][i] - stat->avg_time.cycle) *
                   (everyFuncTime[name][i] - stat->avg_time.cycle);
        }
        stat->std_time.cycle = sqrt(tmp / stat->n_call);
    }

    return funcStatMap;
}

Statistics::FuncStatMap *Statistics::get_Func_stat(DeriveMode mode) {
    FuncStatMap *funcStatMap = new FuncStatMap();
    std::unordered_map <std::string, std::vector<uint64_t>> everyFuncTime;

    for (auto it = _record_collection.begin(), ie = _record_collection.end(); it != ie; ++it) {
        int rank = it->first;
        RecordTrace &rt = *(it->second);
        RecordHelper helper;

        for (RecordTraceIterator rti = rt.begin(), rte = rt.end(); rti != rte; rti = rti.next()) {
            record_t &record = *(rti.val());

            if (!helper.is_mpi(&record)) {
                std::string name = helper.get_record_name(&record);
                uint64_t curCycle = record.timestamps.exit - record.timestamps.enter;

                everyFuncTime[name].push_back(curCycle);
                if ((*funcStatMap).find(name) == funcStatMap->end()) {//  initialize the funcStatMap
                    (*funcStatMap)[name].tot_time.cycle = 0;
                    (*funcStatMap)[name].n_call = 0;
                }
                (*funcStatMap)[name].tot_time.cycle += curCycle;
                (*funcStatMap)[name].n_call += 1;
            }
        }
    }

    for (auto it = (*funcStatMap).begin(); it != (*funcStatMap).end(); ++it) {
        std::string name = it->first;
        func_stat_t *stat = &(it->second);
        // calculate avg
        stat->avg_time.cycle = stat->tot_time.cycle / stat->n_call;
        // calculate std
        uint64_t tmp = 0;
        for (int i = 0; i < everyFuncTime[name].size(); i++) {
            tmp += (everyFuncTime[name][i] - stat->avg_time.cycle) *
                   (everyFuncTime[name][i] - stat->avg_time.cycle);
        }
        stat->std_time.cycle = sqrt(tmp / stat->n_call);
    }

    return funcStatMap;
}

Statistics::func_stat_t Statistics::get_stat_with_name(std::string name, DeriveMode mode) {
    FuncStatMap &funcStatMap = *(get_Func_stat(mode));
    return funcStatMap[name];
}

Statistics::comm_brkdwn_t Statistics::get_Comm_Breakdown_with_rank(int rank) {
    if (_record_collection.find(rank) != _record_collection.end()) {
        RecordTrace &rt = *(_record_collection[rank]);
        comm_brkdwn_t brkwnt_t;
        uint64_t sync_cycle = 0;
        uint64_t comm_cycle = 0;
        RecordHelper helper;
        time_t totalTime = get_total_time_with_rank(rank);

        for (RecordTraceIterator rti = rt.begin(); rti != rt.end(); rti = rti.next()) {
            record_t &record = *(rti.val());
            if (helper.is_event(&record, event_MPI_Barrier)) {
                sync_cycle += record.timestamps.exit - record.timestamps.enter;
            }
            if (helper.is_mpi(&record) && !(helper.is_event(&record, event_MPI_Barrier))) {
                comm_cycle += record.timestamps.exit - record.timestamps.enter;
            }
        }

        brkwnt_t.sync = (double) sync_cycle / (double) totalTime.cycle;
        brkwnt_t.comm = (double) comm_cycle / (double) totalTime.cycle;

        return brkwnt_t;
    } else {
        std::cout << "Can't find record trace of rank " << rank << ". " << std::endl;
        exit(1);
    }
}

double Statistics::get_Comm_Ratio_with_rank(int rank) {
    comm_brkdwn_t brkdwn_t = get_Comm_Breakdown_with_rank(rank);
//    time_t total_time = get_total_time_with_rank(rank);

    return brkdwn_t.comm + brkdwn_t.sync;
}

void Statistics::get_Comm_Ratio_dist(std::vector<double> &dist, double *avg, double *std) {
    int n_rank = _record_collection.size();
    double avg_temp = 0.0;
    double std_temp = 0.0;

    // for (int i = 0; i < n_rank; i++) {
    //     dist.push_back(get_Comm_Ratio_with_rank(i));
    // }

    if (avg != NULL) {
        for (int i = 0; i < n_rank; i++) {
            avg_temp += dist[i];
        }
        avg_temp /= (double) n_rank;
        *(avg) = avg_temp;
    }

    if (std != NULL) {
        if (avg == NULL) {
            for (int i = 0; i < n_rank; i++) {
                avg_temp += dist[i];
            }
            avg_temp /= (double) n_rank;
            for (int i = 0; i < n_rank; i++) {
                std_temp += (dist[i] - avg_temp) * (dist[i] - avg_temp);
            }
            std_temp = std_temp / (double) n_rank;
            std_temp = sqrt(std_temp);
        } else {
            for (int i = 0; i < n_rank; i++) {
                std_temp += (dist[i] - avg_temp) * (dist[i] - avg_temp);
            }
            std_temp = std_temp / (double) n_rank;
            std_temp = sqrt(std_temp);
        }
        *(std) = std_temp;
    }
}

void Statistics::RegisterMetricFunc(std::string name, MetricFunc_t func) {
    _metricFuncMap.insert(std::make_pair(name, func));
}

void Statistics::get_cal_map_with_rank(int rank, PMUStatMap &pmuStatMap, CalMap &calMap) {
    if (_record_collection.find(rank) == _record_collection.end()) {
        JSI_ERROR("Error: Could not find record in Rank %d\n", rank);
    }

    RecordTrace &rt = *(_record_collection[rank]);

    // Fetch event names and num events
    auto &event_list = rt.pmu_event_list();
    auto num_events = rt.num_pmu_events();

    for (RecordTraceIterator rti = rt.begin(), rte = rt.end(); rti != rte; rti = rti.next()) {
        record_t &record = *(rti.val());
        std::string func_name = RecordHelper::get_record_name(&record);

        uint64_t cycle = record.timestamps.exit - record.timestamps.enter;
        // Init map with tsc cycle data
        std::unordered_map<std::string, double> map = {{"CYCLE", cycle}};
        for (int i = 0; i < num_events; i++) {
            auto &pmu_event_name = event_list[i];
            auto metric = (double) RecordHelper::counter_diff(&record, i, num_events);

            // Add pmu event metrics into map
            map[pmu_event_name] = metric;

            if (pmuStatMap.find(func_name) == pmuStatMap.end() ||
                pmuStatMap[func_name].find(pmu_event_name) == pmuStatMap[func_name].end()) {
                pmuStatMap[func_name][pmu_event_name].tot = 0.0;
                pmuStatMap[func_name][pmu_event_name].n_call = 0;
            }
            pmuStatMap[func_name][pmu_event_name].tot += metric;
            pmuStatMap[func_name][pmu_event_name].n_call += 1;
            calMap[func_name][pmu_event_name].push_back(metric);
        }

        for (std::pair <std::string, MetricFunc_t> kv: _metricFuncMap) {
            std::string new_metric_name = kv.first;
            MetricFunc_t func = kv.second;
            (*func)(map);
            if (pmuStatMap.find(func_name) == pmuStatMap.end() ||
                pmuStatMap[func_name].find(new_metric_name) == pmuStatMap[func_name].end()) {
                pmuStatMap[func_name][new_metric_name].tot = 0.0;
                pmuStatMap[func_name][new_metric_name].n_call = 0;
            }
            pmuStatMap[func_name][new_metric_name].tot += map[new_metric_name];
            pmuStatMap[func_name][new_metric_name].n_call += 1;
            calMap[func_name][new_metric_name].push_back(map[new_metric_name]);
        }
    }
}

void Statistics::cal_avg_std(PMUStatMap &pmuStatMap, CalMap &calMap) {
    for (auto it = pmuStatMap.begin(); it != pmuStatMap.end(); it++) {
        std::string func_name = it->first;
        std::unordered_map <std::string, pmu_stat_t> &pmuMap = it->second;
        for (auto pmui = pmuMap.begin(); pmui != pmuMap.end(); pmui++) {
            std::string pmu_event_name = pmui->first;
            pmu_stat_t &stat = pmui->second;
            //  calculate avg
            stat.avg = stat.tot / (double) stat.n_call;

            // calculate stad
            double tmp = 0;
            for (int i = 0; i < calMap[func_name][pmu_event_name].size(); i++) {
                tmp += (calMap[func_name][pmu_event_name][i] - stat.avg) *
                       (calMap[func_name][pmu_event_name][i] - stat.avg);
            }
            stat.std = sqrt(tmp / (double) stat.n_call);
        }
    }
}

void Statistics::get_metrics_with_rank(PMUStatMap &pmuStatMap, int rank) {
    CalMap calMap;
    get_cal_map_with_rank(rank, pmuStatMap, calMap);
    cal_avg_std(pmuStatMap, calMap);
}

void Statistics::get_metrics_merged(PMUStatMap &pmuStatMap) {
    CalMap calMap;
    for (auto it = _record_collection.begin(), ie = _record_collection.end(); it != ie; ++it) {
        int rank = it->first;
        get_cal_map_with_rank(rank, pmuStatMap, calMap);
    }
    cal_avg_std(pmuStatMap, calMap);
}

void Statistics::get_metrics_dist(std::vector <PMUStatMap> &dist) {
    std::vector<int> rank_list;

    for (auto it = _record_collection.begin(), ie = _record_collection.end(); it != ie; ++it) {
        rank_list.push_back(it->first);
    }

#pragma omp parallel for
    for (int i = 0; i < rank_list.size(); i++) {
        int rank = rank_list[i];
        std::cout << "PMU: " << rank << std::endl;
        PMUStatMap pmuStatMap;
        get_metrics_with_rank(pmuStatMap, rank);
        #pragma omp critical
        {
            dist.push_back(pmuStatMap);
        }
        
    }
}


void builtin_metrics::deriveCPI(
        std::unordered_map<std::string /*Metric name*/, double /*Value*/> &map) {
    if (map.find("PAPI_TOT_INS") == map.end()) {
        JSI_ERROR("Missing metric(PAPI_TOT_INS) while deriving CPI.\n");
    }
    if (map.find("PAPI_TOT_CYC") != map.end()) {
        map["CPI"] = map["PAPI_TOT_INS"] / map["PAPI_TOT_CYC"];
    } else if (map.find("CYCLE") != map.end()) {
        map["CPI"] = map["PAPI_TOT_INS"] / map["CYCLE"];
    } else {
        JSI_ERROR("Missing metric(PAPI_TOT_CYC or CYCLE) while deriving CPI.\n");
    }
}