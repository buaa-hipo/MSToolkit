#include <dsr/data_section_view.h>
#include <dsr/regroup_range_adaptor.h>
#include <filesystem>
#include <ranges>
// #include <range/v3/all.hpp>
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
#pragma pack(1)
struct R3
{
    R1 r1;
    R2 r2;
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

    auto data2 = backend->openDataSection<R1>(root, 2, true);
    for (int i = N; i < 2 * N; ++i)
    {
        R1 r = {i, i + 1};
        data2.writeRecord(&r);
    }

    auto data3 = backend->openDataSection<R1>(root, 3, true);
    for (int i = 2 * N; i < 3 * N; ++i)
    {
        R1 r = {i, i + 1};
        data3.writeRecord(&r);
    }
}
void regroup_test()
{
    auto backend = Backend::open("test.db", Backend::BackendMode::SECTION_FILE, RWMode::READ);
    auto root = backend->openRootSection();
    auto data = backend->openDataSection<R1>(root, 1, false);
    auto data2 = backend->openDataSection<R1>(root, 2, false);
    auto data3 = backend->openDataSection<R1>(root, 3, false);
    int i = 0;

    pse::dsr::DataSectionRange<R1> range(&data);
    pse::dsr::DataSectionRange<R1> range2(&data2);
    pse::dsr::DataSectionRange<R1> range3(&data3);
    pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator> view(range.begin(), range.end());
    pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator> view2(range2.begin(), range2.end());
    pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator> view3(range3.begin(), range3.end());

    /* Here we cannot pass view2 (or other view that already pass to the first regroup) to the second chained regroup operation 
     * because view2 is also a member of the first regroup,
     * otherwise the iterator of view2 will be incremented by both of the regroup and cause UB.
     * Besides, the begin and end (sentinel) iterator of view operation like std::views::take(10) are different.
     * !!!Todo: std::views::common is used to make the begin and end iterator of the view operation the same.
     * We handle this situation by adding both begin and end iterator to RegroupIterator.
     * When one of the iterator reach the end, the _is_end flag in RegroupIterator will be set to true.
     * The RegroupIterator equals to its end if the _is_end flag is true.
     * For passing (regroup | std::views::take(10)) to next chained regroup, see the operator| in regroup_range_adaptor.h.
     * Currently, regroup cannot pipe to ranges::views (or ohter range-v3 based views) due to some unknown concepts violations.
     */
    for (auto r : view2 | std::views::take(N / 2) |
                      pse::dsr::regroup<R3>(
                          view, [](const R1 &r1) { return r1; }, [](const R1 &r2) { return (*(R2 *)&r2); }) |
                      std::views::take(100) |
                      pse::dsr::regroup<std::tuple<R3, R1>>(
                          view3, [](const R3 &r3) { return r3; }, [](const R1 &r1) { return r1; }) |
                      std::views::transform([](const std::tuple<R3, R1> &r) {
                          return std::tuple{std::get<0>(r), std::get<1>(r)};
                      }))
    {
        const auto &[r3, r1] = r;
        assert(r3.r1.a == i + N);
        assert(r3.r1.b == i + 1 + N);
        assert(r3.r2.pmus[0] == i + 2);
        assert(r3.r2.pmus[1] == i + 3);
        assert(r1.a == i + 2 * N);
        assert(r1.b == i + 1 + 2 * N);
        spdlog::info("r: {}", r3.r1.a);
        ++i;
    }

    /*
     * The following code is valid.
     * But we cannot change the order of regroup and take.
     */
    // auto view_regroup = view2 | ranges::views::take(10) | pse::dsr::regroup<R3>(
    //                               view, [](const R1 &r1) { return r1; }, [](const R1 &r2) { return (*(R2 *)&r2); });
    // for (const auto &r : view_regroup) {
    //     assert(r.r1.a == i + N);
    //     assert(r.r1.b == i + 1 + N);
    //     assert(r.r2.pmus[0] == i + 2);
    //     assert(r.r2.pmus[1] == i + 3);
    //     spdlog::info("r: {}", r.r1.a);
    //     ++i;
    // }

    spdlog::info("----------------");
    auto view_multi_pass = view2 | std::views::take(3);
    for (const auto &r : view_multi_pass | std::views::take(2))
    {
        spdlog::info("r: {}", r.a);
    }
    spdlog::info("----------------");
    for (const auto &r : view_multi_pass)
    {
        spdlog::info("r: {}", r.a);
    }

    // We can find that view2 is not at the beginning of the range.
    // But the following for-loop cannot exit normally for unkown reason.
    // So it is recommended to not reuse the view after it is being traversed.
    // auto view4 = pse::dsr::make_regroup<R3>(view2, view, [](const R1 &r1) { return r1; }, [](const R1 &r2) { return (*(R2 *)&r2); });
    // for (const auto& r3 : view4) {
    //     spdlog::info("r3.r1.a: {} r3.r2.pmus: {}", r3.r1.a, r3.r2.pmus[0]);
    //     assert(r3.r1.a == i + N);
    //     assert(r3.r1.b == i + 1 + N);
    //     assert(r3.r2.pmus[0] == i + 2);
    //     assert(r3.r2.pmus[1] == i + 3);
    //     ++i;
    // }
}

} // namespace

int main()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    write_test();
    regroup_test();
}