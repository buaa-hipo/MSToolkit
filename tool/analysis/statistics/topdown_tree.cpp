#include "topdown_tree.h"

#include <cmath>
#include <iostream>


TopDownTree::TopDownTree() {
}

TopDownTree::TopDownTree(int rank) {
    _rank = rank;
}

TopDownTree::~TopDownTree() {
    _nodes_list.clear();
}

void TopDownTree::updateTree(void *ip, uint64_t ctxt, std::string name, uint64_t cycle, bool is_child) {
    if (_nodes_list.find(ip) == _nodes_list.end()) {
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
        node.is_leaf = is_child;
        _nodes_list.insert(std::pair<void*, TopDownNode>(ip, node));
        if (ctxt == BACKTRACE_ROOT_NODE) {
            // std::cout << "ROOT NODE: " << name << std::endl;
            _head = node;
        }
    }
    else {
        TopDownNode& node = _nodes_list[ip];
        if (is_child) {
            node.stat.n_call += 1;
            node.is_leaf = true;
            node.stat.square_tot.cycle += cycle * cycle;
            node.stat.square_avg.cycle = node.stat.square_tot.cycle / (double)node.stat.n_call;
            node.stat.std_time.cycle = sqrt(node.stat.square_avg.cycle - node.stat.avg_time.cycle * node.stat.avg_time.cycle);
        }
        node.stat.tot_time.cycle += cycle;
        node.stat.avg_time.cycle = node.stat.tot_time.cycle / (double)node.stat.n_call;
        if (ctxt == BACKTRACE_ROOT_NODE) {
            // std::cout << "ROOT NODE: " << name << std::endl;
            _head = node;
        }
    }
}

void TopDownTree::addChild(void *parent_ip, void *child_ip) {
    TopDownNode& node = _nodes_list[parent_ip];
    node.children.emplace(child_ip);
    if (node.ctxt == BACKTRACE_ROOT_NODE) {
        _head = node;
    }
}

int TopDownTree::get_rank() {
    return _rank;
}

Statistics::time_t TopDownTree::get_total_time() {
    return _total_time;
}

void TopDownTree::set_total_time(Statistics::time_t total_time) {
    _total_time = total_time;
}

std::string TopDownTree::dump() {
    std::string dump_string;
    this->deepSearch(0, _head, dump_string);
    return dump_string;
}

void TopDownTree::deepSearch(int depth, TopDownNode& head, std::string& dump_string) {
    //DEBUG
    // std::cout << "head.ctxt: " << head.ctxt << std::endl;
    // std::cout << "head.name: " << head.name << std::endl;
    // std::cout << "head.ip: " << head.ip << std::endl;
    // std::cout << "count children: " << head.children.size() << std::endl;
    std::string offset;
    std::string header;
    for (int i = 0; i < depth; i++) {
        offset += "\t";
    }
    double percentage = (double)head.stat.tot_time.cycle / (double)_total_time.cycle * 100;
    dump_string += offset + "├──" + head.name + " ———— " + std::to_string(percentage) + "%\n";
    header = offset + "*";
    dump_string += header + "Total time: " + std::to_string(head.stat.tot_time.sec()) + "(s)\n";
    dump_string += header + "Average time: " + std::to_string(head.stat.avg_time.sec()) + "(s)\n";
    dump_string += header + "Standard deviation: " + std::to_string(head.stat.std_time.sec()) + "\n";
    dump_string += header + "Call times: " + std::to_string(head.stat.n_call) + "\n";

    if (head.children.size() > 0) {
        depth += 1;
        for (auto ip: head.children) {
            auto node = _nodes_list[ip];
            this->deepSearch(depth, node, dump_string);
        }
    }
}

std::unordered_map<void*, TopDownNode>& TopDownTree::get_nodes_list() {
    return _nodes_list;
}

void TopDownTree::set_head(TopDownNode& head) {
    _head = head;
}

TopDownNode TopDownTree::get_head() {
    return _head;
}
