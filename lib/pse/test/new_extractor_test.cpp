#include <ral/backend.h>
#include <fsl/raw_backend.h>
#include <ral/extractor.h>
#include <filesystem>
using namespace pse;
#pragma pack(1)
struct R1
{
    int a;
    int64_t time;
};

struct R2
{
    int a;
    int b;
    int c;
    int64_t time;
};
struct R3
{
    int64_t time;
    int a;
    int b;
    int c;
};

void write_test()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::WRITE);
    auto _backend = ral::BackendWrapper(std::move(raw_backend));
    ral::BackendInterface &backend = static_cast<pse::ral::BackendInterface &>(_backend);
    auto root = backend.openRootSection();
    auto dir = root->openDirSection(1, true);
    auto data1 = dir->openDataSection<R1>(1, true, 4, 0);
    auto data2 = dir->openDataSection<R2>(2, true, 12, 0);
    auto data3 = dir->openDataSection<R3>(3, true, 0, 0);
    for (int i = 0; i < 10; ++i)
    {
        R1 r1{10 * (i * 3 + 1), i * 3 + 1};
        data1->write(&r1);
        R2 r2{(i * 3 + 2) * 100, (i * 3 + 2) * 100, (i * 3 + 2) * 100, i * 3 + 2};
        data2->write(&r2);
        R3 r3{i * 3 + 3, (i * 3 + 3) * 1000};
        data3->write(&r3);
    }
}
void read_test()
{
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::READ);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    auto root = backend.openRootSection();
    auto dir = root->openDirSection(1, true);
    auto data1 = dir->openDataSection<R1>(1, true, 4, 0);
    auto data2 = dir->openDataSection<R2>(2, true, 12, 0);
    auto data3 = dir->openDataSection<R3>(3, true, 0, 0);
    std::vector<std::pair<ral::DataSectionInterface::Iterator, ral::DataSectionInterface::Iterator>> iters;
    iters.push_back({data1->begin(), data1->end()});
    iters.push_back({data2->begin(), data2->end()});
    iters.push_back({data3->begin(), data3->end()});
    ral::SectionExtractor extractor{std::move(iters)};
    while (!extractor.invalid())
    {
        auto iter = extractor.get();
        auto desc = iter.desc();
        if (desc == 1)
        {
            spdlog::info("r1.a: {}", static_cast<R1 *>(*iter)->a);
        }
        else if (desc == 2)
        {
            spdlog::info("r2.a: {}", static_cast<R2 *>(*iter)->a);
        }
        else if (desc == 3)
        {
            spdlog::info("r3.a: {}", static_cast<R3 *>(*iter)->a);
        }
        ++extractor;
    }
}
int main()
{
    write_test();
    read_test();
    spdlog::info("pass");
}
