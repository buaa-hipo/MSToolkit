#include <ral/section.hpp>
#include <filesystem>
namespace
{

using namespace pse::ral;
#pragma pack(1)
struct R1
{
    int a;
    int b;
    uint64_t time;
};
#pragma pack(1)
struct R2
{
    R1 r1;
    int pmus[2];
};
const int N = 1000;
void write_test()
{
    auto backend = Backend::open("test.db", Backend::BackendMode::SECTION_FILE, RWMode::WRITE);
    auto root = backend->openRootSection();
    auto data = backend->openDataSection<R1>(root, 1, true, 8);
    for (int i = 0; i < N; ++i)
    {
        R2 r = {{i, i + 1}, {i + 2, i + 3}};
        data.writeRecord(&r.r1);
    }
}
void read_test()
{
    auto backend = Backend::open("test.db", Backend::BackendMode::SECTION_FILE, RWMode::READ);
    auto root = backend->openRootSection();
    auto data = backend->openDataSection<R1>(root, 1, false);
    auto iter = data.any_begin();
    int i = 0;

    while (iter != data.any_end())
    {
        assert(iter.type() == std::type_index(typeid(R1)));
        const R1 &r = iter.get<R1>();
        assert(r.a == i);
        assert(r.b == i + 1);
        const R2 *r2 = (const R2 *)&r;
        spdlog::info("r2: {}, {}", r2->pmus[0], r2->pmus[1]);
        assert(r2->pmus[0] == i + 2);
        assert(r2->pmus[1] == i + 3);
        // assert(r.a == i);
        // assert(r.b == i + 1);
        ++iter;
        ++i;
    }
}

// int main(int argc, char **argv)
// {
//     testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
} // namespace
// TEST_CASE("iterator_test", "[iterator]")
int main()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    write_test();
    read_test();
}