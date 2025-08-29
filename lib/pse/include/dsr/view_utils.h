#pragma once
#include <ral/section.hpp>
#include <range/v3/all.hpp>
#include <optional>

namespace pse
{
namespace dsr
{

/*
 * Brief: Custom inner join function (behave as the sql inner join) for ranges
 */
template <typename Range1, typename Range2, typename Condition>
auto inner_join(Range1 &&range1, Range2 &&range2, Condition &&condition)
{
    return ranges::views::transform(
               range1,
               [&](auto &&item1) {
                   return ranges::views::transform(
                       range2 | ranges::views::filter([&](auto &&item2) { return condition(item1, item2); }),
                       [&](auto &&item2) { return std::make_tuple(item1, item2); });
               }) |
           ranges::views::join;
}

/*
 * Brief: Custom left join function (behave as the sql left join) for ranges
 */
template <typename Range1, typename Range2, typename Condition>
auto left_join(Range1 &&range1, Range2 &&range2, Condition &&condition)
{
    using T2 = ranges::range_value_t<Range2>;

    return ranges::views::transform(
               range1,
               [&](auto &&item1) {
                   auto matches = range2 | ranges::views::filter([&](auto &&item2) { return condition(item1, item2); });
                   auto no_match_view = ranges::views::single(std::make_tuple(item1, (std::optional<T2>)std::nullopt));
                   auto match_view = ranges::views::transform(
                       matches, [&](auto &&item2) { return std::make_tuple(item1, std::make_optional(item2)); });
                   if (matches.begin() == matches.end())
                   {
                       return no_match_view | ranges::to_vector;
                   }
                   return match_view | ranges::to_vector;
               }) |
           ranges::views::join;
}

/*
 * Brief: Custom right join function (behave as the sql right join) for ranges
 */
template <typename Range1, typename Range2, typename Condition>
auto right_join(Range1 &&range1, Range2 &&range2, Condition &&condition)
{
    using T1 = ranges::range_value_t<Range1>;

    return ranges::views::transform(
               range2,
               [&](auto &&item2) {
                   auto matches = range1 | ranges::views::filter([&](auto &&item1) { return condition(item1, item2); });
                   auto no_match_view = ranges::views::single(std::make_tuple((std::optional<T1>)std::nullopt, item2));
                   auto match_view = ranges::views::transform(
                       matches, [&](auto &&item1) { return std::make_tuple(std::make_optional(item1), item2); });
                   if (matches.begin() == matches.end())
                   {
                       return no_match_view | ranges::to_vector;
                   }
                   return match_view | ranges::to_vector;
               }) |
           ranges::views::join;
}

/*
 * Brief: Custom full join function (behave as the sql full join) for ranges
 */
template <typename Rng1, typename Rng2, typename Pred>
auto full_join(Rng1 &&rng1, Rng2 &&rng2, Pred pred)
{
    using T1 = ranges::range_value_t<Rng1>;
    using T2 = ranges::range_value_t<Rng2>;
    using Result = std::tuple<std::optional<T1>, std::optional<T2>>;

    // Step 1: Generate Cartesian product and filter based on join condition
    auto joined = ranges::views::cartesian_product(rng1, rng2) |
                  ranges::views::filter([&](const auto &pair) { return pred(std::get<0>(pair), std::get<1>(pair)); }) |
                  ranges::views::transform([](const auto &pair) -> Result {
                      return {std::make_optional(std::get<0>(pair)), std::make_optional(std::get<1>(pair))};
                  });

    // Step 2: Add unmatched elements from rng1
    auto unmatched1 = rng1 | ranges::views::filter([&](const T1 &v1) {
                          return ranges::none_of(rng2, [&](const T2 &v2) { return pred(v1, v2); });
                      }) |
                      ranges::views::transform([](const T1 &v1) -> Result {
                          return {std::make_optional(v1), std::nullopt};
                      });

    // Step 3: Add unmatched elements from rng2
    auto unmatched2 = rng2 | ranges::views::filter([&](const T2 &v2) {
                          return ranges::none_of(rng1, [&](const T1 &v1) { return pred(v1, v2); });
                      }) |
                      ranges::views::transform([](const T2 &v2) -> Result {
                          return {std::nullopt, std::make_optional(v2)};
                      });

    // Combine all parts together
    return ranges::views::concat(joined, unmatched1, unmatched2);
}

/*
 * Brief: Given a range, a condition and an optional comparator, 
 * return a sorted view that filters the range based on the condition.
 * Do not attempting to combine views and actions in a single pipeline, 
 * which is not directly possible because views and actions serve different purposes.
 */
template <typename Range, typename Condition, typename Comparator = std::less<>>
auto sort_filter(Range &&range, Condition &&condition, Comparator &&comp = Comparator{})
{
    auto filtered_result = range | ranges::views::filter(std::forward<Condition>(condition)) | ranges::to_vector;
    std::ranges::sort(filtered_result, std::forward<Comparator>(comp));
    return filtered_result;
}

/*
 * Brief: Given a range, a condition and an aggregator, 
 * return a view that groups the range based on the condition and aggregates the groups
 */
template <typename Range, typename Condition, typename Aggregator, typename InitValue>
auto group_aggregate(Range &&range, Condition &&condition, Aggregator &&aggregator, InitValue &&init_value)
{
    // auto group_view = range | ranges::views::chunk_by(std::forward<Condition>(condition));
    // return group_view | ranges::views::transform([&aggregator, &init_value](auto &&group) {
    //            return ranges::accumulate(group, init_value, std::forward<Aggregator>(aggregator));
    //        });
}

/*
 * Brief: Given a range and a condition, 
 * for each element as key, other elements satisfying the condition with the key are grouped as values,
 * return a view that form a map from key to values
 */
template <typename Range, typename Condition>
auto element_to_group(Range &&range, Condition &&condition)
{
    return range | ranges::views::transform([&](auto &&key) {
               return std::make_pair(key, range | ranges::views::filter([&](auto &&value) {
                                              return condition(key, value);
                                          }) | ranges::to<std::vector>());
           }) |
           ranges::to<std::unordered_map>();
}

} // namespace dsr

} // namespace pse