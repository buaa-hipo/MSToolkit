#include "instrument/stg.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include "utils/jsi_log.h"

using namespace std;

static STG_Key_t  cur_stg_key = 0;
static STG_Val_t* cur_stg_val = 0;

static STG stg;
STG* createOrGetSTG() {
    return &stg;
}

STG_Val_t* STG::transfer_to(STG_Key_t key) {
    if(!nodes.empty()) {
        auto e = make_pair(cur_stg_key, key);
        auto ie = edges.find(e);
        if(ie!=edges.end()) {
            ++(ie->second);
        } else {
            edges[e] = 1;
        }
    }
    cur_stg_key = key;
    auto it = nodes.find(key);
    if(it!=nodes.end()) {
        // already exist
        ++(it->second.count);
        cur_stg_val = &(it->second);
        return cur_stg_val;
    } else {
        // add new stg
        STG_Val_t val;
        val.count = 0;
        nodes[key] = val;
        cur_stg_val = &nodes[key];
        return cur_stg_val;
    }
    return NULL;
}

string STG::to_string() {
    stringstream sstream;
    sstream << "# Nodes\n";
    for(auto it=nodes.begin(), ie=nodes.end(); it!=ie; ++it) {
        STG_Val_t& val = it->second;
        sstream << it->first << "," << val.count << "\n";
    }
    sstream << "# Edges\n";
    for(auto it=edges.begin(), ie=edges.end(); it!=ie; ++it) {
        sstream << it->first.first << "," << it->first.second << "," << it->second << "\n";
    }
    return sstream.str();
}

void STG::print() {
    cout << to_string() << endl;
}

void STG::fprint(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if(fp==NULL) {
        JSI_WARN("failed to open file: %s\n", filename);
        return ;
    }
    fprintf(fp, "%s\n", to_string().c_str());
    fclose(fp);
}