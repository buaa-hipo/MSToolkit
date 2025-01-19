#include "instrument/backtrace.h"
#include <fsl/raw_backend.h>
#include <filesystem>
using namespace pse;
bool has_error = 0;

backtrace_context_t g_ctx;

void write_backtrace_test()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::WRITE);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    auto root = backend.openRootSection();
    auto dir = root->openDirSection(1, true);

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

    backtrace_db_dump(dir);
    backtrace_finalize();

    puts("====================================================");
}
void read_backtrace_test()
{
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::READ);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    auto root = backend.openRootSection();
    auto dir = root->openDirSection(1, true);
    auto bt_tree = jsi::toolkit::BacktraceTree::create_loading_bt_tree();
    bt_tree->backtrace_db_load(dir);

    puts(bt_tree->backtrace_get_context_string(g_ctx, 10));
    puts(bt_tree->backtrace_get_context_string(g_ctx));
    printf("g_ctx == %d\n", g_ctx);

    delete bt_tree;
}

int main()
{
    write_backtrace_test();
    read_backtrace_test();
    if (!has_error)
    spdlog::info("pass");
    else
    spdlog::error("failed");
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
}
