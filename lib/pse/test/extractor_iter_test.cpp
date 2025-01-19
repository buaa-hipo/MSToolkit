#include <ral/extractor.h>
#include <ral/section.hpp>
#include <filesystem>

using namespace pse::ral;
struct R1
{
    int a;
    int b;
    uint64_t time;
};
struct R2
{
    int c;
    uint64_t time;
};
const int N = 1000;
void write_test()
{
    auto backend = Backend::open("test.db", Backend::BackendMode::SECTION_FILE, RWMode::WRITE);
    auto root = backend->openRootSection();
    auto data = backend->openDataSection<R1>(root, 1, true);
    auto data2 = backend->openDataSection<R2>(root, 2, true);
    for (int i = 0; i < N; ++i)
    {
        if (i % 3 == 0)
        {
            R1 r = {i, i + 1, (uint64_t)i};
            data.writeRecord(&r);
        }
        else
        {
            R2 r = {i * 2, (uint64_t)i};
            data2.writeRecord(&r);
        }
    }
}
void read_test()
{
    auto backend = Backend::open("test.db", Backend::BackendMode::SECTION_FILE, RWMode::READ);
    auto root = backend->openRootSection();
    auto data = backend->openDataSection<R1>(root, 1, false);
    auto data2 = backend->openDataSection<R2>(root, 2, false);
    auto iter = data.any_begin();
    auto iter2 = data2.any_begin();
    const auto &r1 = iter.get<R1>();
    const auto &r2 = iter2.get<R2>();
    spdlog::info("r1.a={}, r1.b={}, r1.time={}", r1.a, r1.b, r1.time);
    spdlog::info("r2.c={}, r2.time={}", r2.c, r2.time);
    spdlog::info("1.time: {}", iter.time());
    spdlog::info("2.time: {}", iter2.time());
    std::vector<std::pair<SectionIterator, SectionIterator>> iters;
    iters.emplace_back(data.any_range());
    iters.emplace_back(data2.any_range());
    auto any_iter = Extractor::Iterator(std::move(iters));
    for (int i = 0; i < N; ++i)
    {
        if (i % 3 == 0)
        {
            assert(any_iter.type() == std::type_index(typeid(R1)));
            const auto &r1 = any_iter.get<R1>();
            assert(r1.a == i);
            assert(r1.b == i + 1);
            assert(r1.time == i);
        }
        else
        {
            assert(any_iter.type() == std::type_index(typeid(R2)));
            const auto &r2 = any_iter.get<R2>();
            assert(r2.c == i * 2);
            assert(r2.time == i);
        }
        ++any_iter;
    }
}
int main()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    write_test();
    read_test();
    spdlog::info("pass");
}