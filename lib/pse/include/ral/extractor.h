#pragma once
#include <ral/section.h>
#include <vector>
#include <queue>

namespace pse
{
namespace ral
{
class Extractor
{
public:
    class Iterator
    {
        struct less
        {
            bool operator()(const std::pair<SectionIterator, SectionIterator> &a,
                            const std::pair<SectionIterator, SectionIterator> &b) const
            {
                return a.first.time() > b.first.time();
            }
        };
        std::priority_queue<std::pair<SectionIterator, SectionIterator>,
                            std::vector<std::pair<SectionIterator, SectionIterator>>,
                            less>
            _queue;

    public:
        Iterator(std::vector<std::pair<SectionIterator, SectionIterator>> &&iters)
        {
            for (auto &iter : iters)
            {
                _queue.emplace(std::move(iter));
            }
        }

        std::type_index type() const { return _queue.top().first.type(); }

        template <typename record_t>
        const record_t &get()
        {
            return _queue.top().first.get<record_t>();
        }
        Iterator &operator++()
        {
            auto top = std::move(const_cast<std::pair<SectionIterator, SectionIterator> &>(_queue.top()));
            _queue.pop();
            top.first.operator++();
            if (top.first != top.second)
            {
                _queue.emplace(std::move(top));
            }
            return *this;
        }
    };
};

class SectionExtractor
{
public:
    struct less
    {
        bool operator()(const std::pair<DataSectionInterface::Iterator, DataSectionInterface::Iterator> &a,
                        const std::pair<DataSectionInterface::Iterator, DataSectionInterface::Iterator> &b) const
        {
            return a.first.time() > b.first.time();
        }
    };
    std::priority_queue<std::pair<DataSectionInterface::Iterator, DataSectionInterface::Iterator>,
                        std::vector<std::pair<DataSectionInterface::Iterator, DataSectionInterface::Iterator>>,
                        less>
        _queue;

public:
    SectionExtractor(std::vector<std::pair<DataSectionInterface::Iterator, DataSectionInterface::Iterator>> &&iters)
    {
        for (auto &iter : iters)
        {
            _queue.emplace(std::move(iter));
        }
    }

    SectionExtractor(const SectionExtractor &) = default;

    void *operator*() { return *(_queue.top().first); }

    auto get() const { return _queue.top().first; }

    SectionExtractor &operator++()
    {
        auto top = std::move(
            const_cast<std::pair<DataSectionInterface::Iterator, DataSectionInterface::Iterator> &>(_queue.top()));
        _queue.pop();
        top.first.operator++();
        if (top.first != top.second)
        {
            _queue.emplace(std::move(top));
        }
        return *this;
    }

    SectionExtractor operator++(int)
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    bool invalid() const { return _queue.empty(); }
};

} // namespace ral
} // namespace pse
