#pragma once
#include <ral/section.hpp>
#include <range/v3/all.hpp>
#include <vector> // Todo: currently using std::vector for storing results, can be replaced with other data structure.
#include <thread>
#include <future>
#include <algorithm> // for std::transform
#include <numeric>   // for std::reduce

namespace pse
{
namespace dsr
{

/* 
 * Brief: An interface that parallelly transforms the elements of a range using a function.
 */
template <typename ValueType, typename Range, typename Func>
auto parallel_transform(Range &&range, Func func)
{
    // using ValueType = std::decay_t<decltype(*std::begin(range))>;
    spdlog::info("ranges::distance(range): {}", ranges::distance(range));
    std::vector<ValueType> results(ranges::distance(range));

    auto first = ranges::begin(range);
    auto last = ranges::end(range);
    auto result_first = results.begin();

    // Determine the number of available hardware threads
    auto num_threads = std::thread::hardware_concurrency();
    // print num_threads
    spdlog::info("num_threads: {}", num_threads);
    auto part_size = ranges::distance(range) / num_threads;
    spdlog::info("part_size: {}", part_size);

    std::vector<std::future<void>> futures;
    for (unsigned i = 0; i < num_threads; ++i)
    {
        auto part_first = ranges::next(first, i * part_size);
        auto part_last = (i == num_threads - 1) ? last : ranges::next(part_first, part_size);
        auto result_part_first = result_first + i * part_size;

        futures.emplace_back(std::async(
            std::launch::async, [=, &func] { std::transform(part_first, part_last, result_part_first, func); }));
    }

    for (auto &future : futures)
    {
        future.get();
    }

    return results;
}

/* 
 * Brief: An interface that parallelly reduces the elements of a range.
 */
template <typename Range, typename T, typename AccuBinaryOp, typename BinaryOp>
T parallel_reduce(Range &&range, T init, AccuBinaryOp accu_binary_op, BinaryOp binary_op)
{
    using ValueType = std::decay_t<decltype(*std::begin(range))>;

    auto num_threads = std::thread::hardware_concurrency();
    auto part_size = std::distance(std::begin(range), std::end(range)) / num_threads;

    std::vector<std::future<T>> futures;
    auto first = std::begin(range);

    for (unsigned i = 0; i < num_threads; ++i)
    {
        auto part_first = std::next(first, i * part_size);
        auto part_last = (i == num_threads - 1) ? std::end(range) : std::next(part_first, part_size);

        futures.emplace_back(std::async(std::launch::async, [part_first, part_last, accu_binary_op] {
            return std::accumulate(part_first, part_last, T{}, accu_binary_op);
        }));
    }

    // Reduce the results of the partial reductions
    for (auto &future : futures)
    {
        init = binary_op(init, future.get());
    }

    return init;
}

/* 
 * Brief: An interface that parallelly sorts the elements of a range.
 * Todo: Uses std::execution::par with std::sort to perform sorting in parallel.
 * But <execution> library is not available in the current environment (tbb version maybe to old).
 */
template <typename Range, typename Compare>
auto parallel_sort(Range &&range, Compare comp)
{
    using ValueType = std::decay_t<decltype(*std::begin(range))>;
    std::vector<ValueType> results{range.begin(), range.end()};

    auto first = results.begin();
    auto last = results.end();

    auto num_threads = std::thread::hardware_concurrency();
    auto part_size = results.size() / num_threads;

    std::vector<std::future<void>> futures;
    for (unsigned i = 0; i < num_threads; ++i)
    {
        auto part_first = ranges::next(first, i * part_size);
        auto part_last = (i == num_threads - 1) ? last : ranges::next(part_first, part_size);

        futures.emplace_back(std::async(std::launch::async, [=, &comp] { std::sort(part_first, part_last, comp); }));
    }

    for (auto &future : futures)
    {
        future.get();
    }

    // Bottom-up merge sort to merge the sorted partitions
    for (std::size_t size = part_size; size < results.size(); size *= 2)
    {
        for (std::size_t left_start = 0; left_start < results.size() - size; left_start += 2 * size)
        {
            auto middle = ranges::next(results.begin(), left_start + size);
            auto right_end = ranges::next(results.begin(), std::min(left_start + 2 * size, results.size()));
            std::inplace_merge(ranges::next(results.begin(), left_start), middle, right_end, comp);
        }
    }

    return results;
}

} // namespace dsr

} // namespace pse