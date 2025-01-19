#pragma once
#include "ral/section.h"
#include <string_view>
#include <fsl/fsl_block.h>
#include <fsl/fsl_lib.h>
#include <ral/backend.h>
#include <fsl/basic_section.h>
#include <ral/section_declare.h>

namespace pse
{
namespace fsl
{

class StreamSection : public ral::SectionBase, public ral::StreamSectionMixin<StreamSection>
{
    friend ral::StreamSectionMixin<StreamSection>;

    std::shared_ptr<BlockManager> manager;
    DataSectionDescBlock *sec;

public:
    StreamSection(std::shared_ptr<BlockManager> manager, DataSectionDescBlock *sec)
    : SectionBase(STREAM)
    , manager(manager)
    , sec(sec)
    {
        sec->dataSectionType = STREAM;
    }

private:
    size_t read(void *buf, size_t offset, size_t len) { return manager->readDataSection(sec, offset, buf, len); }

    size_t write(const void *buf, size_t len)
    {
        manager->writeDataSectionAtEnd(sec, buf, len);
        return len;
    }
    size_t size() const { return sec->sectionSize; }
};

class RawStringSection : public ral::SectionBase, public ral::StringSectionMixin<RawStringSection>
{
    friend ral::StringSectionMixin<RawStringSection>;
    std::shared_ptr<BlockManager> manager;
    DataSectionDescBlock *sec;

public:
    RawStringSection(std::shared_ptr<BlockManager> manager, DataSectionDescBlock *sec)
    : SectionBase(STRING)
    , manager(manager)
    , sec(sec)
    {
        sec->dataSectionType = STRING;
    }

private:
    size_t write(const char *str)
    {
        int len = strlen(str);
        auto pos = manager->allocateAtDataSection(sec, sizeof(int) + len);
        manager->writeDataSection(sec, pos, &len, sizeof(int));
        manager->writeDataSection(sec, pos + sizeof(int), str, len);
        return pos;
    }

    size_t read(char *str, size_t offset, int buf_len)
    {
        int len;
        manager->readDataSection(sec, offset, &len, sizeof(int));
        if (buf_len >= len)
        {
            manager->readDataSection(sec, offset + sizeof(int), str, len);
        }
        return len;
    }
    size_t total_length() const { return sec->sectionSize; }
};

/**
 * @brief directly store records by row.
 * 0~63 bytes head: variable_field_offset(4byte), record_size(4byte)
 * recores are stored from 64 bytes, and no padding.
 */
class RawDataSection : public ral::SectionBase, public ral::DataSectionMixin<RawDataSection>
{
    friend ral::DataSectionMixin<RawDataSection>;
    constexpr static size_t HEAD_LENGTH = 64;
    std::shared_ptr<BlockManager> manager;
    DataSectionDescBlock *sec;
    int _time_offset;
    utils::EncodedStruct type_desc;

public:
    RawDataSection(std::shared_ptr<BlockManager> manager,
                   DataSectionDescBlock *sec,
                   const utils::EncodedStruct &_type_desc,
                   int time_offset)
    : SectionBase(DATA)
    , manager(manager)
    , sec(sec)
    , _time_offset(time_offset)
    , type_desc(get_type_desc(_type_desc))
    {
        sec->dataSectionType = DATA;
    }

private:
    void write(const void *record, size_t num) { manager->writeDataSectionAtEnd(sec, record, type_desc.size*num); }
    void read(void *record, size_t i)
    {
        manager->readDataSection(sec, HEAD_LENGTH + i * type_desc.size, record, type_desc.size);
    }

    size_t get_record_size()
    {
        size_t size;
        manager->readDataSection(sec, 0, &size, sizeof(size));
        return size;
    }
    size_t time_offset() const { return _time_offset; }
    utils::EncodedStruct get_type_desc(const utils::EncodedStruct &_type_desc)
    {
        if (sec->sectionSize == 0)
        {
            manager->writeDataSectionAtEnd(
                sec, &_type_desc.variable_field_offset, sizeof(_type_desc.variable_field_offset));
            manager->writeDataSectionAtEnd(sec, &_type_desc.size, sizeof(_type_desc.size));
            manager->writeDataSectionAtEnd(sec, &_time_offset, sizeof(_time_offset));
            sec->sectionSize = HEAD_LENGTH;
            return _type_desc;
        }
        else
        {
            unsigned int size;
            int variable_field_offset;
            manager->readDataSection(sec, 0, &variable_field_offset, sizeof(variable_field_offset));
            manager->readDataSection(sec, 4, &size, sizeof(size));
            manager->readDataSection(sec, 8, &_time_offset, sizeof(_time_offset));
            return {{}, {}, {}, "", variable_field_offset, size};
        }
    }

    size_t size() const { return (sec->sectionSize - HEAD_LENGTH) / type_desc.size; }

    size_t record_size() const { return type_desc.size; }
    ral::desc_t self_desc() const { return sec->self_desc; }
};

class RawDirSection : public ral::SectionBase, public ral::DirSectionMixin<RawDirSection>
{
    BasicDirSection sec;
    friend ral::DirSectionMixin<RawDirSection>;

public:
    RawDirSection(BasicDirSection sec)
    : SectionBase(DIR)
    , sec(sec)
    {
    }

    class Iterator
    {
        pse::fsl::DirSectionIterator iter;
        mutable Block *sec = nullptr;
        mutable bool valid = false;

        Block *get() const
        {
            auto block = *iter;
            if (!block)
            {
                return nullptr;
            }
            sec = block;
            valid = true;
            return sec;
        }

    public:
        Iterator &operator++()
        {
            ++iter;
            valid = false;
            return *this;
        }
        Iterator operator++(int)
        {
            auto res = *this;
            ++iter;
            valid = false;
            return res;
        }
        Iterator(pse::fsl::DirSectionIterator iter)
        : iter(iter)
        {
        }
        bool operator==(const Iterator &other) const
        {
            // spdlog::info("compare {} and {}", (uint64_t)get(), (uint64_t)other.get());
            return get() == other.get();
        }
        bool operator!=(const Iterator &other) const
        {
            // spdlog::info("compare {} and {}", (uint64_t)get(), (uint64_t)other.get());
            auto res = get() != other.get();
            return res;
        }

        bool isa(SectionBase::SectionType type) const
        {
            if (type == SectionBase::DIR)
            {
                if (get() && Block::isa<DirSectionDescBlock>(get()))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                if (auto data_sec = Block::nullable_cast<DataSectionDescBlock>(get()))
                {
                    return data_sec->dataSectionType == type;
                }
                else
                {
                    return false;
                }
            }
        }

        RawDirSection getDirSection()
        {
            if (get() && Block::isa<DirSectionDescBlock>(get()))
            {
                return RawDirSection(BasicDirSection(iter.get_manager(), Block::cast<DirSectionDescBlock>(get())));
            }
            else
            {
                spdlog::error("not a dir section");
                throw std::runtime_error("not a dir section");
            }
        }

        RawDataSection getDataSection()
        {
            if (auto data_sec = Block::nullable_cast<DataSectionDescBlock>(get()))
            {
                return RawDataSection(iter.get_manager(), data_sec, {}, 0);
            }
            else
            {
                spdlog::error("not a data section");
                throw std::runtime_error("not a data section");
            }
        }

        RawStringSection getStringSection()
        {
            if (auto data_sec = Block::nullable_cast<DataSectionDescBlock>(get()))
            {
                return RawStringSection(iter.get_manager(), data_sec);
            }
            else
            {
                spdlog::error("not a string section");
                throw std::runtime_error("not a string section");
            }
        }

        StreamSection getStreamSection()
        {
            if (auto data_sec = Block::nullable_cast<DataSectionDescBlock>(get()))
            {
                return StreamSection(iter.get_manager(), data_sec);
            }
            else
            {
                spdlog::error("not a stream section");
                throw std::runtime_error("not a stream section");
            }
        }

        ral::desc_t getDesc() const { return iter.getDesc(); }
    };
    Iterator begin() { return Iterator(sec.begin()); }
    Iterator end() { return Iterator(sec.end()); }

    ral::desc_t self_desc() const { return sec.self_desc(); }

    pse::ral::SectionBase::SectionType getSectionType(ral::desc_t desc) const { return sec.getSectionType(desc); }

private:
    RawDirSection openDirSection(ral::desc_t desc, bool create)
    {
        return RawDirSection(sec.openDirSection(desc, create));
    }

    RawDataSection
    openDataSection(ral::desc_t desc, bool create, const utils::EncodedStruct &type_desc, int time_offset)
    {
        return RawDataSection(sec.get_manager(), sec.openDataSection(desc, create), type_desc, time_offset);
    }
    RawStringSection openStringSection(ral::desc_t desc, bool create)
    {
        return RawStringSection(sec.get_manager(), sec.openDataSection(desc, create));
    }
    StreamSection openStreamSection(ral::desc_t desc, bool create)
    {
        return StreamSection(sec.get_manager(), sec.openDataSection(desc, create));
    }
};

class RawSectionBackend : public ral::BackendMixin<RawSectionBackend>
{
private:
    std::shared_ptr<BlockManager> manager;
    friend ral::BackendMixin<RawSectionBackend>;

public:
    RawSectionBackend(std::string_view name, ral::RWMode mode)
    : manager(std::make_shared<BlockManager>(name, mode))
    {
    }

    bool isLeader()
    {
        return manager->isLeader();
    }

private:
    RawDirSection openRootSection() { return BasicDirSection::openRootSection(manager); }
};

} // namespace fsl
} // namespace pse
