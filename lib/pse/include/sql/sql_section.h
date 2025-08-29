#pragma once
#include <string>
#include <memory>
#include <exception>
#include <ral/section_declare.h>
#include <ral/section_mixin.h>
#include <sql/driver.h>

namespace pse
{
namespace sql
{

class SQLBackend;
template <typename record_t>
class DataSectionImpl : public ral::DataSectionImplMixin<record_t, DataSectionImpl<record_t>>
{
    using self_t = DataSectionImpl<record_t>;
    friend ral::DataSectionImplMixin<record_t, DataSectionImpl<record_t>>;
    std::shared_ptr<SQLBackend> _backend;
    SQLiteHandle handle;
    std::string tableName;

public:
    DataSectionImpl(std::string_view table,
                    std::shared_ptr<SQLBackend> &&backend,
                    bool create,
                    int recordVariableLength = 0);

private:
    // FIXME: add offset support
    // FIXME: add varibale length support
    void _writeRecord(size_t offset, const record_t *record) { handle.insert(tableName, *record); }
    // TODO: add readRecord implement
    // FIXME: add varibale length support
    void _readRecord(size_t offset, record_t *record) { handle.query(tableName, offset, *record); }

public:
    bool valid() { return handle.tableExists(tableName); }

    class Iterator : public ral::SectionIteratorBase<record_t>
    {
        friend self_t;
        self_t *_sec;
        size_t _id;
        mutable record_t _record;
        mutable bool _recordValid = false;

    public:
        Iterator(self_t *sec, size_t i)
        : _sec(sec)
        , _id(i)
        {
        }

    public:
        virtual const record_t &operator*() const override
        {
            if (!_recordValid)
            {
                _sec->_readRecord(_id, &_record);
                _recordValid = true;
            }
            return _record;
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
            // TODO: add time support
            throw std::runtime_error("Not implemented");
            return 0;
        }
    };

    std::unique_ptr<Iterator> begin() { return std::make_unique<Iterator>(this, 0); }
    // FIXME: fix end
    std::unique_ptr<Iterator> end() { throw std::runtime_error("Not implemented"); }
};
class DirSectionImpl : public ral::DirSectionImplMixin<DirSectionImpl, DataSectionImpl>
{
    std::string _name;
    using backend_t = SQLBackend;
    friend backend_t;
    friend ral::DirSectionImplMixin<DirSectionImpl, DataSectionImpl>;
    std::shared_ptr<backend_t> _backend;

    auto &backend() { return _backend; }

public:
    DirSectionImpl(std::string_view name, std::shared_ptr<backend_t> &&backend)
    : _name(name)
    , _backend(backend)
    {
    }

    bool valid() { return true; }
};

class StringSectionImpl
{
};

class SQLBackend : public std::enable_shared_from_this<SQLBackend>
{
    std::string _path;

public:
    template <typename record_t>
    using DataSectionImpl_t = DataSectionImpl<record_t>;
    using DirSectionImpl_t = DirSectionImpl;

    SQLBackend(std::string_view path)
    : _path(path)
    {
    }

    auto &path() { return _path; }

    template <typename record_t>
    std::unique_ptr<DataSectionImpl<record_t>>
    openDataSec(DirSectionImpl *dir, ral::desc_t desc, bool create, int recordVariableLength = 0)
    {
        return std::make_unique<DataSectionImpl<record_t>>(
            dir->_name + "_" + std::to_string(desc), std::move(shared_from_this()), create);
    }

    std::unique_ptr<DirSectionImpl> openDirSec(DirSectionImpl *dir, ral::desc_t desc, bool create)
    {
        return std::make_unique<DirSectionImpl>(dir->_name + "_" + std::to_string(desc), std::move(shared_from_this()));
    }
    std::unique_ptr<DirSectionImpl> openRootSec()
    {
        return std::make_unique<DirSectionImpl>("R", std::move(shared_from_this()));
    }

    static std::shared_ptr<SQLBackend> create(std::string_view name, ral::RWMode mode)
    {
        return std::make_shared<SQLBackend>(name);
    }
};

template <typename record_t>
inline pse::sql::DataSectionImpl<record_t>::DataSectionImpl(std::string_view table,
                                                            std::shared_ptr<SQLBackend> &&backend,
                                                            bool create,
                                                            int recordVariableLength)
: _backend(backend)
, handle(_backend->path())
, tableName(table)
{
    if (create)
    {
        handle.createTable<record_t>(tableName);
    }
}
} // namespace sql

} // namespace pse
