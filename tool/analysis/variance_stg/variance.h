#include <unordered_map>
#include <vector>
#include "record/record_reader.h"

class VarianceMap {
  public:
    enum TimeUnit {
        NSEC, USEC, MSEC, SEC
    };
    typedef struct {
        double variance;
        record_t* data;
    } CommVarianceRecord;

    typedef struct {
        double variance;
        uint64_t ins_count;
        uint64_t ts_enter;
        uint64_t ts_exit;
    } CalcVarianceRecord;

    typedef struct {
        std::vector<CommVarianceRecord*> comm_vars;
        std::vector<CalcVarianceRecord*> calc_vars;
        uint64_t offset;
    } VarianceData;

    VarianceMap(RecordTraceCollection& collection, BacktraceCollection& bt_collection, const std::string& ref_metric);
    ~VarianceMap();

    const std::unordered_map<int, VarianceData*>& getVarMap();

    /* Dump CSV to draw heatmap in microsecond */
    void dump_comm_csv(const char* filename);
    void dump_calc_csv(const char* filename);

  private:
    std::unordered_map<int/*rank*/, VarianceData*> _var_map;
    std::string _ref_metric;
};