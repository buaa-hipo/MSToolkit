#ifndef __JSI_TOPDOWNTREE_H__
#define __JSI_TOPDOWNTREE_H__
#include <stdio.h>
#include <vector>
#include <unordered_set>
#include "record/record_type.h"
#include "record/record_reader.h"
#include "record/wrap_defines.h"
#include "instrument/backtrace.h"
#include "utils/tsc_timer.h"

#include "statistic.h"

#define TOP_DOWN_ROOT (0)

struct TopDownNode{
    void* ip = nullptr;
    int ctxt;
    std::string name = "";
    bool is_leaf = false;
    Statistics::func_stat_t stat;

    std::unordered_set<void*> children;
};

class TopDownTree {
public:
    TopDownTree(int rank);


    TopDownTree();

    ~TopDownTree();
    
    void updateTree(void *ip, uint64_t ctxt, std::string name, uint64_t cycle, bool is_child);

    void addChild(void *parent_ip, void *child_ip);

    int get_rank();

    Statistics::time_t get_total_time();

    void set_total_time(Statistics::time_t total_time);

    std::string dump();


    std::unordered_map<void*, TopDownNode>& get_nodes_list();

    void set_head(TopDownNode&);

    TopDownNode get_head();

private:
    int _rank;
    Statistics::time_t _total_time;
    TopDownNode _head;
    std::unordered_map<void*, TopDownNode> _nodes_list;
    void deepSearch(int depth, TopDownNode& head, std::string& dump_string);

};

#endif