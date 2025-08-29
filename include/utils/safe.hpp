// Local safe guard utilities
#ifndef __SAFE_H__
#define __SAFE_H__

extern thread_local bool in_tool;
extern thread_local bool unsafe_state;
extern thread_local bool thread_data_initialized;

using ThreadSafeGuardCallback = void(*)(void);

void _safe_enter();
void _safe_thread_init();
void _safe_thread_finalize();
void registerThreadSafeGuard(ThreadSafeGuardCallback init_cb, ThreadSafeGuardCallback fini_cb);

inline
void jsi_thread_init() {
    // unsafe_state = false;
    // thread_data_initialized = true;
    _safe_thread_init();
}

inline
void jsi_thread_finalize() {
    // unsafe_state = true;
    // thread_data_initialized = false;
    _safe_thread_finalize();
}

inline
bool jsi_safe_enter() {
    if (in_tool || !thread_data_initialized || unsafe_state) { return false; }
    in_tool = true;
    return true;
}

inline
void jsi_safe_exit() {
    in_tool = false; 
}

inline
void jsi_safe_enter_instr() {
    _safe_enter();
    in_tool = true;
}

inline
void jsi_safe_exit_instr() {
    in_tool = false;
}

inline
void jsi_mark_unsafe_enter() {
    unsafe_state = true;
}

inline
void jsi_mark_unsafe_exit() {
    unsafe_state = false;
}

#endif