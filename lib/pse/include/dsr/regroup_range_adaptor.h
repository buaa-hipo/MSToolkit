#pragma once
#include <ranges> // Currently, regroup is more suitable with std::ranges.
// #include <range/v3/all.hpp>

namespace pse
{
namespace dsr
{

template <typename Range1, typename Range2, typename ValueType, typename Selector1, typename Selector2>
class RegroupIterator
{
public:
    using value_type = ValueType;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type;
    using It1 = decltype(std::declval<Range1>().begin());
    using It2 = decltype(std::declval<Range2>().begin());
    using End1 = decltype(std::declval<Range1>().end());
    using End2 = decltype(std::declval<Range2>().end());

    RegroupIterator(It1 it1, It2 it2, End1 end1, End2 end2, Selector1 selector1, Selector2 selector2)
    : m_it1(it1)
    , m_it2(it2)
    , m_end1(end1)
    , m_end2(end2)
    , m_selector1(selector1)
    , m_selector2(selector2)
    {
    }

    RegroupIterator(End1 end1, End2 end2, Selector1 selector1, Selector2 selector2, bool is_end)
    : m_end1(end1)
    , m_end2(end2)
    , m_selector1(selector1)
    , m_selector2(selector2)
    , _is_end(is_end)
    {
    }

    RegroupIterator() = default;

    reference operator*() const { return value_type{m_selector1(*m_it1), m_selector2(*m_it2)}; }
    pointer operator->() const { return &(operator*()); }

    RegroupIterator &operator++()
    {
        ++m_it1;
        ++m_it2;
        if (m_it1 == m_end1 || m_it2 == m_end2)
        {
            _is_end = true;
        }
        return *this;
    }

    RegroupIterator operator++(int)
    {
        RegroupIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    friend bool operator==(const RegroupIterator &a, const RegroupIterator &b)
    {
        if (a._is_end && b._is_end)
        {
            return true;
        }
        // If one of the iterator is end, the m_it is non-initialized.
        // Currently, DataSectionRangeIterator cannot compare to non-initialized DataSectionRangeIterator.
        if (a._is_end || b._is_end)
        {
            return false;
        }
        return (a.m_it1 == b.m_it1) && (a.m_it2 == b.m_it2);
    }

    friend bool operator!=(const RegroupIterator &a, const RegroupIterator &b) { return !(a == b); }

private:
    It1 m_it1;
    It2 m_it2;
    End1 m_end1;
    End2 m_end2;
    Selector1 m_selector1;
    Selector2 m_selector2;
    bool _is_end = false;
};

template <typename Range1, typename Range2, typename ValueType, typename Selector1, typename Selector2>
class RegroupView
{
public:
    using iterator = RegroupIterator<Range1, Range2, ValueType, Selector1, Selector2>;

    RegroupView(Range1 range1, Range2 range2, Selector1 selector1, Selector2 selector2)
    : m_range1(range1)
    , m_range2(range2)
    , m_selector1(selector1)
    , m_selector2(selector2)
    {
    }

    iterator begin()
    {
        return iterator(m_range1.begin(), m_range2.begin(), m_range1.end(), m_range2.end(), m_selector1, m_selector2);
    }

    iterator end() { return iterator(m_range1.end(), m_range2.end(), m_selector1, m_selector2, 1); }

private:
    Range1 m_range1;
    Range2 m_range2;
    Selector1 m_selector1;
    Selector2 m_selector2;
};

template <typename Range2, typename ValueType, typename Selector1, typename Selector2>
struct regroup_with
{
    Range2 range2;
    Selector1 selector1;
    Selector2 selector2;
};

// std::views::take and similar views can sometimes require owning views of their underlying ranges to avoid dangling references.
// To make the regroup view compatible with std::views::take, we need to ensure that it can properly own or reference the underlying ranges.
// One way to address this is by using std::ranges::ref_view or std::ranges::owning_view to explicitly manage the lifetime of the ranges.
// This is done by wrapping the ranges in a std::ranges::ref_view or std::ranges::owning_view before passing them to the regroup view.
template <typename Range1, typename Range2, typename ValueType, typename Selector1, typename Selector2>
auto operator|(Range1 &&range1, regroup_with<Range2, ValueType, Selector1, Selector2> &&adaptor)
{
    using Range1RefView = std::ranges::ref_view<std::decay_t<Range1>>;
    using Range2RefView = std::ranges::ref_view<std::decay_t<Range2>>;
    return RegroupView<Range1RefView, Range2RefView, ValueType, Selector1, Selector2>(
        std::ranges::ref_view(range1), std::ranges::ref_view(adaptor.range2), adaptor.selector1, adaptor.selector2);
}

template <typename ValueType, typename Range2, typename Selector1, typename Selector2>
auto regroup(Range2 &&range2, Selector1 selector1, Selector2 selector2)
{
    return regroup_with<std::decay_t<Range2>, ValueType, Selector1, Selector2>{
        std::forward<Range2>(range2), selector1, selector2};
}

/**
 * @brief Function to accept two ranges, two lambda functions, and return the regrouped view.
 */
template <typename ValueType, typename Range1, typename Range2, typename Selector1, typename Selector2>
auto make_regroup(Range1 &&range1, Range2 &&range2, Selector1 selector1, Selector2 selector2)
{
    return std::forward<Range1>(range1) | regroup<ValueType>(std::forward<Range2>(range2), selector1, selector2);
}

} // namespace dsr

} // namespace pse