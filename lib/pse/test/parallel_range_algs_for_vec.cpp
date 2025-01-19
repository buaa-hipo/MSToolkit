#include <dsr/data_section_view.h>
#include <dsr/parallel_range_algs_for_vec.h>
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

    // implement the sort function for R1
    friend bool operator<=(const R1 &a, const R1 &b) { return a.b <= b.b; }
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
void ranges_test()
{
    auto backend = Backend::open("test.db", Backend::BackendMode::SECTION_FILE, RWMode::READ);
    auto root = backend->openRootSection();
    auto data = backend->openDataSection<R1>(root, 1, false);
    int i = 0;
    // Check if the iterator satisfies the input_iterator concept
    static_assert(std::input_iterator<pse::dsr::DataSectionRange<R1>::Iterator>);
    // Check if the range satisfies the range concept
    static_assert(std::ranges::range<pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator>>);

    pse::dsr::DataSectionRange<R1> range(&data);
    pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator> view(range.begin(), range.end());

    // For our range, we cannot perform multiple-pass traversal.
    // Checking distance
    // auto distance = std::ranges::distance(view);
    // spdlog::info("Distance: {}", distance);
    // Second distance check
    // distance = std::ranges::distance(view);
    // spdlog::info("Distance (second check): {}", distance); // equal to zero

    // view | ranges::to<std::vector<R1>>() results in a zero-length vector.
    // we can use ranges::views::take or ranges::views::transform (they return a new range) to transform the view to a vector correctly.
    // If we use ranges::views::move, we can transform the view to a N-length vector.
    // Attention: ranges::views::move cannot handle the pmus because it copy R1 to true R1.
    auto transformed_view =
        view | ranges::views::transform([](const R1 &r) { return *(R2 *)&r; }) | ranges::to<std::vector<R2>>();
    std::vector<R2> vec{transformed_view.begin(), transformed_view.end()};
    spdlog::info("{}", vec.size());
    auto transformed_vector = pse::dsr::parallel_transform<R2>(vec, [](R2 &r) {
        r.r1.a = 100;
        return r;
    });
    for (const auto &r : transformed_vector | ranges::views::take(10))
    {
        assert(r.r1.a == 100);
        assert(r.r1.b == i + 1);
        assert(r.pmus[0] == i + 2);
        assert(r.pmus[1] == i + 3);
        spdlog::info("{}", r.pmus[1]);
        i++;
    }

    // reduce sum of the transformed vector
    auto sum = pse::dsr::parallel_reduce(
        transformed_vector, 0, [](int init, R2 &r) { return init + r.r1.a; }, std::plus<int>());
    assert(sum == 100 * N);

    // sort random data parallelly using parallel_sort
    // ranges::views::move can be used in here cause no pmu.
    auto data2 = backend->openDataSection<R1>(root, 2, false);
    pse::dsr::DataSectionRange<R1> range2(&data2);
    pse::dsr::RangeView<pse::dsr::DataSectionRange<R1>::Iterator> view2(range2.begin(), range2.end());
    auto transformed_view2 = ranges::views::move(view2) | ranges::to<std::vector<R1>>();
    auto ints = pse::dsr::parallel_sort(transformed_view2, [](const R1 &a, const R1 &b) { return a.b < b.b; });
    // check if the data is sorted in ascending order
    for (int i = 1; i < N; ++i)
    {
        assert(ints[i - 1] <= ints[i]); // operator <= will be used in here
        // spdlog::info("{}", ints[i].b);
    }
}

} // namespace

int main()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    write_test();
    ranges_test();
}