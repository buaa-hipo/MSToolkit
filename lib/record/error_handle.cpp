#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 打印调用栈
void print_stack_trace() {
    void *buffer[100];
    int size = backtrace(buffer, 100);
    fprintf(stderr, "=== Segmentation Fault (SIGSEGV) ===\n");
    backtrace_symbols_fd(buffer, size, STDERR_FILENO);
}

// 信号处理函数
void sigsegv_handler(int sig, siginfo_t *info, void *context) {
    (void)context;  // 避免未使用警告

    fprintf(stderr, "Caught signal %d (SIGSEGV), faulting address: %p\n", sig, info->si_addr);
    print_stack_trace();

    // 终止程序
    _exit(EXIT_FAILURE);
}

// 初始化信号拦截
__attribute__((constructor (101)))
void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sigsegv_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}