#include "instrument/pmu_collector.h"
#include "utils/jsi_log.h"
#include "record/record_writer.h"
#include "record/record_type.h"
#include "record/wrap_defines.h"
#include "utils/tsc_timer.h"

#include <iostream>
#include <cassert>
#include <papi.h>
#include <chrono>
#include <thread>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <algorithm>
#include "utils/configuration.h"
#include "utils/safe.hpp"

#define SAMPLING_INIT_PRIORITY (RECORD_INIT_PRIORITY+300)
#define SAMPLING_FINI_PRIORITY (RECORD_FINI_PRIORITY+300)

class PAPISampler {
private:
    double sampling_interval = 1.0;

public:
    PAPISampler() {

        // char* jsi_sampling_interval = getenv("JSI_SAMPLING_INTERVAL");
        // sampling_interval = std::stod(jsi_sampling_interval);
        sampling_interval = EnvConfigHelper::get_double("JSI_SAMPLING_INTERVAL", 0.1/*10Hz by default*/);
        
        startTimers();
    }

    // Set timer callback Function
    static void timer_handler(int signum) {
        // static int count = 0;
        // JSI_INFO("Timer triggered , num of events %d.\n", pmu_collector_get_num_events());
        if(!jsi_safe_enter()) {
            return ;
        }
        record_t * rec = (record_t*) alloca(
            sizeof(record_t) + sizeof(uint64_t) * pmu_collector_get_num_events()
        );
        rec[0].MsgType = (int16_t) event_SAMPLING_Counters;
        rec[0].timestamps.enter = get_tsc_raw();

        if(jsi_backtrace_enabled) {
            rec[0].ctxt = backtrace_context_get();
        }

        if (pmu_collector_get_num_events()>0) {
            uint64_t* counters = RecordWriterHelper::counters(rec);
            int r = pmu_collector_get_all(counters);
        }

        // RecordWriter::traceStore((const record_t *) rec);
        RecordWriter::samplingStore(event_SAMPLING_Counters, rec);
        DEALLOCATE(rec, sizeof(record_t) + sizeof(uint64_t) * 2);
        jsi_safe_exit();
    }

    // Initial timer setting
    void startTimers() {
        struct sigaction sa;
        struct itimerval timer;

        // Set Function to handle signal
        sa.sa_handler = &timer_handler;
        sa.sa_flags = SA_RESTART;
        sigaction(SIGALRM, &sa, NULL);

        int sec = static_cast<int>(sampling_interval);
        int usec = static_cast<int>((sampling_interval - sec) * 1000000);

        // Set timer
        // The trigger interval is (tv_sec + tv_usec)
        // tc_usec is set among (0, 999999]
        // JSI_INFO("sec is : %d, usec is : %d\n", sec, usec);
        timer.it_value.tv_sec = sec;
        timer.it_value.tv_usec = usec;
        timer.it_interval.tv_sec = sec;
        timer.it_interval.tv_usec = usec;

        // Start timer
        setitimer(ITIMER_REAL, &timer, NULL);
    }

    // Cleaning Function
    static void cleaning() {
        struct itimerval zero_timer;
        zero_timer.it_value.tv_sec = 0;
        zero_timer.it_value.tv_usec = 0;
        zero_timer.it_interval.tv_sec = 0;
        zero_timer.it_interval.tv_usec = 0;
        setitimer(ITIMER_REAL, &zero_timer, NULL);
    }
};

PAPISampler* globalPAPISampler;

__attribute__((constructor (SAMPLING_INIT_PRIORITY)))
void jsi_sampling_init(){
    globalPAPISampler = new PAPISampler;

    JSI_LOG(JSILOG_INFO, "Initialize JSI SAMPLING Library.\n");
}

__attribute__((destructor (SAMPLING_FINI_PRIORITY)))
void jsi_sampling_finalize(){
    globalPAPISampler->cleaning();
    JSI_LOG(JSILOG_INFO, "Finalize JSI SAMPLING Library.\n");
}