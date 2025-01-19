#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sstream>

// #include "record/record_writer.h"
#include "utils/jsi_log.h"

class EnvConfigHelper {
    public:
    static inline __attribute__((always_inline))
    std::string get(const char* envname, std::string v_default, const std::vector<std::string>& valid_value) {
        char* val = getenv(envname);
        if(val==NULL) {
            return v_default;
        }
        std::string str = std::string(val);
        int n = valid_value.size();
        if(n!=0) {
            int i;
            for(i=0; i<n; ++i) {
                if(str==valid_value[i]) {
                    break;
                }
            }
            if(i==n) {
                std::stringstream sstream;
                for(int i=0; i<n; ++i) {
                    sstream << valid_value[i] << ",";
                }
                JSI_ERROR("Error when parsing Configuration %s: invalid value %s. The valid value must be: %s\n", envname, val, sstream.str().c_str());
            }
        }
        return str;
    }
    
    static inline __attribute__((always_inline))
    std::string get(const char* envname, std::string v_default) {
        return get(envname, v_default, {});
    }

    static inline __attribute__((always_inline))
    int get_int(const char* envname, int v_default) {
        char* val = getenv(envname);
        if(val==NULL) {
            return v_default;
        }
        int res;
        std::stringstream sstream;
        sstream << val;
        sstream >> res;
        if(sstream.fail()){
            JSI_ERROR("Invalid %s configuration value %s: cannot convert to integer\n", envname, val);
        }
        return res;
    }

    static inline __attribute__((always_inline))
    double get_double(const char* envname, double v_default) {
        char* val = getenv(envname);
        if(val==NULL) {
            return v_default;
        }
        double res;
        std::stringstream sstream;
        sstream << val;
        sstream >> res;
        if(sstream.fail()){
            JSI_ERROR("Invalid %s configuration value %s: cannot convert to douebl\n", envname, val);
        }
        return res;
    }

    // ON - return true; OFF - return false
    static inline __attribute__((always_inline))
    bool get_enabled(const char* envname, bool v_default) {
        char* val = getenv(envname);
        if(val==NULL) {
            return v_default;
        }
        std::string str(val);
        if(str=="ON") {
            return true;
        }
        if(str=="OFF") {
            return false;
        }
        JSI_ERROR("Invalid %s configuration value %s: Should be ON/OFF\n", envname, val);
        return v_default;
    }
};