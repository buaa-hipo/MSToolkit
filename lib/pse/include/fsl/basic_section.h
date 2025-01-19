#pragma once
#include <fsl/fsl_lib.h>
namespace pse
{
namespace fsl
{

class BasicStringSection
{
    DataSectionDescBlock *sec;
    BlockManager *manager;

public:
    BasicStringSection(DataSectionDescBlock *sec, BlockManager *manager)
    : sec(sec)
    , manager(manager)
    {
    }
    void write(const std::string &record)
    {
        int string_num;
        auto data = record.c_str();
        int len = record.size();
        if (sec->sectionSize == 0)
        {
            string_num = 1;
        }
        else
        {
            string_num = manager->readDataSection(sec, 0, &string_num, sizeof(string_num));
            string_num++;
        }
        manager->writeDataSection(sec, 0, &string_num, sizeof(string_num));
        manager->writeDataSectionAtEnd(sec, &len, sizeof(len));
        manager->writeDataSectionAtEnd(sec, data, len);
    }
};
class ParallelBasicStringSection
{
    DataSectionDescBlock *sec;
    BlockManager *manager;

public:
    ParallelBasicStringSection(DataSectionDescBlock *sec, BlockManager *manager)
    : sec(sec)
    , manager(manager)
    {
    }
    void write(const std::string &record)
    {
        auto lock = utils::SpinLock<int>(sec->atomic_lock);
        auto guard = utils::LockGuard(lock, true);
        int string_num;
        auto data = record.c_str();
        int len = record.size();
        if (sec->sectionSize == 0)
        {
            string_num = 1;
        }
        else
        {
            string_num = manager->readDataSection(sec, 0, &string_num, sizeof(string_num));
            string_num++;
        }
        manager->writeDataSection(sec, 0, &string_num, sizeof(string_num));
        manager->writeDataSectionAtEnd(sec, &len, sizeof(len));
        manager->writeDataSectionAtEnd(sec, data, len);
    }
};
class BasicMapSection
{
};
class BasicDirSection
{
    std::shared_ptr<BlockManager> manager;
    DirSectionDescBlock *sec;

public:
    auto get_manager() -> decltype(manager) { return manager; }
    BasicDirSection(std::shared_ptr<BlockManager> manager, DirSectionDescBlock *sec)
    : manager(manager)
    , sec(sec)
    {
    }

    DirSectionIterator begin() { return DirSectionIterator(0, sec, manager); }
    DirSectionIterator end() { return DirSectionIterator(DirSectionDescBlock::LEVEL3_ENTRY_SUM, sec, manager); }

    BasicDirSection openDirSection(Block::desc_t desc, bool create, bool force = false)
    {
        return BasicDirSection(manager, manager->openDirSection(sec, desc, create, force));
    }

    static BasicDirSection openRootSection(std::shared_ptr<BlockManager> manager)
    {
        return BasicDirSection(manager, manager->openRootSection());
    }

    DataSectionDescBlock *
    openDataSection(Block::desc_t desc, bool create, bool force = false, int recordVariableLength = 0)
    {
        return manager->openDataSection(sec, desc, create, force, recordVariableLength);
    }

    ral::desc_t self_desc() const { return sec->self_desc; }

    pse::ral::SectionBase::SectionType getSectionType(ral::desc_t desc) const
    {
        auto blk = manager->openSection(sec, desc, false, false, 0);
        if (!blk)
        {
            return pse::ral::SectionBase::UNKNOWN;
        }
        if (auto data_sec = Block::dyn_cast<DataSectionDescBlock>(blk))
        {
            return (pse::ral::SectionBase::SectionType)data_sec->dataSectionType;
        }
        else if (Block::dyn_cast<DirSectionDescBlock>(blk))
        {
            return pse::ral::SectionBase::DIR;
        }
        else
        {
            return pse::ral::SectionBase::UNKNOWN;
        }
    }
};

} // namespace fsl

} // namespace pse
