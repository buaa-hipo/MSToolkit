#ifndef __JSI_STG_H__
#define __JSI_STG_H__

#include <stdint.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <set>

typedef uint64_t STG_Key_t; /* current ip OR backtrace context*/
// per rank state tranfer graph for online loop detection & counting
struct STG_Val_t {
    // record counts for temporal sampling, count from 0
    uint64_t count;
};
typedef std::pair<STG_Key_t/*src*/, STG_Key_t/*dst*/> STG_Edge_t;

struct edge_hash {
    std::size_t operator() (const STG_Edge_t &edge) const
    {
        return edge.first ^ edge.second;
    }
};

class STG {
  public:
    STG() {}
    ~STG() {}
    STG_Val_t* transfer_to(STG_Key_t key);
    void print();
    std::string to_string();
    void fprint(const char* filename);
  private:
    std::unordered_map<STG_Key_t, STG_Val_t> nodes;
    std::unordered_map<STG_Edge_t, uint64_t/*count*/, edge_hash> edges;
};

STG* createOrGetSTG();

#endif