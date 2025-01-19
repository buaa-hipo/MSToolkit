#include <fsl/fsl_section.hpp>
#include <spdlog/spdlog.h>
using namespace pse::fsl;
struct R1
{
    int a;
    float b;
    uint64_t time;
};
constexpr int len = 1024 * 1024;
void write_test()
{
    auto manager = FSL_Backend::create("b.data", pse::ral::WRITE);
    auto root = manager->openRootSec();
    auto dir1 = root->openDir(1, true);
    auto dir2 = dir1->openDir(2, true);
    auto dataSec = dir2->openData<R1>(3, true);
    for (int i = 0; i < len; ++i)
    {
        R1 r{i, (float)i, (uint64_t)i};
        dataSec->writeRecord(i, &r);
    }
}
void read_test()
{
    auto manager = FSL_Backend::create("b.data", pse::ral::READ);
    auto root = manager->openRootSec();
    auto dir1 = root->openDir(1, false);
    auto dir2 = dir1->openDir(2, false);
    auto dataSec = dir2->openData<R1>(3, false);
    for (int i = 0; i < len; ++i)
    {
        R1 r;
        dataSec->readRecord(i, &r);
        if (r.a != i || r.b != i || r.time != i)
        {
            spdlog::error("test failed");
            exit(-1);
        }
    }
}

int main()
{
    write_test();
    read_test();
    spdlog::info("pass");
    return 0;
}