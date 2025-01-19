#include <ral/backend.h>
#include <fsl/raw_backend.h>
#include <filesystem>
using namespace pse;
bool has_error = 0;
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
void write_test()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::WRITE);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    // ral::BackendInterface &backend = static_cast<pse::ral::BackendInterface &>(_backend);
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
    for (int i = 0; i < 1000; ++i)
    {
        R2 r2;
        R1 &r1 = r2.r1;
        data->read(&r1, i);
        if (r1.a != i)
        {
            spdlog::error("failed at test1: r1.a {} != {}", r1.a, i);
            has_error = 1;
        }
        if (r1.b != i+1)
        {
            spdlog::error("failed at test1: r1.b {} != {}", r1.b, i+1);
            has_error = 1;
        }
        if (r1.time != i+2)
        {
            spdlog::error("failed at test1: r1.time {} != {}", r1.time, i+2);
            has_error = 1;
        }
        if (r2.pmu[0] != i+3)
        {
            spdlog::error("failed at test1: r2.pmu[0] {} != {}", r2.pmu[0], i+3);
            has_error = 1;
        }
        if (r2.pmu[1] != i+4)
        {
            spdlog::error("failed at test1: r2.pmu[1] {} != {}", r2.pmu[1], i+4);
            has_error = 1;
        }
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
            has_error = 1;
        }
        res = data->read(buffer, string_offsets[i], 1024);
        buffer[res] = '\0';
        if (strcmp(buffer, hello) != 0)
        {
            spdlog::error("buffer={}", buffer);
            has_error = 1;
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
            has_error = 1;
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
    if (!has_error)
    spdlog::info("pass");
    else
    spdlog::error("failed");
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
}
