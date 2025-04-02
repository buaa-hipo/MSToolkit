#include "instrument/MT_PMU_collector.h"
#include "instrument/pmu_collector.h"
#include "record/record_utils.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>

using namespace std;

#ifndef __JSI_RECORD_TYPE_H__
#define JSI_ACCL_API_MSGTYPE_START ((int16_t)(-0x100))
#define JSI_GET_ACCL_API_MSGTYPE(x)                                            \
  (-((int16_t)(x)) + JSI_ACCL_API_MSGTYPE_START)
#endif

#define READER_FILE_NONEXISTENT(file)                                          \
  do {                                                                         \
    cerr << "File - [" << (file) << "] does not exist" << endl;                \
    exit(1);                                                                   \
  } while (0);

#define READER_FAIL_TO_OPEN(file)                                              \
  do {                                                                         \
    cerr << "Failed to open file - [" << file << "]" << endl;                  \
    exit(1);                                                                   \
  } while (0);

class AcclRecordTrace {
private:
    const int16_t dev_pmu_num_;
    size_t trace_size_;
    const char *dev_type_;
    const char *dev_pmu_list_;
    void *trace_;
public:
    AcclRecordTrace(void *trace, int dev_pmu_num, size_t trace_size, const char *dev_pmu_list, const char *dev_type)
        : dev_pmu_num_(dev_pmu_num), trace_(trace), trace_size_(trace_size), dev_pmu_list_(dev_pmu_list), dev_type_(dev_type) {}
    AcclRecordTrace(AcclRecordTrace&) = delete;
    AcclRecordTrace(AcclRecordTrace&&) = delete;
    class Iterator {
    private:
        // size_t pos_;
        const int16_t record_pmu_num_;
        void *cur_;
    public:
        Iterator(void *addr, int num): cur_(addr), record_pmu_num_(num) {}
        bool operator==(const Iterator& other) const {
            return cur_ == other.cur_;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

        size_t record_size() const {
            return record_utils::get_record_size(static_cast<record_t*>(cur_));
        }

        Iterator& operator++() {
            printf("Moving forward for %lu bytes\n", record_size() + 2 * record_pmu_num_ * sizeof(uint64_t));
            cur_ = static_cast<char*>(cur_) + record_size() + 2 * record_pmu_num_ * sizeof(uint64_t);
            return *this;
        }

        void* get() const {
            return cur_;
        }

        std::string to_string() {
            return record_utils::to_string(static_cast<record_t*>(cur_));
        }
    };

    Iterator begin() { return Iterator(trace_, dev_pmu_num_); }
    Iterator end() { return Iterator(static_cast<char*>(trace_) + trace_size_, dev_pmu_num_); }

    const char* get_trace_type() const {
        return dev_type_;
    }

    std::string to_string() {
        std::stringstream result;
        static auto event_list = new std::vector<std::string>();
        if (event_list->empty()) {
            parse_dev_pmu_events_list(dev_pmu_list_, event_list);
        }
        for (auto it = this->begin(); it != this->end(); ++it) {
            printf("Size of record %s: %lu\n", record_utils::get_record_name(static_cast<record_t*>(it.get())).c_str(), it.record_size());
            auto current_record = static_cast<record_t*>(it.get());
            auto record_size = record_utils::get_record_size(current_record);
            auto pmus = (uint64_t*)((char*)current_record + record_utils::get_record_size(current_record));
            result << it.to_string();
            for (int i = 0; i < event_list->size(); ++i) {
                result << (*event_list)[i] << " : (" << pmus[i] << ", " << pmus[i + dev_pmu_num_] << ", " << pmus[i + dev_pmu_num_] - pmus[i] << ")\n";
            }
        }
        return result.str();
    }
};

static int pmu_num;

#ifdef DMA_CALLBACK
void print_kernel_info(filesystem::path &kernel_records) {
  ifstream file(kernel_records, ifstream::in | ifstream::binary);
  if (!file) {
    cerr << "Failed to open file - [" << kernel_records << "]" << endl;
    exit(1);
  }
  printf("\n\nBegin of Kernel Records:\n");
  accl_activity_record_t *record = new accl_activity_record_t;
  while (file.read((char *)record, sizeof(accl_activity_record_t))) {
    printf("{\n");
    printf("\tDomain: %d\n", record->domain);
    printf("\tKind: %d\n", record->kind);
    printf("\tOperation: %d\n", record->op);
    printf("\tCorrelation ID: %lu\n", record->correlation_id);
    printf("\tBegin TS: %lu\n", record->begin_ns);
    printf("\tEnd TS: %lu\n", record->end_ns);
    printf("\tPID: %d\n", record->process_id);
    printf("\tTID: %d\n", record->thread_id);
    printf("\tExtra info: %lu\n", record->kernel_index);
    printf("}\n");
  }
}
#endif

void print_trace(const char *trace) {
  ifstream file(trace, ifstream::in | ifstream::binary);
  //   if ((bool)file) {
  //     cerr << "Failed to open file - [" << host_trace << "]" << endl;
  //     exit(1);
  //   }
  filesystem::path path = trace;
  auto file_size = filesystem::file_size(path);
  cout << "trace size: " << file_size << endl;
  char buf[1024];
  file.read(buf, file_size);
  printf("\n\nBegin of Host Trace:\n");
  AcclRecordTrace accl_record_trace(buf, pmu_num, file_size, "hthread:::CYCLE,hthread:::BRTK", "MATRIX");
  printf("%s", accl_record_trace.to_string().c_str());
}

int main(int argc, char **argv) {
  if (argc < 2) {
    cerr << "Usage: \n mt_trace_reader [trace] [pmu_num]" << endl;
    exit(1);
  }

  if (argc > 2) {
    pmu_num = stoi(argv[2]);
  } else {
    pmu_num = 26;
  }

  print_trace(argv[1]);
}