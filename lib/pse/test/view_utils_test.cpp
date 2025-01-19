#include <dsr/data_section_view.h>
#include <filesystem>
#include <dsr/view_utils.h>
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
        R1 r = {i, i + 1};
        data2.writeRecord(&r);
    }
}
// void custom_join_test()
// {
//     auto backend = Backend::open("test.db", Backend::BackendMode::SECTION_FILE, RWMode::READ);
//     auto root = backend->openRootSection();
//     auto data = backend->openDataSection<R1>(root, 1, false);
//     pse::dsr::DataSectionRange<R1> range1(&data);
//     pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator> view1(range1.begin(), range1.end());
//     auto transformed_view1 =
//         view1 | ranges::views::transform([](const R1 &r) { return *(R2 *)&r; }) | ranges::to_vector;

//     int i = 0; // 0 is reserve for left side
//     // test left join
//     auto data2 = backend->openDataSection<R1>(root, 2, false);
//     pse::dsr::DataSectionRange<R1> range2(&data2);
//     pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator> view2(range2.begin(), range2.end());
//     // be careful, ranges::move != ranges::views::move
//     auto transformed_view2 = ranges::views::move(view2) | ranges::to_vector;
//     auto left_join_view = pse::dsr::left_join(
//         transformed_view1, transformed_view2, [](const R2 &r1, const R1 &r2) { return r1.r1.a == r2.b; });
//     for (auto [r1, r2] : left_join_view)
//     {
//         if (r2)
//         {
//             assert(r1.r1.a == i);
//             assert((*r2).b == i);
//         }
//         else
//         {
//             assert(r1.r1.a == i);
//         }
//         // spdlog::info("r1: {}, r2: {}", r1.r1.a, r2 ? (*r2).b : -1);
//         i++;
//     }

//     auto inner_join_view = pse::dsr::inner_join(
//         transformed_view1, transformed_view2, [](const R2 &r1, const R1 &r2) { return r1.r1.a == r2.b; });
//     i = 1; // 0 is discarded
//     for (auto [r1, r2] : inner_join_view)
//     {
//         assert(r1.r1.a == i);
//         assert(r2.b == i);
//         // spdlog::info("r1: {}, r2: {}", r1.r1.a, r2.b);
//         i++;
//     }

//     auto right_join_view = pse::dsr::right_join(
//         transformed_view1, transformed_view2, [](const R2 &r1, const R1 &r2) { return r1.r1.a == r2.b; });
//     i = 1; // 0 is discarded
//     for (auto [r1, r2] : right_join_view)
//     {
//         if (r1)
//         {
//             assert((*r1).r1.a == i);
//             assert(r2.b == i);
//         }
//         else
//         {
//             assert(r2.b == i);
//         }
//         // spdlog::info("r1: {}, r2: {}", r1 ? (*r1).r1.a : -1, r2.b);
//         i++;
//     }

//     auto full_join_view = pse::dsr::full_join(
//         transformed_view1, transformed_view2, [](const R2 &r1, const R1 &r2) { return r1.r1.a == r2.b; });
//     i = 1; // 0 is discarded
//     for (auto [r1, r2] : full_join_view)
//     {
//         if (r1 && r2)
//         {
//             assert((*r1).r1.a == i);
//             assert((*r2).b == i);
//         }
//         // the order cannot be ganranteed, maybe we can sort them in some way
//         // else if (r1)
//         // {
//         //     assert((*r1).r1.a == i);
//         // }
//         // else
//         // {
//         //     assert((*r2).b == i);
//         // }
//         spdlog::info("r1: {}, r2: {}", r1 ? (*r1).r1.a : -1, r2 ? (*r2).b : -1);
//         i++;
//     }
// }

void sort_filter_test()
{
    auto view1 = ranges::views::iota(0, N); // 0, 1, 2, ..., N-1
    // shuffle the data
    std::vector<int> data(view1.begin(), view1.end());
    std::shuffle(data.begin(), data.end(), std::mt19937{std::random_device{}()});
    // sort the data
    auto sorted_view1 = pse::dsr::sort_filter(data, [](int x) { return x % 2 == 0; });
    // check if the data is sorted in ascending order
    int pre = -1;
    for (const auto &r : sorted_view1)
    {
        assert(pre <= r);
        pre = r;
    }
}

// void group_aggregate_test()
// {
//     auto view1 = ranges::views::iota(0, N); // 0, 1, 2, ..., N-1
//     int acc = 0;
//     // Note that the chunk_by only take the adjacent elements each time.
//     // Use the nums which are multiples of 10 as the critical point.
//     auto gg = pse::dsr::group_aggregate(
//         view1,
//         [](int x1, int x2) {
//             spdlog::info("x1:{}, x2:{}", x1, x2);
//             return (x1 % 10 >= 0 && x2 % 10 > 0);
//         },
//         [](int acc, int x) {
//             acc += x;
//             return acc;
//         },
//         acc);
//     int i = 0;
//     int manual_acc = 0;
//     for (const auto &r : gg)
//     {
//         int count = 0;
//         while (count < 10)
//         {
//             manual_acc += i;
//             i++;
//             count++;
//         }
//         assert(r == manual_acc);
//         manual_acc = 0;
//     }
// }

void element_to_group_test()
{
    auto view1 = ranges::views::iota(0, N); // 0, 1, 2, ..., N-1
    auto e2g =
        pse::dsr::element_to_group(view1, [](int key, int value) { return (value - key) >= 0 && (value - key) < 10; });
    for (const auto &[key, values] : e2g)
    {
        for (const auto &value : values)
        {
            assert(value - key >= 0 && value - key < 10);
        }
    }
}

} // namespace

int main()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    write_test();
    // custom_join_test();
    sort_filter_test();
    // group_aggregate_test();
    element_to_group_test();
}