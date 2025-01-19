#pragma once
#include <string>
#include <memory>
#include <variant>
#include <any>
#include <typeindex>
#include <ral/desc_manager.h>
#include <fsl/fsl_section.hpp>
#include <sql/sql_section.h>
namespace pse
{
namespace ral
{
class Backend;

class SectionBase
{
public:
    enum SectionType
    {
        UNKNOWN = -1,
        DATA = 0,
        STRING,
        STREAM,
        DIR
    };

    SectionBase(SectionType type)
    : _type(type)
    {
    }

    const SectionType _type;
};

class DuckTypeSectionIteratorBase
{
public:
    virtual ~DuckTypeSectionIteratorBase() = default;
    virtual DuckTypeSectionIteratorBase &operator++() = 0;
    virtual std::type_index type() const = 0;
    virtual bool operator==(const DuckTypeSectionIteratorBase &other) const = 0;
    virtual bool operator!=(const DuckTypeSectionIteratorBase &other) const = 0;
    virtual uint64_t time() const = 0;
};
template <typename record_t>
class SectionIteratorBase : public DuckTypeSectionIteratorBase
{
public:
    virtual ~SectionIteratorBase() = default;
    virtual SectionIteratorBase<record_t> &operator++() = 0;
    virtual const record_t &operator*() const = 0;
    virtual std::type_index type() const override { return typeid(record_t); }
};

template <typename record_t>
class DataSection;

class SectionIterator
{
    template <typename record_t>
    friend class DataSection;

public:
    ~SectionIterator() = default;
    SectionIterator &operator++()
    {
        _impl->operator++();
        return *this;
    }

    template <typename record_t>
    const record_t &get() const
    {
        return static_cast<SectionIteratorBase<record_t> *>(_impl.get())->operator*();
    }

    std::type_index type() const { return _impl->type(); }

    bool operator==(const SectionIterator &other) const
    {
        return type() == other.type() && _impl->operator==(*other._impl.get());
    }

    bool operator!=(const SectionIterator &other) const { return !(*this == other); }

    SectionIterator(std::unique_ptr<DuckTypeSectionIteratorBase> &&impl)
    : _impl(std::move(impl))
    {
    }

    uint64_t time() const { return _impl->time(); }

    SectionIterator(SectionIterator &&) = default;
    SectionIterator &operator=(SectionIterator &&) = default;

    SectionIterator(const SectionIterator &) = delete;
    SectionIterator &operator=(const SectionIterator &) = delete;

private:
    std::unique_ptr<DuckTypeSectionIteratorBase> _impl;
};
// class AnyIterator
// {
//     template <typename record_t>
//     AnyIterator(SectionIteratorBase<record_t> &&iter)
//     : _iter(std::move(iter))
//     , _value([this]() { return std::any_cast<SectionIteratorBase<record_t> &>(_iter).operator*(); })
//     ,
//     , _increment([this]() { std::any_cast<SectionIteratorBase<record_t> &>(_iter).operator++(); })
//     {
//     }

//     std::any _iter;
//     std::function<std::any()> _value;
//     std::function<void()> _increment;
//     std::any operator*() { return _value(); }
//     AnyIterator &operator++()
//     {
//         _increment();
//         return *this;
//     }
// };
class DataSectionBase
{
public:
    virtual ~DataSectionBase() = default;
    virtual SectionIterator any_begin() const = 0;
    virtual SectionIterator any_end() const = 0;
};
template <typename record_t>
class DataSection : public DataSectionBase
{
    friend Backend;
    size_t offset = 0;
    std::shared_ptr<Backend> _backend;
    using section_t =
        std::variant<std::unique_ptr<fsl::DataSectionImpl<record_t>>, std::unique_ptr<sql::DataSectionImpl<record_t>>>;
    section_t _sec;
    DataSection(std::shared_ptr<Backend> backend, section_t &&sec)
    : _backend(backend)
    , _sec(std::move(sec))
    {
    }

    DataSection(const DataSection<record_t> &) = delete;
    DataSection<record_t> &operator=(const DataSection<record_t> &) = delete;

    void open(std::shared_ptr<Backend> backend, section_t &&sec)
    {
        _backend = backend;
        _sec = std::move(sec);
    }

public:
    using AnyIterator = SectionIterator;
    bool valid()
    {
        return std::visit([](auto &val) { return val && val->valid(); }, _sec);
    }
    void writeRecord(const record_t *record)
    {
        std::visit([&](auto &&val) { val->writeRecord(offset++, record); }, _sec);
    }
    void readRecord(record_t *record)
    {
        std::visit([&](auto &&val) { val->readRecord(offset++, record); }, _sec);
    }
    DataSection() = default;
    DataSection(DataSection<record_t> &&sec) { move_from(std::move(sec)); }
    DataSection &operator=(DataSection<record_t> &&sec)
    {
        move_from(std::move(sec));
        return *this;
    }

    virtual AnyIterator any_begin() const override
    {
        return std::visit([&](auto &&val) { return AnyIterator{val->begin()}; }, _sec);
    }

    virtual AnyIterator any_end() const override
    {
        return std::visit([&](auto &&val) { return AnyIterator{val->end()}; }, _sec);
    }

    auto any_range() const { return std::make_pair(any_begin(), any_end()); }

private:
    void move_from(DataSection<record_t> &&sec)
    {
        if (this == &sec)
        {
            return;
        }
        _backend = std::move(sec._backend);
        _sec = std::move(sec._sec);
        offset = sec.offset;
    }
};
class DirSection
{
    friend Backend;
    std::shared_ptr<Backend> _backend;
    void syncSectionDesc(std::string_view sectionName, desc_t desc);
    using section_t = std::variant<std::unique_ptr<fsl::DirSectionImpl>, std::unique_ptr<sql::DirSectionImpl>>;
    section_t _sec;

    DirSection(const DirSection &) = delete;
    DirSection &operator=(const DirSection &) = delete;

    void open(std::shared_ptr<Backend> backend, section_t &&sec)
    {
        _backend = backend;
        _sec = std::move(sec);
    }

public:
    // TODO: implement section map
    SectionDescMap secMap;
    DirSection openDir(desc_t desc, bool create);

private:
    template <typename record_t>
    DataSection<record_t> openData(std::string_view sectionName, bool create);
    template <typename record_t>
    DataSection<record_t> openData(bool create);

public:
#ifdef RAL_TYPE_REGISTER_H
    template <RecordEnum_t val>
    auto openDataStatic(bool create);
    template <RecordEnum_t val>
    auto openDataStaticUnique(bool create);

    // DataSection<typename EnumRegister<RecordEnum_t, val>::record_t> openDataStatic(ENUM e, bool create);
    // template <RecordEnum_t val>
    // auto openDataStatic(std::string_view sectionName, bool create);
    // DataSection<typename EnumRegister<RecordEnum_t, val>::record_t> openDataStatic(std::string_view sectionName,
#endif

    bool valid()
    {
        return std::visit([](auto &&val) { return val && val->valid(); }, _sec);
    }

    DirSection() = default;
    DirSection(DirSection &&sec) { move_from(std::move(sec)); }
    DirSection &operator=(DirSection &&sec)
    {
        move_from(std::move(sec));
        return *this;
    }

private:
    void move_from(DirSection &&sec)
    {
        if (this == &sec)
        {
            return;
        }
        _backend = std::move(sec._backend);
        _sec = std::move(sec._sec);
        secMap = std::move(sec.secMap);
    }
};

template <typename DirSectionImpl_t>
class DirSectionMixin
{
public:
    auto openDirSection(desc_t desc, bool create)
    {
        auto self = static_cast<DirSectionImpl_t *>(this);
        return self->openDirSection(desc, create);
    }
    auto openDataSection(desc_t desc, bool create, const utils::EncodedStruct &type_desc, int time_offset)
    {
        auto self = static_cast<DirSectionImpl_t *>(this);
        return self->openDataSection(desc, create, type_desc, time_offset);
    }
    auto openStringSection(desc_t desc, bool create)
    {
        auto self = static_cast<DirSectionImpl_t *>(this);
        return self->openStringSection(desc, create);
    }
    auto openStreamSection(desc_t desc, bool create)
    {
        auto self = static_cast<DirSectionImpl_t *>(this);
        return self->openStreamSection(desc, create);
    }
    auto begin()
    {
        auto self = static_cast<DirSectionImpl_t *>(this);
        return self->begin();
    }
    auto end()
    {
        auto self = static_cast<DirSectionImpl_t *>(this);
        return self->end();
    }

    auto self_desc() const
    {
        auto self = static_cast<const DirSectionImpl_t *>(this);
        return self->self_desc();
    }

    auto getSectionType(desc_t desc) const
    {
        auto self = static_cast<const DirSectionImpl_t *>(this);
        return self->getSectionType(desc);
    }
};

template <typename StreamSectionImpl_t>
class StreamSectionMixin
{
private:
    size_t off = 0;

public:
    size_t read(void *buf, size_t len)
    {
        auto self = static_cast<StreamSectionImpl_t *>(this);
        auto res = self->read(buf, off, len);
        off += len;
        return res;
    }

    size_t write(const void *buf, size_t len)
    {
        auto self = static_cast<StreamSectionImpl_t *>(this);
        off += len;
        return self->write(buf, len);
    }

    off_t tell() { return off; }

    size_t size() const
    {
        auto self = static_cast<const StreamSectionImpl_t *>(this);
        return self->size();
    }
};
template <typename StringSectionImpl_t>
class StringSectionMixin
{
public:
    size_t write(const char *str)
    {
        auto self = static_cast<StringSectionImpl_t *>(this);
        return self->write(str);
    }
    size_t read(char *str, size_t offset, int buf_len)
    {
        auto self = static_cast<StringSectionImpl_t *>(this);
        return self->read(str, offset, buf_len);
    }

    size_t total_length() const
    {
        auto self = static_cast<const StringSectionImpl_t *>(this);
        return self->total_length();
    }
};
template <typename DataSectionImpl_t>
class DataSectionMixin
{
public:
    void write(const void *record, size_t num)
    {
        auto self = static_cast<DataSectionImpl_t *>(this);
        self->write(record, num);
    }
    void read(void *record, size_t i)
    {
        auto self = static_cast<DataSectionImpl_t *>(this);
        self->read(record, i);
    }
    size_t size() const { return static_cast<const DataSectionImpl_t *>(this)->size(); }
    size_t record_size() const { return static_cast<const DataSectionImpl_t *>(this)->record_size(); }
    size_t time_offset() const { return static_cast<const DataSectionImpl_t *>(this)->time_offset(); }
    ral::desc_t self_desc() const { return static_cast<const DataSectionImpl_t *>(this)->self_desc(); }
};

class DataSectionInterface
{
public:
    virtual void write(const void *record) = 0;
    virtual void read(void *record, size_t i) = 0;
    virtual ~DataSectionInterface() = default;
    virtual size_t size() const = 0;
    virtual size_t record_size() const = 0;
    virtual size_t time_offset() const = 0;
    virtual desc_t self_desc() const = 0;

    class Iterator
    {
        DataSectionInterface *_sec;
        int idx;
        mutable std::vector<char> _recordBuffer;

    public:
        Iterator(DataSectionInterface *sec, int idx)
        : _sec(sec)
        , idx(idx)
        {
            _recordBuffer.resize(_sec->record_size());
        }

        Iterator &operator++()
        {
            ++idx;
            return *this;
        }
        Iterator operator++(int)
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        void *operator*() const
        {
            _sec->read(_recordBuffer.data(), idx);
            return _recordBuffer.data();
        }

        bool operator==(const Iterator &other) const { return idx == other.idx && desc() == other.desc(); }
        bool operator!=(const Iterator &other) const { return !(*this == other); }
        size_t time() const { return *(size_t *)((char *)*(*this) + _sec->time_offset()); }
        desc_t desc() const { return _sec->self_desc(); }
    };

    Iterator begin() { return Iterator(this, 0); }
    Iterator end() { return Iterator(this, size()); }
};
class StringSectionInterface
{
public:
    virtual size_t write(const char *str) = 0;
    virtual size_t read(char *str, size_t offset, int buf_len) = 0;
    virtual size_t total_length() const = 0;
    virtual ~StringSectionInterface() = default;
};
class StreamSectionInterface
{
public:
    virtual size_t read(void *buf, size_t len) = 0;

    virtual size_t write(const void *buf, size_t len) = 0;

    virtual off_t tell() = 0;
    virtual size_t size() const = 0;
    virtual ~StreamSectionInterface() = default;
};
class DirSectionInterface
{
public:
    virtual std::unique_ptr<DirSectionInterface> openDirSection(desc_t desc, bool create) = 0;

private:
public:
    virtual std::unique_ptr<DataSectionInterface>
    openDataSection(desc_t desc, bool create, const utils::EncodedStruct &type_desc, int time_offset) = 0;
    template <typename T>
    std::unique_ptr<DataSectionInterface>
    openDataSection(desc_t desc, bool create, int time_offset, int recordVariableLength = 0)
    {
        if (recordVariableLength > 0)
        {
            return openDataSection(
                desc, create, utils::StructEncoder<T>::get_encoded_struct(recordVariableLength), time_offset);
        }
        return openDataSection(desc, create, utils::StructEncoder<T>::get_encoded_struct(), time_offset);
    }
    virtual std::unique_ptr<StringSectionInterface> openStringSection(desc_t desc, bool create) = 0;
    virtual std::unique_ptr<StreamSectionInterface> openStreamSection(desc_t desc, bool create) = 0;
    virtual ~DirSectionInterface() = default;
    virtual desc_t self_desc() const = 0;
    virtual SectionBase::SectionType getSectionType(desc_t desc) const = 0;

    class Iterator
    {
        void *_impl;
        std::function<bool(SectionBase::SectionType)> _isa;
        std::function<std::unique_ptr<DirSectionInterface>()> _getDirSection;
        std::function<std::unique_ptr<DataSectionInterface>()> _getDataSection;
        std::function<std::unique_ptr<StringSectionInterface>()> _getStringSection;
        std::function<std::unique_ptr<StreamSectionInterface>()> _getStreamSection;
        std::function<void()> _self_add;
        std::function<void *()> _copy;
        std::function<void()> _destory;
        std::function<bool(const Iterator &)> _eq;
        std::function<desc_t()> _getDesc;

    public:
        template <typename T>
        Iterator(T impl)
        {
            _impl = new T(impl);
            _isa = [this](SectionBase::SectionType type) { return static_cast<T *>(_impl)->isa(type); };
            _getDataSection = [this]() { return std::move(static_cast<T *>(_impl)->getDataSection()); };
            _getDirSection = [this]() { return std::move(static_cast<T *>(_impl)->getDirSection()); };
            _getStringSection = [this]() { return std::move(static_cast<T *>(_impl)->getStringSection()); };
            _getStreamSection = [this]() { return std::move(static_cast<T *>(_impl)->getStreamSection()); };
            _self_add = [this]() { ++(*static_cast<T *>(_impl)); };
            _copy = [this]() { return (void *)new T(*static_cast<T *>(_impl)); };
            _destory = [this]() {
                delete static_cast<T *>(_impl);
                _impl = nullptr;
            };
            _eq = [this](const Iterator &other) { return *static_cast<T *>(_impl) == *static_cast<T *>(other._impl); };
            _getDesc = [this]() { return static_cast<T *>(_impl)->getDesc(); };
        }

        bool isa(SectionBase::SectionType type) const { return _isa(type); }

        std::unique_ptr<DirSectionInterface> getDirSection() { return _getDirSection(); }
        std::unique_ptr<DataSectionInterface> getDataSection() { return _getDataSection(); }
        std::unique_ptr<StringSectionInterface> getStringSection() { return _getStringSection(); }
        std::unique_ptr<StreamSectionInterface> getStreamSection() { return _getStreamSection(); }

        Iterator(const Iterator &other)
        {
            _impl = other._copy();
            _isa = other._isa;
            _getDataSection = other._getDataSection;
            _getDirSection = other._getDirSection;
            _getStringSection = other._getStringSection;
            _getStreamSection = other._getStreamSection;
            _self_add = other._self_add;
            _copy = other._copy;
            _destory = other._destory;
            _eq = other._eq;
            _getDesc = other._getDesc;
        }

        Iterator &operator=(const Iterator &other)
        {
            if (this == &other)
            {
                return *this;
            }
            _destory();
            _impl = other._copy();
            _isa = other._isa;
            _getDataSection = other._getDataSection;
            _getDirSection = other._getDirSection;
            _getStringSection = other._getStringSection;
            _getStreamSection = other._getStreamSection;
            _self_add = other._self_add;
            _copy = other._copy;
            _destory = other._destory;
            _eq = other._eq;
            _getDesc = other._getDesc;
            return *this;
        }

        Iterator operator++(int)
        {
            auto res = *this;
            _self_add();
            return res;
        }

        Iterator &operator++()
        {
            _self_add();
            return *this;
        }

        desc_t getDesc() const { return _getDesc(); }

        bool operator==(const Iterator &other) const { return _eq(other); }
        bool operator!=(const Iterator &other) const { return !_eq(other); }

        ~Iterator() { _destory(); }
    };
    virtual Iterator begin() = 0;
    virtual Iterator end() = 0;
};
template <typename T>
    requires std::is_base_of_v<DataSectionMixin<T>, T>
class DataSectionWrapper : public DataSectionInterface
{
    T _sec;
    unsigned int _record_size;
    size_t cur;
    char* buffer;
    static constexpr size_t BUFFER_RECORD_NUM = 1;

public:
    DataSectionWrapper(T &&sec, unsigned int record_size)
    : _sec(std::move(sec)),
    _record_size(record_size),
    cur(0)
    {
        buffer = new char[record_size * BUFFER_RECORD_NUM];
        if (_record_size == -1)
        {
            _record_size = this->record_size();
        }
    }
    virtual ~DataSectionWrapper()
    {
        flush();
        delete[] buffer;
    }

    void flush()
    {
        if (cur > 0)
        {
            auto &_sec_mixin = static_cast<DataSectionMixin<T> &>(_sec);
            _sec_mixin.write(buffer, cur);
            cur = 0;
        }
    }
    virtual void write(const void *record) override
    {
        auto &_sec_mixin = static_cast<DataSectionMixin<T> &>(_sec);
        memcpy(buffer + cur * _record_size, record, _record_size);
        ++cur;
        if (cur == BUFFER_RECORD_NUM)
        {
            flush();
        }
    }
    virtual void read(void *record, size_t i) override
    {
        auto &_sec_mixin = static_cast<DataSectionMixin<T> &>(_sec);
        _sec_mixin.read(record, i);
    }

    virtual size_t size() const override
    {
        auto &_sec_mixin = static_cast<const DataSectionMixin<T> &>(_sec);
        return _sec_mixin.size();
    }

    virtual size_t record_size() const override
    {
        auto &_sec_mixin = static_cast<const DataSectionMixin<T> &>(_sec);
        return _sec_mixin.record_size();
    }

    virtual size_t time_offset() const override
    {
        auto &_sec_mixin = static_cast<const DataSectionMixin<T> &>(_sec);
        return _sec_mixin.time_offset();
    }

    virtual desc_t self_desc() const override
    {
        auto &_sec_mixin = static_cast<const DataSectionMixin<T> &>(_sec);
        return _sec_mixin.self_desc();
    }
};
template <typename T>
    requires std::is_base_of_v<StringSectionMixin<T>, T>
class StringSectionWrapper : public StringSectionInterface
{
    T _sec;

public:
    StringSectionWrapper(T &&sec)
    : _sec(std::move(sec))
    {
    }

    virtual size_t write(const char *str) override
    {
        auto &_sec_mixin = static_cast<StringSectionMixin<T> &>(_sec);
        return _sec_mixin.write(str);
    }
    virtual size_t read(char *str, size_t offset, int buf_len) override
    {
        auto &_sec_mixin = static_cast<StringSectionMixin<T> &>(_sec);
        return _sec_mixin.read(str, offset, buf_len);
    }
    virtual size_t total_length() const
    {
        auto &_sec_mixin = static_cast<const StringSectionMixin<T> &>(_sec);
        return _sec_mixin.total_length();
    }
};
template <typename T>
    requires std::is_base_of_v<StreamSectionMixin<T>, T>
class StreamSectionWrapper : public StreamSectionInterface
{
    T _sec;

public:
    StreamSectionWrapper(T &&sec)
    : _sec(std::move(sec))
    {
    }

    virtual size_t write(const void *buf, size_t len) override
    {
        auto &_sec_mixin = static_cast<StreamSectionMixin<T> &>(_sec);
        return _sec_mixin.write(buf, len);
    }
    virtual size_t read(void *buf, size_t len) override
    {
        auto &_sec_mixin = static_cast<StreamSectionMixin<T> &>(_sec);
        return _sec_mixin.read(buf, len);
    }
    virtual off_t tell() override
    {
        auto &_sec_mixin = static_cast<StreamSectionMixin<T> &>(_sec);
        return _sec_mixin.tell();
    }

    virtual size_t size() const override
    {
        auto &_sec_mixin = static_cast<const StreamSectionMixin<T> &>(_sec);
        return _sec_mixin.size();
    }
};
template <typename T>
    requires std::is_base_of_v<DirSectionMixin<T>, T>
class DirSectionWrapper : public DirSectionInterface
{
    using self_t = DirSectionWrapper<T>;
    T _sec;

public:
    DirSectionWrapper(T &&sec)
    : _sec(std::move(sec))
    {
    }
    virtual std::unique_ptr<DirSectionInterface> openDirSection(desc_t desc, bool create) override
    {
        auto &_sec_mixin = static_cast<DirSectionMixin<T> &>(_sec);
        return std::make_unique<self_t>(std::move(_sec_mixin.openDirSection(desc, create)));
    }
    virtual std::unique_ptr<DataSectionInterface>
    openDataSection(desc_t desc, bool create, const utils::EncodedStruct &type_desc, int time_offset) override
    {
        auto &_sec_mixin = static_cast<DirSectionMixin<T> &>(_sec);
        using sec_t = decltype(_sec_mixin.openDataSection(desc, create, type_desc, 0));
        return std::make_unique<DataSectionWrapper<sec_t>>(
            std::move(_sec_mixin.openDataSection(desc, create, type_desc, time_offset)),
            type_desc.size);
    }
    virtual std::unique_ptr<StringSectionInterface> openStringSection(desc_t desc, bool create)
    {
        auto &_sec_mixin = static_cast<DirSectionMixin<T> &>(_sec);
        using sec_t = decltype(_sec_mixin.openStringSection(desc, create));
        return std::make_unique<StringSectionWrapper<sec_t>>(std::move(_sec_mixin.openStringSection(desc, create)));
    }
    virtual std::unique_ptr<StreamSectionInterface> openStreamSection(desc_t desc, bool create)
    {
        auto &_sec_mixin = static_cast<DirSectionMixin<T> &>(_sec);
        using sec_t = decltype(_sec_mixin.openStreamSection(desc, create));
        return std::make_unique<StreamSectionWrapper<sec_t>>(std::move(_sec_mixin.openStreamSection(desc, create)));
    }

    virtual desc_t self_desc() const
    {
        auto &_sec_mixin = static_cast<const DirSectionMixin<T> &>(_sec);
        return _sec_mixin.self_desc();
    }

    virtual SectionBase::SectionType getSectionType(desc_t desc) const
    {
        auto &_sec_mixin = static_cast<const DirSectionMixin<T> &>(_sec);
        return _sec_mixin.getSectionType(desc);
    }

    class DirSectionIterator
    {
        using iterator_impl_t = typename T::Iterator;
        iterator_impl_t _impl;

    public:
        DirSectionIterator(iterator_impl_t impl)
        : _impl(impl)
        {
        }

        DirSectionIterator &operator++()
        {
            ++_impl;
            return *this;
        }
        DirSectionIterator operator++(int)
        {
            auto res = *this;
            ++_impl;
            return res;
        }

        bool isa(SectionBase::SectionType type) const { return _impl.isa(type); }

        bool operator==(const DirSectionIterator &other) const { return _impl == other._impl; }
        bool operator!=(const DirSectionIterator &other) const { return _impl != other._impl; }
        std::unique_ptr<DirSectionInterface> getDirSection()
        {
            if (!_impl.isa(SectionBase::DIR))
            {
                return nullptr;
            }
            return std::make_unique<self_t>(_impl.getDirSection());
        }
        std::unique_ptr<DataSectionInterface> getDataSection()
        {
            if (!_impl.isa(SectionBase::DATA))
            {
                return nullptr;
            }
            return std::make_unique<DataSectionWrapper<std::result_of_t<decltype (&DirSectionMixin<T>::openDataSection)(
                DirSectionMixin<T>, desc_t, bool, const utils::EncodedStruct, int)>>>(_impl.getDataSection(), -1);
        }

        std::unique_ptr<StringSectionInterface> getStringSection()
        {
            if (!_impl.isa(SectionBase::STRING))
            {
                return nullptr;
            }
            return std::make_unique<StringSectionWrapper<
                std::result_of_t<decltype (&DirSectionMixin<T>::openStringSection)(DirSectionMixin<T>, desc_t, bool)>>>(
                _impl.getStringSection());
        }
        std::unique_ptr<StreamSectionInterface> getStreamSection()
        {
            if (!_impl.isa(SectionBase::STREAM))
            {
                return nullptr;
            }
            return std::make_unique<StreamSectionWrapper<
                std::result_of_t<decltype (&DirSectionMixin<T>::openStreamSection)(DirSectionMixin<T>, desc_t, bool)>>>(
                _impl.getStreamSection());
        }

        desc_t getDesc() const { return _impl.getDesc(); }
    };

    Iterator begin() { return Iterator(DirSectionIterator(_sec.begin())); }
    Iterator end() { return Iterator(DirSectionIterator(_sec.end())); }
};
} // namespace ral

} // namespace pse
