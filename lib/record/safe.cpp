#include "utils/safe.hpp"
#include <stdio.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include "utils/jsi_log.h"

thread_local bool in_tool=false;
thread_local bool unsafe_state=false;
thread_local bool thread_data_initialized=false;

std::vector<ThreadSafeGuardCallback>* init_cb_list=nullptr;
std::vector<ThreadSafeGuardCallback>* fini_cb_list=nullptr;



static uint32_t get_pid() {
    static auto pid = getpid();
    return pid;
}

static int get_tid() {
    thread_local static auto tid = syscall(SYS_gettid);
    return tid;
}

void _safe_thread_init() {
    if (thread_data_initialized) {
        JSI_INFO("thread data already initialized.");
        return ;
    }
    if (init_cb_list!=nullptr) {
        for(int i=0; i<init_cb_list->size(); ++i) {
            JSI_INFO("[DEBUG][PID=%d, TID=%d] THREAD INIT for i=%d, init=%p\n", get_pid(), get_tid(), i, (*init_cb_list)[i]);
            (*init_cb_list)[i]();
        }
    } else {
        JSI_DEBUG("[PID=%d, TID=%d] No initialize function REGISTERED\n", get_pid(), get_tid());
    }
    thread_data_initialized = true;
    JSI_INFO("[DEBUG][PID=%d, TID=%d] THREAD INITIALIZED\n", get_pid(), get_tid());
}

void _safe_thread_finalize() {
    if (!thread_data_initialized) {
        JSI_INFO("thread data not initialized.");
        return ;
    }
    if (fini_cb_list!=nullptr) {
        for(int i=0; i<fini_cb_list->size(); ++i) {
            JSI_INFO("[DEBUG][PID=%d, TID=%d] THREAD FINALIZE for i=%d, fini=%p\n", get_pid(), get_tid(), i, (*fini_cb_list)[i]);
            (*fini_cb_list)[i]();
        }
    } else {
        JSI_DEBUG("[PID=%d, TID=%d] No finalize function REGISTERED\n", get_pid(), get_tid());
    }
    thread_data_initialized = false;
    JSI_INFO("[DEBUG][PID=%d, TID=%d] THREAD FINALIZED\n", get_pid(), get_tid());
}

void _safe_enter() {
    JSI_DEBUG("[JSI DEBUG PID=%d, TID=%d] tried to safe enter: thread_initialized=%d, unsafe_state=%d, in_tool=%d\n", get_pid(), get_tid(), thread_data_initialized, unsafe_state, in_tool);
    if (unsafe_state) {
        exit(1);
    }
    if (!thread_data_initialized) {
        _safe_thread_init();
    }
}

// global, not thread-safe
void registerThreadSafeGuard(ThreadSafeGuardCallback init_cb, ThreadSafeGuardCallback fini_cb) {
    if(!init_cb_list) {
        init_cb_list = new std::vector<ThreadSafeGuardCallback>();
        fini_cb_list = new std::vector<ThreadSafeGuardCallback>();
    }
    if(init_cb) {
        JSI_DEBUG("REGISTER for init_cb: init_cb=%p\n", init_cb);
        init_cb_list->push_back(init_cb);
    }
    if(fini_cb) {
        JSI_DEBUG("REGISTER for fini_cb: fini_cb=%p\n", fini_cb);
        fini_cb_list->push_back(fini_cb);
    }
}