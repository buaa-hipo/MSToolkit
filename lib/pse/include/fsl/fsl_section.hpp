#pragma once
#include <fsl/fsl_block.h>
#include <ral/time_detector.h>
#include <ral/section_mixin.h>
#include <utils/compile_time.h>
#include <memory>
#include <fsl/fsl_lib.h>

namespace pse
{
namespace fsl
{

class BlockManager;
class FSL_Backend;
template <typename record_t>
class DataSectionImpl : public ral::DataSectionImplMixin<record_t, DataSectionImpl<record_t>>
{
    using self_t = DataSectionImpl<record_t>;
    friend ral::DataSectionImplMixin<record_t, DataSectionImpl<record_t>>;
    friend FSL_Backend;
    DataSectionDescBlock *sec;
    std::shared_ptr<FSL_Backend> _manager;

    auto &backend() { return _manager; }

public:
    DataSectionImpl(DataSectionDescBlock *block, std::shared_ptr<FSL_Backend> &&manager)
    : sec(block)
    , _manager(manager)
    {
    }

    bool valid() { return sec != nullptr; }

private:
    /**
     * @brief write record to data section
     * 
     * @param i write to i-th record
     * @param record 
     */
    void _writeRecord(size_t i, const record_t *record);
    /**
     * @brief read record from data section
     * 
     * @param i read from i-th record
     * @param record 
     */
    void _readRecord(size_t i, record_t *record);

public:
    class Iterator : public ral::SectionIteratorBase<record_t>
    {
        friend self_t;
        self_t *_sec;
        size_t _id;
        // mutable record_t _record;
        mutable std::unique_ptr<char[]> _recordBuffer;
        mutable bool _recordValid = false;

    public:
        Iterator(self_t *sec, size_t i)
        : _sec(sec)
        , _id(i)
        , _recordBuffer(new char[sizeof(record_t) + sec->sec->recordVariableLength])
        {
        }

        Iterator(Iterator &&) = default;

    public:
        virtual const record_t &operator*() const override
        {
            if (!_recordValid)
            {
                _sec->_readRecord(_id, (record_t *)_recordBuffer.get());
                _recordValid = true;
            }
            return *(const record_t *)_recordBuffer.get();
        }

        virtual Iterator &operator++() override
        {
            _recordValid = false;
            _id++;
            return *this;
        }

        bool operator==(const Iterator &other) const { return _id == other._id; }
        virtual bool operator==(const ral::DuckTypeSectionIteratorBase &other) const override
        {
            return _id == static_cast<const Iterator &>(other)._id;
        }
        virtual bool operator!=(const ral::DuckTypeSectionIteratorBase &other) const override
        {
            return !(*this == other);
        }
        virtual ~Iterator() = default;
        virtual uint64_t time() const override
        {
            this->operator*();
            static_assert(ral::TimeAccessor<record_t>::has_field(), "record_t must have time field");
            return ral::TimeAccessor<record_t>::get_field(*(record_t *)_recordBuffer.get());
            // return ((record_t *)_recordBuffer.get())->time;
        }
    };

    std::unique_ptr<Iterator> begin() { return std::make_unique<Iterator>(this, 0); }
    std::unique_ptr<Iterator> end()
    {
        return std::make_unique<Iterator>(this, sec->sectionSize / (sizeof(record_t) + sec->recordVariableLength));
    }
};
class DirSectionImpl : public ral::DirSectionImplMixin<DirSectionImpl, DataSectionImpl>
{
    friend FSL_Backend;
    friend ral::DirSectionImplMixin<DirSectionImpl, DataSectionImpl>;
    DirSectionDescBlock *sec;
    std::shared_ptr<FSL_Backend> _manager;
    using backend_t = FSL_Backend;

    auto &backend() { return _manager; }

public:
    DirSectionImpl(DirSectionDescBlock *block, std::shared_ptr<FSL_Backend> &&manager)
    : sec(block)
    , _manager(manager)
    {
    }
    bool valid() { return sec != nullptr; }

    // std::unique_ptr<DirSectionImpl> openDir(Block::desc_t desc, bool create);
    // template <typename record_t>
    // std::unique_ptr<DataSectionImpl<record_t>> openData(Block::desc_t desc, bool create);
};

class StringSectionImpl
{
    friend FSL_Backend;
    DataSectionDescBlock *sec;
    std::shared_ptr<FSL_Backend> _manager;
    using backend_t = FSL_Backend;

    auto &backend() { return _manager; }
    void writeString(std::string_view str);
    void readString(std::string &str, size_t offset);
};
class FSL_Backend : public std::enable_shared_from_this<FSL_Backend>
{
    BlockManager _manager;

public:
    FSL_Backend(std::string_view name, ral::RWMode mode)
    : _manager(name, mode)
    {
    }
    template <typename record_t>
    using DataSectionImpl_t = DataSectionImpl<record_t>;
    using DirSectionImpl_t = DirSectionImpl;

    template <typename record_t>
    std::unique_ptr<DataSectionImpl<record_t>>
    openDataSec(DirSectionImpl *dir, Block::desc_t desc, bool create, int recordVariableLength = 0)
    {
        auto subsec = _manager.openDataSection(dir->sec, desc, create, false, recordVariableLength);
        if (!subsec)
        {
            return {};
        }
        return std::make_unique<DataSectionImpl<record_t>>(subsec, std::move(shared_from_this()));
    }

    std::unique_ptr<DirSectionImpl> openDirSec(DirSectionImpl *dir, Block::desc_t desc, bool create)
    {
        auto subdir = _manager.openDirSection(dir->sec, desc, create);
        if (!subdir)
        {
            return {};
        }
        return std::make_unique<DirSectionImpl>(subdir, std::move(shared_from_this()));
    }
    std::unique_ptr<DirSectionImpl> openRootSec()
    {
        return std::make_unique<DirSectionImpl>(_manager.openRootSection(), std::move(shared_from_this()));
    }
    static std::shared_ptr<FSL_Backend> create(std::string_view name, ral::RWMode mode)
    {
        return std::make_shared<FSL_Backend>(name, mode);
    }
    void writeDataSection(DataSectionDescBlock *sec, size_t offset, const void *buf, size_t len)
    {
        _manager.writeDataSection(sec, offset, buf, len);
    }
    void writeDataSectionAtEnd(DataSectionDescBlock *sec, const void *buf, size_t len)
    {
        _manager.writeDataSectionAtEnd(sec, buf, len);
    }
    size_t allocateAtDataSection(DataSectionDescBlock *sec, size_t len)
    {
        return _manager.allocateAtDataSection(sec, len);
    }
    size_t readDataSection(DataSectionDescBlock *sec, size_t offset, void *buf, size_t len)
    {
        return _manager.readDataSection(sec, offset, buf, len);
    }
};

template <typename record_t>
inline void DataSectionImpl<record_t>::_writeRecord(size_t offset, const record_t *record)
{
    _manager->writeDataSection(sec,
                               offset * (sizeof(record_t) + sec->recordVariableLength),
                               record,
                               sizeof(record_t) + sec->recordVariableLength);
}

template <typename record_t>
inline void DataSectionImpl<record_t>::_readRecord(size_t offset, record_t *record)
{
    _manager->readDataSection(sec,
                              offset * (sizeof(record_t) + sec->recordVariableLength),
                              record,
                              sizeof(record_t) + sec->recordVariableLength);
}

// std::unique_ptr<DirSectionImpl> pse::fsl::DirSectionImpl::openDir(Block::desc_t desc, bool create)
// {
//     return _manager->openDirSec(this, desc, create);
// }

// template <typename record_t>
// inline std::unique_ptr<DataSectionImpl<record_t>> pse::fsl::DirSectionImpl::openData(Block::desc_t desc, bool create)
// {
//     return _manager->openDataSec<record_t>(this, desc, create);
// }

inline void StringSectionImpl::writeString(std::string_view str)
{
    auto offset = _manager->allocateAtDataSection(sec, 4 + str.size());
    int len = str.size();
    _manager->writeDataSection(sec, offset, &len, 4);
    _manager->writeDataSection(sec, offset + 4, str.data(), str.size());
}
inline void StringSectionImpl::readString(std::string &str, size_t offset)
{
    int len;
    _manager->readDataSection(sec, offset, &len, 4);
    str.resize(len);
    _manager->readDataSection(sec, offset + 4, str.data(), len);
}
} // namespace fsl

} // namespace pse
