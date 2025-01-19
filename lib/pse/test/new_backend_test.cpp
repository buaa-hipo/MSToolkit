#include <ral/backend.h>
#include <fsl/raw_backend.h>
#include <filesystem>
#include <ral/time_detector.h>
#include <string_view>
using namespace std::literals;
using namespace pse;
#pragma pack(1)
struct R1
{
    int a;
    int b;
    int64_t time;
};

#pragma pack(1)
struct R2
{
    R1 r1;
    int pmu[2];
};
struct metric_t
{
    uint64_t enter, exit;
};
struct record_t
{
    int16_t MsgType;
#ifdef ENABLE_BACKTRACE
    uint64_t ctxt;
#endif
    metric_t timestamps;
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
    auto data = dir->openDataSection<R1>(1, true, 0, 8);
    for (int i = 0; i < 1000; ++i)
    {
        R2 r2{{i, i + 1, i + 2}, i + 3, i + 4};
        data->write(&r2);
    }
}
void read_test()
{
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::READ);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    auto root = backend.openRootSection();
    auto dir = root->openDirSection(1, true);
    auto data = dir->openDataSection<R1>(1, true, 0, 0);
    spdlog::info("size={}", data->size());
    int i = 0;
    for (auto iter = data->begin(); iter != data->end(); ++iter)
    {
        R2 &r2 = *static_cast<R2 *>(*iter);
        assert(r2.r1.a == i);
        assert(r2.r1.b == i + 1);
        assert(r2.r1.time == i + 2);
        assert(r2.pmu[0] == i + 3);
        assert(r2.pmu[1] == i + 4);
        i++;
    }
}
std::vector<size_t> string_offsets;
void write_test2()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::WRITE);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    auto root = backend.openRootSection();
    auto dir = root->openDirSection(1, true);
    auto data = dir->openStringSection(2, true);
    char hello[7]{"hello"};
    for (int i = 0; i < 1000; ++i)
    {
        hello[5] = '0' + i % 10;
        auto res = data->write(hello);
        string_offsets.push_back(res);
    }
}
void read_test2()
{
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::WRITE);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    auto root = backend.openRootSection();
    auto dir = root->openDirSection(1, true);
    auto data = dir->openStringSection(2, true);
    char hello[7]{"hello"};
    for (int i = 0; i < 1000; ++i)
    {
        hello[5] = '0' + i % 10;
        char buffer[1024] = {0};
        auto res = data->read(buffer, string_offsets[i], 3);
        if (res != 6)
        {
            spdlog::error("res={}", res);
        }
        res = data->read(buffer, string_offsets[i], 1024);
        buffer[res] = '\0';
        if (strcmp(buffer, hello) != 0)
        {
            spdlog::error("buffer={}", buffer);
        }
    }
}
void write_test3()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::WRITE);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    auto root = backend.openRootSection();
    auto dir = root->openDirSection(1, true);
    auto data = dir->openStreamSection(2, true);
    char hello[7]{"hello"};
    for (int i = 0; i < 1000; ++i)
    {
        hello[5] = '0' + i % 10;
        auto res = data->write(hello, 6);
        string_offsets.push_back(res);
    }
}
void read_test3()
{
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::WRITE);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    auto root = backend.openRootSection();
    auto dir = root->openDirSection(1, true);
    auto data = dir->openStreamSection(2, true);
    char hello[7]{"hello"};
    for (int i = 0; i < 1000; ++i)
    {
        hello[5] = '0' + i % 10;
        char buffer[1024] = {0};
        auto res = data->read(buffer, 6);
        buffer[res] = '\0';
        if (strcmp(buffer, hello) != 0)
        {
            spdlog::error("buffer={}", buffer);
        }
    }
}
int main()
{
    write_test();
    read_test();
    write_test2();
    read_test2();
    write_test3();
    read_test3();
    spdlog::info("pass");

    record_t r;
    r.timestamps.enter = 1;
    PSE_GETFIELD(r, "timestamps.exit"sv) = 2;
    PSE_GETFIELD(r, std::string_view("timestamps.exit")) = 2;
    PSE_GETFIELD_V(r, std::string_view("timestamps.exit"));
    spdlog::info("r.time: {}", r.timestamps.enter);
}
