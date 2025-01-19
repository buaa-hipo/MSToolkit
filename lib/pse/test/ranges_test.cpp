#include <dsr/data_section_view.h>
#include <filesystem>
#include <range/v3/all.hpp>
#include <algorithm>
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
struct R1_transform
{
    int a;

    // implement the sort function for R1_transform
    friend bool operator<=(const R1_transform &a, const R1_transform &b) { return a.a <= b.a; }

    friend bool operator<(const R1_transform &a, const R1_transform &b) { return a.a < b.a; }
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

    // create random data
    auto data2 = backend->openDataSection<R1>(root, 2, true);
    for (int i = 0; i < N; ++i)
    {
        R1 r = {i, std::rand() % N};
        data2.writeRecord(&r);
    }
}
// void ranges_test()
// {
//     auto backend = Backend::open("test.db", Backend::BackendMode::SECTION_FILE, RWMode::READ);
//     auto root = backend->openRootSection();
//     auto data = backend->openDataSection<R1>(root, 1, false);
//     int i = 0;
//     // Check if the iterator satisfies the input_iterator concept
//     static_assert(std::input_iterator<pse::dsr::DataSectionRange<R1>::Iterator>);
//     // Check if the range satisfies the range concept
//     static_assert(ranges::range<pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator>>);
//     // The range does not satisfy the forward_range concept since we make the iterator satisfy the std::weakly_incrementable concept
//     static_assert(!ranges::forward_range<pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator>>);

//     pse::dsr::DataSectionRange<R1> range(&data);
//     pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator> view(range.begin(), range.end());
//     for (const auto &r : view | ranges::views::take(10) | ranges::views::transform([](const R1 &r) {
//                              R1_transform r1;
//                              r1.a = r.a;
//                              return r1;
//                          }) | ranges::views::filter([](const R1_transform &r1) { return r1.a % 2 == 0; }))
//     {
//         spdlog::info("r1_t: {}", r.a);
//         assert(r.a % 2 == 0 && r.a == i);
//         i += 2;
//     }

//     // noted that we do not reset i to 0, since the view is single-pass.
//     // cast view to R2
//     for (const auto &r : view | ranges::views::transform([](const R1 &r) { return *(R2 *)&r; }) |
//                              ranges::views::filter([](const R2 &r) { return r.r1.a % 2 == 0; }))
//     {
//         spdlog::info("r1_t: {}", r.r1.a);
//         assert(r.r1.a % 2 == 0 && r.r1.a == i && r.pmus[0] == i + 2 && r.pmus[1] == i + 3);
//         i += 2;
//     }

//     // sort random data
//     auto data2 = backend->openDataSection<R1>(root, 2, false);
//     pse::dsr::DataSectionRange<R1> range2(&data2);
//     pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator> view2(range2.begin(), range2.end());
//     // It seems that ranges::views cannot work with ranges::to
//     // The transformed_view is a still a view, and the vector is temporary.
//     auto transformed_view = view2 | ranges::views::transform([](const R1 &r) {
//                                 R1_transform r1;
//                                 r1.a = r.b;
//                                 return r1;
//                             }) |
//                             ranges::to<std::vector<R1_transform>>();
//     // We need to copy the data to a vector to sort it.
//     std::vector<R1_transform> ints{transformed_view.begin(), transformed_view.end()};
//     // sort needs a comparison function
//     ranges::actions::sort(ints, std::less<R1_transform>{});
//     // // check if the data is sorted in ascending order
//     for (int i = 1; i < N; ++i)
//     {
//         assert(ints[i - 1] <= ints[i]); // operator <= will be used in here
//         spdlog::info("ints: {}", ints[i].a);
//     }

//     // test chunk_by
//     // chunk_by and transform == aggreate?
//     auto group_view =
//         ints | ranges::views::chunk_by([](const R1_transform &a, const R1_transform &b) { return a.a == b.a; });
//     spdlog::info("group_view size: {}", ranges::distance(group_view));
//     auto sum_view = group_view | ranges::views::transform([](auto &&group) {
//                         return ranges::accumulate(group, 0, [](int acc, const R1_transform &r) { return acc + r.a; });
//                     });
//     assert(ranges::distance(sum_view) == ranges::distance(group_view));
//     for (const auto &sum : sum_view)
//     {
//         spdlog::info("Sum of group: {}", sum);
//     }
// }

} // namespace

int main()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    write_test();
    // ranges_test();
}