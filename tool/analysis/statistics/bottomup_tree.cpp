#include "bottomup_tree.h"

#include <cmath>
#include <iostream>

#define DEBUG std::cout << "DEBUG" << std::endl

BottomUpForest::BottomUpForest(int rank) {
    _rank = rank;
}

BottomUpForest::~BottomUpForest() {
    _trees_list.clear();
}

void BottomUpForest::updateForest(int rank, std::string tree_name, void *src_ip, void *ip, uint64_t ctxt, std::string node_name, uint64_t cycle, bool is_child) {
    if (_trees_list.find(tree_name) == _trees_list.end() && is_child) {
        BottomUpTree *tree = new BottomUpTree(rank);
        tree->updateTree(src_ip, ip, ctxt, node_name, cycle, true);
        _trees_list.insert(std::pair<std::string, BottomUpTree*>(tree_name, tree));
    }
    else {
        BottomUpTree *tree = _trees_list[tree_name];
        // std::cout << "tree size: " << tree->get_size() << std::endl;
        tree->updateTree(src_ip, ip, ctxt, node_name, cycle, is_child);
    }   
}

void BottomUpForest::forestAddChild(std::string tree_name, std::string child_name, std::string parent_name, void *src_ip) {
    BottomUpTree* tree = _trees_list[tree_name];
    tree->addChild(child_name, parent_name, src_ip);
}

int BottomUpForest::forest_get_rank() {
    return _rank;
}

std::string BottomUpForest::forest_dump() {
    std::string res = "";
    for (auto pair: _trees_list) {
        BottomUpTree *tree = pair.second;
        res += tree->dump();
        res += "\n";
        res += "=================================================================================================\n";
    }
    return res;
}

BottomUpTree::BottomUpTree(int rank) {
    _rank = rank;
}

BottomUpTree::BottomUpTree(){};

BottomUpTree::~BottomUpTree() {
    _nodes_list.clear();
}

void BottomUpTree::updateTree(void *src_ip, void *ip, uint64_t ctxt, std::string name, uint64_t cycle, bool is_child) {
    // std::cout << "name: " << name << "  _nodes_list size: " << _nodes_list.size() << std::endl;
    if (_nodes_list.find(name) != _nodes_list.end()) {
        if (is_child) {
            BottomUpNode& node = *((_nodes_list[name].begin())->second);
            node.src_ip.push_back(src_ip);
            node.stat.n_call += 1;
            node.stat.tot_time.cycle += cycle;
            node.stat.avg_time.cycle = node.stat.tot_time.cycle / (double)node.stat.n_call;
            node.stat.square_tot.cycle += cycle * cycle;
            node.stat.square_avg.cycle = node.stat.square_tot.cycle / (double)node.stat.n_call;
            node.stat.std_time.cycle = sqrt(node.stat.square_avg.cycle - node.stat.avg_time.cycle * node.stat.avg_time.cycle);
        }
        else {
            if (_nodes_list[name].find(src_ip) != _nodes_list[name].end()) {
                BottomUpNode& node = *(_nodes_list[name][src_ip]);
                node.stat.tot_time.cycle += cycle;
                node.stat.avg_time.cycle = node.stat.tot_time.cycle / (double)node.stat.n_call;
            }
            else {
                BottomUpNode *node = new BottomUpNode;
                node->ip = ip;
                node->src_ip.push_back(src_ip);
                node->name = name;
                node->ctxt = ctxt;
                node->stat.tot_time.cycle = cycle;
                node->stat.avg_time.cycle = cycle;
                node->stat.square_tot.cycle = cycle * cycle;
                node->stat.square_avg.cycle = cycle * cycle;
                node->stat.std_time.cycle = 0;
                node->stat.n_call = 1;
                _nodes_list[name].insert(std::pair<void *, BottomUpNode*>(src_ip, node));
            }
        }
    }
    else {
        // std::cout << "node_name: " << name << std::endl;
        BottomUpNode *node = new BottomUpNode;
        std::unordered_map<void *, BottomUpNode*> map;
        node->ip = ip;
        node->src_ip.push_back(src_ip);
        node->name = name;
        node->ctxt = ctxt;
        node->stat.tot_time.cycle = cycle;
        node->stat.avg_time.cycle = cycle;
        node->stat.square_tot.cycle = cycle * cycle;
        node->stat.square_avg.cycle = cycle * cycle;
        node->stat.std_time.cycle = 0;
        node->stat.n_call = 1;
        map.insert(std::pair<void *, BottomUpNode*>(src_ip, node));
        _nodes_list.insert(std::pair<std::string, std::unordered_map<void *, BottomUpNode*> >(name, map));
        // std::cout << "tree_size: " << this->get_size() << std::endl;
        if (is_child) {
            _head = node;
        }
    }
}

void BottomUpTree::addChild(std::string child_name, std::string parent_name, void *src_ip) {
    // std::cout << "child_name: " << child_name << "  " << "parent_name: " << parent_name << std::endl;
    BottomUpNode* node = _nodes_list[parent_name][src_ip];
    std::pair<std::string, void *> pair = std::make_pair(child_name, src_ip);
    auto& children = node->children;
    // std::cout << "CHILD: " << pair.first << "         src_ipa: " << src_ip << std::endl;
    children.insert(pair);
    // std::cout << "child.size: " << children.size() << std::endl;
}

int BottomUpTree::get_rank() {
    return _rank;
}

int BottomUpTree::get_size() {
    return _nodes_list.size();
}

void BottomUpTree::deepSearch(int depth, BottomUpNode *head, std::string& dump_string) {
    //DEBUG
    // std::cout << "head.ctxt: " << head.ctxt << std::endl;
    // std::cout << "head.name: " << head->name << std::endl;
    // std::cout << "head.ip: " << head.ip << std::endl;
    // std::cout << "count children: " << head->children.size() << std::endl;
    std::string offset;
    std::string header;
    for (int i = 0; i < depth; i++) {
        offset += "\t";
    }
    // double percentage = (double)head.stat.tot_time.cycle / (double)_total_time.cycle * 100;
    dump_string += offset + "├──" + head->name + "\n"; 
    // dump_string += offset + "├──" + head.name + " ———— " + std::to_string(percentage) + "%\n";
    header = offset + "*";
    dump_string += header + "Total time: " + std::to_string(head->stat.tot_time.sec()) + "(s)\n";
    dump_string += header + "Average time: " + std::to_string(head->stat.avg_time.sec()) + "(s)\n";
    dump_string += header + "Standard deviation: " + std::to_string(head->stat.std_time.sec()) + "\n";
    dump_string += header + "Call times: " + std::to_string(head->stat.n_call) + "\n";

    if (head->children.size() > 0) {
        depth += 1;
        for (auto it: head->children) {
            std::string func_name = it.first;
            void *src_ip = it.second;
            auto node = _nodes_list[func_name][src_ip];
            deepSearch(depth, node, dump_string);
        }
    }
}

std::string BottomUpTree::dump() {
    std::string dump_string;
    // std::cout << _head.name << std::endl;
    deepSearch(0, _head, dump_string);
    return dump_string;
}