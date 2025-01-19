#ifndef __JSI_BOTTOMUPTREE_H__
#define __JSI_BOTTOMUPTREE_H__
#include <stdio.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "record/record_type.h"
#include "record/record_reader.h"
#include "record/wrap_defines.h"
#include "instrument/backtrace.h"
#include "utils/tsc_timer.h"

#include "statistic.h"

struct hashFunction {
    size_t operator()(const std::pair<std::string, void *>& x) const{
        size_t h1 = std::hash<std::string>{}(x.first);
        size_t h2 = std::hash<void *>{}(x.second);
        return h1 ^ h2;
    }
};

struct BottomUpNode{
    std::vector<void*> src_ip;
    void* ip = nullptr;
    int ctxt;
    std::string name = "";
    Statistics::func_stat_t stat;

    std::unordered_set<std::pair<std::string/*func name*/, void* /*src_ip*/>, hashFunction> children;
};

class BottomUpTree {
public:
    BottomUpTree();

    BottomUpTree(int rank);

    ~BottomUpTree();
    
    void updateTree(void *src_ip, void *ip, uint64_t ctxt, std::string name, uint64_t cycle, bool is_child);

    void addChild(std::string child_name, std::string parent_name, void *src_ip);

    int get_rank();

    int get_size();
    
    std::string dump();
private:
    int _rank;
    Statistics::time_t _total_time;
    BottomUpNode *_head;
    std::unordered_map<std::string, std::unordered_map<void* /*src ip*/, BottomUpNode*> > _nodes_list;

    void deepSearch(int depth, BottomUpNode *head, std::string& dump_string);
};

class BottomUpForest {
public:
    BottomUpForest(int rank);

    ~BottomUpForest();

    std::string forest_dump();

    void updateForest(int rank, std::string tree_name, void *src_ip, void *ip, uint64_t ctxt, std::string node_name, uint64_t cycle, bool is_child);

    void forestAddChild(std::string tree_name, std::string child_name, std::string parent_name, void *src_ip);

    int forest_get_rank();

private:
    int _rank;
    
    std::unordered_map<std::string/*src name*/, BottomUpTree*> _trees_list;
};


#endif