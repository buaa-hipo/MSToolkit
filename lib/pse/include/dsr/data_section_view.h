#pragma once
#include <ral/section.hpp>

namespace pse
{
namespace dsr
{

template <typename T>
class DataSectionRange
{
private:
    pse::ral::DataSection<T> *_data_section;

public:
    DataSectionRange(pse::ral::DataSection<T> *data)
    : _data_section(data)
    {
    }

    DataSectionRange()
    : _data_section(nullptr)
    {
    }

    ~DataSectionRange()
    {
        // if (_data_section)
        // {
        //     _data_section->close();
        // }
    }

    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T *;
        using reference = const T &;

        Iterator()
        : _si(nullptr)
        {
        }
        Iterator(std::shared_ptr<pse::ral::SectionIterator> si)
        : _si(si)
        {
        }

        reference operator*() const { return _si->get<T>(); }
        pointer operator->() { return &_si->get<T>(); }

        Iterator &operator++()
        {
            ++(*_si);
            return *this;
        }

        Iterator &operator++(int)
        {
            ++(*_si);
            return *this;
        }

        friend bool operator==(const Iterator &a, const Iterator &b) { return *(a._si) == *(b._si); }
        friend bool operator!=(const Iterator &a, const Iterator &b) { return *(a._si) != *(b._si); }

    private:
        std::shared_ptr<pse::ral::SectionIterator> _si;
    };

    Iterator begin() const
    {
        auto iter = std::make_shared<pse::ral::SectionIterator>(_data_section->any_begin());
        return Iterator(iter);
    }
    Iterator end() const
    {
        auto iter = std::make_shared<pse::ral::SectionIterator>(_data_section->any_end());
        return Iterator(iter);
    }
};

template <typename Iterator>
class RangeView
{
public:
    using iterator = Iterator;

    RangeView(iterator begin, iterator end)
    : _begin(begin)
    , _end(end)
    {
    }

    iterator begin() const { return _begin; }
    iterator end() const { return _end; }

private:
    iterator _begin;
    iterator _end;
};

} // namespace dsr

} // namespace pse