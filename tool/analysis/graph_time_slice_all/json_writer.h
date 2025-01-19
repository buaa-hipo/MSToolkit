#include "utils/rapidjson/prettywriter.h"
#include "utils/rapidjson/stringbuffer.h"
#include "utils/jsi_log.h"

using namespace rapidjson;

class ChromeTraceWriter {
  private:
    FILE* fp;
    StringBuffer s;
    PrettyWriter<StringBuffer> writer;
  public:
    class Arg {
      public:
        enum DataType {
            STR, U32, U64, I32, I64
        };
        std::string name;
        uint32_t data_type;
        union {
            const char* str;
            uint32_t u32;
            uint64_t u64;
            int32_t i32;
            int64_t i64;
        } val;
        
        Arg(std::string arg_name, int arg) {
            name = arg_name;
            data_type = DataType::I32;
            val.i32=arg; 
        }

        Arg(std::string arg_name, int64_t arg) {
            name = arg_name;
            data_type = DataType::I64;
            val.i64=arg; 
        }

        Arg(std::string arg_name, uint32_t arg) {
            name = arg_name;
            data_type = DataType::U32;
            val.u32=arg; 
        }

        Arg(std::string arg_name, uint64_t arg) {
            name = arg_name;
            data_type = DataType::U64;
            val.u64=arg; 
        }

        Arg(std::string arg_name, std::string arg) {
            name = arg_name;
            data_type = DataType::STR;
            val.str=arg.c_str(); 
        }

        Arg(std::string arg_name, const char* arg) {
            name = arg_name;
            data_type = DataType::STR;
            val.str = arg;
        }

        static void write(PrettyWriter<StringBuffer>& writer, Arg& arg) {
            writer.Key(arg.name.c_str());
            switch(arg.data_type) {
                case STR:
                    writer.String(arg.val.str);
                    break;
                case U32:
                    writer.Uint(arg.val.u32);
                    break;
                case U64:
                    writer.Uint64(arg.val.u64);
                    break;
                case I32:
                    writer.Int(arg.val.i32);
                    break;
                case I64:
                    writer.Int64(arg.val.i64);
                    break;
            }
        }
    };
    ChromeTraceWriter(const char* filename) : writer(s) {
        fp = fopen(filename, "w");
        if(fp==NULL) {
            JSI_ERROR("Cannot open file for write: %s\n", filename);
        }
        writer.StartObject();
        writer.Key("traceEvents");
        writer.StartArray();
    }
    ~ChromeTraceWriter() {
        writer.EndArray();
        // Use default time unit: us
        // writer.Key("displayTimeUnit");
        // writer.String("ns");
        writer.EndObject();
        fwrite(s.GetString(), 1, s.GetSize(), fp);
        fclose(fp);
    }
    void add_event(std::string name, std::string cat, std::string ph, double ts, int pid, int tid, int id, std::vector<Arg>& args) {
        writer.StartObject();
        writer.Key("name");
        writer.String(name.c_str());
        writer.Key("cat");
        writer.String(cat.c_str());
        writer.Key("ph");
        writer.String(ph.c_str());
        writer.Key("ts");
        writer.Double(ts);
        writer.Key("pid");
        writer.Int(pid);
        writer.Key("tid");
        writer.Int(tid);
        writer.Key("id");
        writer.Int(id);
        int n=args.size();
        if(n>0) {
            writer.Key("args");
            writer.StartObject();
            for(int i=0; i<n; ++i) {
                Arg::write(writer, args[i]);
            }
            writer.EndObject();
        }
        writer.EndObject();
    }

    void add_complete_event(std::string name, std::string cat, std::string ph, int ts_id, double ts, double dur, int pid, int tid, std::vector<Arg>& args) {
        writer.StartObject();
        writer.Key("name");
        writer.String(name.c_str());
        writer.Key("cat");
        writer.String(cat.c_str());
        writer.Key("ph");
        writer.String(ph.c_str());
        writer.Key("ts_id");
        writer.Int(ts_id);
        writer.Key("ts");
        writer.Double(ts);
        writer.Key("dur");
        writer.Double(dur);
        writer.Key("pid");
        writer.Int(pid);
        writer.Key("tid");
        writer.Int(tid);
        int n=args.size();
        if(n>0) {
            writer.Key("args");
            writer.StartObject();
            for(int i=0; i<n; ++i) {
                Arg::write(writer, args[i]);
            }
            writer.EndObject();
        }
        writer.EndObject();
    }

    void add_duration_event(std::string name, std::string cat, int ts_id, double ts_beg, double ts_end, int pid, int tid, std::vector<Arg>& args) {
        add_complete_event(name, cat, "X", ts_id, ts_beg, ts_end-ts_beg, pid, tid, args);
    }
    void add_flow_start_event(std::string name, std::string cat, double ts, int pid, int tid, int flow_id, std::vector<Arg>& args) {
        add_event(name, cat, "s", ts, pid, tid, flow_id, args);
    }
    void add_flow_end_event(std::string name, std::string cat, double ts, int pid, int tid, int flow_id, std::vector<Arg>& args) {
        add_event(name, cat, "t", ts, pid, tid, flow_id, args);
    }
};