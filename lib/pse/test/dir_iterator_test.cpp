#include <ral/backend.h>
#include <ral/section.h>
#include <fsl/raw_backend.h>
#include <filesystem>
using namespace pse;
struct R1
{
    int a;
};

void write_test()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::WRITE);
    auto _backend = ral::BackendWrapper(std::move(raw_backend));
    ral::BackendInterface &backend = static_cast<pse::ral::BackendInterface &>(_backend);
    auto root = backend.openRootSection();
    for (int i = 100; i < 5000; ++i)
    {
        auto dir = root->openDirSection(i, true);
        auto data = dir->openDataSection<R1>(1, true, 0, 0);
        R1 r1{i};
        data->write(&r1);
    }
}

void read_test()
{
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::WRITE);
    auto _backend = ral::BackendWrapper(std::move(raw_backend));
    ral::BackendInterface &backend = static_cast<pse::ral::BackendInterface &>(_backend);
    auto root = backend.openRootSection();
    int i = 100;
    for (auto iter = root->begin(); iter != root->end(); ++iter)
    {
        spdlog::info("iter desc: {}", iter.getDesc());
        auto dir = iter.getDirSection();
        auto data = dir->openDataSection<R1>(1, false, 0, 0);
        R1 r1;
        data->read(&r1, 0);
        if (r1.a != i || i != iter.getDesc())
        {
            spdlog::error("failed at cmp {}", i);
            exit(-1);
        }
        i++;
    }
}
int main()
{
    write_test();
    read_test();
}