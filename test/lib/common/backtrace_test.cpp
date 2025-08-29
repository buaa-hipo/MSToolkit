#include "instrument/backtrace.h"

#include <iostream>

backtrace_context_t g_ctx;

int main() {
    backtrace_init_recording(10);
    auto ctx = backtrace_context_get();
    [=](int n) {
        auto ctx2 = backtrace_context_get();
        backtrace_context_print(ctx, 10);
        std::cout << "n == " << n << std::endl;
        backtrace_context_print(ctx2, 10);
    }(5);

    [=]() {
        [=]() {
            [=]() {
                [=]() {
                    g_ctx = backtrace_context_get();
                    backtrace_context_print(g_ctx, 10);
                }();
            }();
        }();
    }();

    auto f = fopen("a.bin", "wb");
    backtrace_db_dump(f);
    fclose(f);
    backtrace_finalize();

    puts("====================================================");

    auto bt_tree = jsi::toolkit::BacktraceTree::create_loading_bt_tree();
    f = fopen("a.bin", "rb");
    bt_tree->backtrace_db_load(f);

    puts(bt_tree->backtrace_get_context_string(g_ctx, 10));
    puts(bt_tree->backtrace_get_context_string(g_ctx));
    printf("g_ctx == %d\n", g_ctx);

    delete bt_tree;

    return 0;
}