// Local safe guard utilities
#ifndef __SAFE_H__
#define __SAFE_H__

static thread_local bool in_tool=false;

bool jsi_safe_enter() {
    if (in_tool) { return false; }
    in_tool = true;
    return true;
}

void jsi_safe_exit() {
    in_tool = false; 
}

#endif