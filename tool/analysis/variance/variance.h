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
	    int rank;
        record_t* data;
    } CommVarianceRecord;

    typedef struct {
        double variance;
	    int rank;
        record_t* p_rec;
        record_t* c_rec;
        uint64_t count;
    } CalcVarianceRecord;

#define CalcVarianceRecordGetEnterTS(p_cvr) (p_cvr->p_rec->timestamps.exit)
#define CalcVarianceRecordGetExitTS(p_cvr) (p_cvr->c_rec->timestamps.enter)

    typedef struct {
        CommVarianceRecord comm_rec;
        uint64_t accl_enter;
        uint64_t accl_exit;
    } AcclVarianceRecord;

    typedef struct {
        std::vector<CommVarianceRecord*> comm_vars;
        std::vector<CalcVarianceRecord*> calc_vars;
        std::vector<AcclVarianceRecord*> accl_calc_vars;
        std::vector<AcclVarianceRecord*> accl_memcpy_vars;
        uint64_t offset;
        uint64_t gpu_offset; // as gpu event uses different clock, we use brute force aligntment with the first gpu event.
    } VarianceData;

    VarianceMap(RecordTraceCollection& collection, BacktraceCollection& bt_collection, RecordTraceExtCollection& rte_collection, const std::string& ref_metric, bool enable_async_comm);
    ~VarianceMap();

    const std::unordered_map<int, VarianceData*>& getVarMap();

    /* Dump CSV to draw heatmap in microsecond */
    void dump_comm_csv(const char* filename);
    void dump_calc_csv(const char* filename);
    void dump_accl_calc_csv(const char* filename);
    void dump_accl_memcpy_csv(const char* filename);

    void dump_comm_heatmap(const char* filename, uint64_t resolution_ms=100);
    void dump_calc_heatmap(const char* filename, uint64_t resolution_ms=100);
    void dump_accl_calc_heatmap(const char* filename, uint64_t resolution_ms=100);
    void dump_accl_memcpy_heatmap(const char* filename, uint64_t resolution_ms=100);

  private:
    std::unordered_map<int/*rank*/, VarianceData*> _var_map;
    std::string _ref_metric;
    bool enable_accl;
};
