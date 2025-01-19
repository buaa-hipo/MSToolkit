#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>

#include <mutex>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <fsl/fsl_lib.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <exception>

#include <utils/lock.h>

namespace pse
{
namespace fsl
{

size_t FileSectionLayerDriver::fileSize()
{
    struct stat s;
    fstat(fd, &s);
    return s.st_size;
}

void FileSectionLayerDriver::preserve(size_t len)
{
    // if (driver_atomic_lock)
    // {
    //     driver_atomic_lock->lock();
    // }
    utils::LockGuard lock_guard(driver_atomic_lock, driver_atomic_lock != nullptr);
    if (len > fileSize())
    {
        auto res = ftruncate(fd, len);
        if (res < 0)
        {
            spdlog::error("failed to ftruncate fd {} with file size {}: {}", fd, len, strerror(errno));
            throw std::runtime_error(
                fmt::format("failed to ftruncate fd {} with file size {}: {}", fd, len, strerror(errno)));
        }
    }
    // if (driver_atomic_lock)
    // {
    //     driver_atomic_lock->unlock();
    // }
}
void *FileSectionLayerDriver::load(size_t offset)
{
    size_t pageOffset = pageAlign(offset);
    // auto iter = this->id2ptr.find(pageOffset);
    // if (iter != this->id2ptr.end())
    // {
    //     return iter->second;
    // }
    auto tryRes = tryLoad(offset);
    if (tryRes)
    {
        return tryRes;
    }
    // spdlog::info("load page offset: {}", pageOffset);
    auto nextPreservedLength = std::max(pageOffset + PAGE_SIZE, 2 * mapedEnd);
    if (nextPreservedLength > fileSize())
    {
        preserve(nextPreservedLength);
    }
    auto newLoadedPage = [this, nextPreservedLength]() {
        utils::LockGuard lock_guard(driver_atomic_lock, driver_atomic_lock != nullptr);
        if (mapedEnd < nextPreservedLength)
        {
            auto res = (char *)mmap(mmap_ptr + mapedEnd,
                                    nextPreservedLength - mapedEnd,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_FIXED,
                                    fd,
                                    mapedEnd);
            mprotect(res, nextPreservedLength - mapedEnd, PROT_READ | PROT_WRITE);
            mapedEnd = nextPreservedLength;
            return res;
        }
        else
        {
            return mmap_ptr + mapedEnd;
        }
    }();
    // assert(page == mmap_ptr + mapedEnd);
    spdlog::info("mmap {:x} with length {:x} to {:x}, total length {:x}",
                 (size_t)(mmap_ptr + mapedEnd),
                 nextPreservedLength - mapedEnd,
                 (size_t)(mmap_ptr + nextPreservedLength),
                 nextPreservedLength);
    // spdlog::info("map page: {} for offset {:X}", page, pageOffset);
    // checke pageoffset is PAGE_SIZE align
    if (pageOffset % PAGE_SIZE != 0)
    {
        spdlog::error("page offset {} is not PAGE_SIZE align", pageOffset);
        throw std::runtime_error("page offset is not PAGE_SIZE align");
    }
    // auto page = (void *)new char[PAGE_SIZE];
    if (newLoadedPage == MAP_FAILED)
    {
        spdlog::error("failed to mmap offset {} for fd {}: {}", offset, fd, strerror(errno));
        throw std::runtime_error("mmap failed");
    }
    // this->id2ptr.insert({pageOffset, page});
    // this->ptr2id.insert({page, pageOffset});
    // return page;
    return mmap_ptr + pageOffset;
}
void FileSectionLayerDriver::unload(void *ptr)
{
    printf("unload ptr: %p\n", ptr);
    auto i1 = this->ptr2id.find(ptr);
    assert(i1 != this->ptr2id.end() && "try to munmap an illegal address");
    auto res = this->id2ptr.erase(i1->second);
    assert(res && "try to munmap an illegal address");
    this->ptr2id.erase(i1);

    auto err = munmap(ptr, PAGE_SIZE);
    if (err != 0)
    {
        spdlog::error("failed to mnumap ptr {} for fd {}: {}", ptr, fd, strerror(errno));
        throw std::runtime_error("failed to munmap");
    }
}
void *FileSectionLayerDriver::tryLoad(size_t offset)
{
    size_t pageOffset = pageAlign(offset);
    if (pageOffset + PAGE_SIZE <= mapedEnd)
    {
        return mmap_ptr + pageOffset;
    }
    return nullptr;

    // auto iter = this->id2ptr.find(pageOffset);
    // if (iter != this->id2ptr.end())
    // {
    //     return iter->second;
    // }
    // return nullptr;
}

void *FileSectionLayerDriver::loadBlock(size_t blockId)
{
    // if (driver_atomic_lock)
    // {
    //     driver_atomic_lock->lock();
    // }
    // spdlog::debug("load blockId {}", blockId);
    auto pagePtr = (char *)load(blockId * BLOCK_SIZE);
    // if (driver_atomic_lock)
    // {
    //     driver_atomic_lock->unlock();
    // }
    // spdlog::debug("load blockId {} at {:x} with offset {:x}",
    //               blockId,
    //               (size_t)pagePtr + (blockId % BLOCKS_PER_PAGE) * BLOCK_SIZE,
    //               blockId % BLOCKS_PER_PAGE * BLOCK_SIZE + (pagePtr - mmap_ptr));
    // spdlog::debug("page offet: {:x}", pagePtr - mmap_ptr);
    // spdlog::debug("page align: {:x}", pageAlign(blockId * BLOCK_SIZE));
    // printf("load pageptr: %p\n", pagePtr);
    return pagePtr + (blockId % BLOCKS_PER_PAGE) * BLOCK_SIZE;
}

std::pair<void *, Block::block_id_t> FileSectionLayerDriver::allocateBlock()
{
    // std::lock_guard lock(*driver_atomic_lock);
    auto blkId = blockAllocated->fetch_add(1);
    auto blk = loadBlock(blkId);
    return {blk, (Block::block_id_t)blkId};
}

std::tuple<void *, Block::block_id_t, size_t> FileSectionLayerDriver::allocateBlock(size_t blockNum)
{
    // std::lock_guard lock(*driver_atomic_lock);
    auto blkId = blockAllocated->fetch_add(blockNum);
    auto blk = loadBlock(blkId);
    auto blocks_reserved = BLOCKS_PER_PAGE - blkId % BLOCKS_PER_PAGE;
    auto continousNum = blockNum < blocks_reserved ? blockNum : blocks_reserved;
    return {blk, (Block::block_id_t)blkId, continousNum};
}

bool FileSectionLayerDriver::blockExists(size_t blockId) { return fileSize() >= detail::blockId2Offset(blockId + 1); }

FileSectionLayerDriver::FileSectionLayerDriver(std::string_view filename)
: _filename(filename)
, blockAllocated(nullptr)
, driver_atomic_lock(nullptr)
{
    fd = open(_filename.c_str(), O_RDWR | O_CREAT | O_EXCL, 0777);
    if (fd == -1)
    {
        fd = open(_filename.c_str(), O_RDWR, 0777);
        this->is_creator = false;
    }
    else
    {
        this->is_creator = true;
    }
    if (fd < 0)
    {
        spdlog::error("failed to open file {}: {}", filename, strerror(errno));
        throw std::runtime_error("failed to open file");
    }

    mmap_ptr = (char *)mmap(
        nullptr, DEFAULT_INIT_MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED | MAP_NORESERVE, -1, 0);
    mprotect(mmap_ptr, DEFAULT_INIT_MMAP_SIZE, PROT_NONE);
    if (mmap_ptr == MAP_FAILED)
    {
        spdlog::error("failed to init mmap: {}", strerror(errno));
        throw std::runtime_error("mmap failed");
    }
}

// FIXME: safe munmap
FileSectionLayerDriver::~FileSectionLayerDriver()
{
    // for (const auto &p : this->ptr2id)
    // {
    //     munmap(p.first, PAGE_SIZE);
    // }
    close(fd);
    delete blockAllocated;
    delete driver_atomic_lock;
    // msync(mmap_ptr, 1L << 36, MS_SYNC);
    auto res = munmap(mmap_ptr, DEFAULT_INIT_MMAP_SIZE);
    if (res != 0)
    {
        spdlog::error("failed to munmap mmap_ptr: {}", strerror(errno));
    }
    spdlog::info("driver exit");
}

BlockManager::BlockManager(std::string_view name, ral::RWMode mode)
: driver{name}
, _mode(mode)
{
    // TODO: READ / WRITE / APPEND / RW mode support
    if (mode == ral::WRITE)
    {
        std::string lock_name(name);
        lock_name += ".lock";
        if (driver.is_creator)
        {
            super = reinterpret_cast<SuperBlock *>(driver.loadBlock(0));
            super->init();
            root = reinterpret_cast<DirSectionDescBlock *>(driver.loadBlock(1));
            root->init();
            driver.blockAllocated = new std::atomic_ref(super->blockAllocated);
            driver.blockAllocated->store(2);
            driver.driver_atomic_lock = new utils::SpinLock(super->atomic_lock);
            spdlog::debug("open lock file");
            spdlog::debug("access to lock file: {}", access(lock_name.c_str(), F_OK));
            auto res = open(lock_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0777);
            if (res < 0)
            {
                spdlog::error("failed to create lock file {}: {}", lock_name, strerror(errno));
                throw std::runtime_error("failed to create lock file");
            }
        }
        else
        {
            while (access(lock_name.c_str(), F_OK) != 0)
            {
                spdlog::info("wait for lock file {}", lock_name);
                sleep(1);
            }
            spdlog::debug("detect lock file");
            super = reinterpret_cast<SuperBlock *>(driver.loadBlock(0));
            root = reinterpret_cast<DirSectionDescBlock *>(driver.loadBlock(1));
            driver.blockAllocated = new std::atomic_ref(super->blockAllocated);
            driver.driver_atomic_lock = new utils::SpinLock(super->atomic_lock);
            assert(driver.blockAllocated->load() > 0);
        }
    }
    else if (mode == ral::READ)
    {
        assert(driver.blockExists(0));
        super = Block::cast<SuperBlock>(driver.loadBlock(0));
        assert(driver.blockExists(super->rootId));
        root = Block::cast<DirSectionDescBlock>(driver.loadBlock(1));
        driver.blockAllocated = new std::atomic_ref(super->blockAllocated);
        driver.driver_atomic_lock = new utils::SpinLock(super->atomic_lock);
    }
}

DirSectionDescBlock *BlockManager::openRootSection() { return root; }

void *BlockManager::openSection(
    DirSectionDescBlock *parent, Block::desc_t desc, int createMode, bool force, int recordVariableLength)
{
    // TODO: limit the lock scope
    utils::SpinLock lock(parent->atomic_lock);
    utils::LockGuard lock_guard(lock, !force); // Define the lock_guard class with the SpinLock type
    if (desc == 0)
    {
        return nullptr;
    }

    auto entry = findEntry(parent, desc, createMode);
    if (!entry)
    {
        return nullptr;
    }

    if (!entry->blockId && createMode)
    {
        if (createMode == CREATE_DIR_SECTION)
        {
            auto [blk, blkId] = allocate<DirSectionDescBlock>();
            blk->self_desc = desc;
            entry->blockId = blkId;
            entry->desc = desc;
            spdlog::debug("create dir section {} at block {}: {:x}", desc, blkId, (size_t)blk);
            assert(blk != nullptr);
            return blk;
        }
        else if (createMode == CREATE_DATA_SECTION)
        {
            auto [blk, blkId] = allocate<DataSectionDescBlock>();
            blk->recordVariableLength = recordVariableLength;
            blk->self_desc = desc;
            entry->blockId = blkId;
            entry->desc = desc;
            spdlog::debug("create data section {} at block {}: {:x}", desc, blkId, (size_t)blk);
            assert(blk != nullptr);
            return blk;
        }
        else
        {
            throw std::invalid_argument(fmt::format("create mode {} not supported", createMode));
        }
    }
    if (entry->blockId)
    {
        assert(driver.blockExists(entry->blockId));
        auto blk = driver.loadBlock(entry->blockId);
        spdlog::debug("open section {} at block {}: {:x} with type {}",
                      desc,
                      entry->blockId,
                      (size_t)blk,
                      ((DataSectionDescBlock *)blk)->blockTypeId);
        assert((Block::isa<DataSectionDescBlock, DirSectionDescBlock>(blk)));
        assert(blk != nullptr);
        return blk;
    }
    else
    {
        return nullptr;
    }
}

DirSectionDescBlock *BlockManager::openDirSection(DirSectionDescBlock *sec, Block::desc_t desc, bool create, bool force)
{
    int createMode = create ? CREATE_DIR_SECTION : NO_CREATE;
    return Block::nullable_cast<DirSectionDescBlock>(openSection(sec, desc, createMode, force, 0));
}

DataSectionDescBlock *BlockManager::openDataSection(
    DirSectionDescBlock *sec, Block::desc_t desc, bool create, bool force, int recordVariableLength)
{
    int createMode = create ? CREATE_DATA_SECTION : NO_CREATE;
    return Block::nullable_cast<DataSectionDescBlock>(openSection(sec, desc, createMode, force, recordVariableLength));
}

void BlockManager::writeDataSection(DataSectionDescBlock *sec, size_t offset, const void *buf, size_t len)
{
    spdlog::debug("block addr: {:x}", (size_t)sec);
    {
        utils::SpinLock lock(sec->atomic_lock);
        std::lock_guard guard(lock);
        if (offset == -1ul)
        {
            offset = sec->sectionSize;
            sec->sectionSize += len;
        }
        else
        {
            sec->sectionSize = std::max(sec->sectionSize, offset + len);
        }
    }
    if (len == 0)
    {
        return;
    }
    size_t blockOffset = offset & (BLOCK_SIZE - 1);
    if (blockOffset != 0)
    {
        auto blk = loadDataBlock(sec, offset);
        size_t copyLen = std::min(BLOCK_SIZE - blockOffset, len);
        memcpy(blk + blockOffset, buf, copyLen);
        offset += copyLen;
        buf = (char *)buf + copyLen;
        len -= copyLen;
    }
    while (len > BLOCK_SIZE)
    {
        auto tryBlockNum = detail::upperBlockNum(len);

        auto [blk, realBlockNum] = loadDataBlock(sec, offset, tryBlockNum);
        if (realBlockNum == tryBlockNum)
        {
            memcpy(blk, buf, len);
            return;
        }
        size_t copyLen = BLOCK_SIZE * realBlockNum;
        memcpy(blk, buf, copyLen);
        offset += copyLen;
        buf = (char *)buf + copyLen;
        len -= copyLen;
    }
    if (len > 0)
    {
        auto blk = loadDataBlock(sec, offset);
        memcpy(blk, buf, len);
    }
}

void BlockManager::writeDataSectionAtEnd(DataSectionDescBlock *sec, const void *buf, size_t len)
{
    writeDataSection(sec, -1, buf, len);
}

size_t pse::fsl::BlockManager::allocateAtDataSection(DataSectionDescBlock *sec, size_t len)
{
    std::atomic_ref ref(sec->sectionSize);
    return ref.fetch_add(len);
}
size_t BlockManager::readDataSection(DataSectionDescBlock *sec, size_t offset, void *buf, size_t len)
{
    size_t validLen = sec->sectionSize - offset;
    size_t realReadLen = std::min(len, validLen);
    size_t res = realReadLen;

    size_t blockOffset = offset & (BLOCK_SIZE - 1);
    if (blockOffset != 0)
    {
        auto blk = (char *)loadDataBlock(sec, offset);
        size_t copyLen = std::min(BLOCK_SIZE - blockOffset, realReadLen);
        memcpy(buf, blk + blockOffset, copyLen);
        offset += copyLen;
        buf = (char *)buf + copyLen;
        realReadLen -= copyLen;
    }
    while (realReadLen > BLOCK_SIZE)
    {
        auto blk = loadDataBlock(sec, offset);
        size_t copyLen = std::min(BLOCK_SIZE, realReadLen);
        memcpy(buf, blk, copyLen);
        offset += copyLen;
        buf = (char *)buf + copyLen;
        realReadLen -= copyLen;
    }

    if (realReadLen > 0)
    {
        auto blk = loadDataBlock(sec, offset);
        size_t copyLen = realReadLen;
        memcpy(buf, blk, copyLen);
    }
    return res;
}

DirSectionDescBlock::Entry *BlockManager::findEntry(DirSectionDescBlock *sec, Block::desc_t desc, bool create)
{
    for (size_t i = 0; i < DirSectionDescBlock::ENTRY_NUM; ++i)
    {
        if (sec->direct[i].desc == desc)
        {
            return &(sec->direct[i]);
        }
        if (sec->direct[i].desc == 0 && create)
        {
            sec->direct[i].desc = desc;
            sec->direct[i].blockId = 0;
            return &(sec->direct[i]);
        }
        else if (sec->direct[i].desc == 0)
        {
            return nullptr;
        }
    }
    spdlog::debug("direct entry is full");
    // TODO: Add indirect block support.

    if (sec->indirects[0])
    {
        auto res = findEntry(Block::cast<DescIndirectBlock>(driver.loadBlock(sec->indirects[0])), desc, create);
        if (res)
        {
            return res;
        }
    }
    else
    {
        if (create)
        {
            auto [ptr, id] = allocate<DescIndirectBlock>();
            sec->indirects[0] = id;
            return findEntry(ptr, desc, create);
        }
    }
    spdlog::debug("level 1 is full");

    for (size_t i = 2; i < DirSectionDescBlock::LEVELS_ENTRY_SUM.size(); ++i)
    {
        if (sec->indirects[i - 1])
        {
            auto ptr = Block::dyn_cast<NoDescIndirectBlock>(driver.loadBlock(sec->indirects[i - 1]));
            auto res = findEntry(ptr, desc, i, create);
            if (res)
            {
                return res;
            }
        }
        else if (create)
        {
            auto [ptr, id] = allocate<NoDescIndirectBlock>();
            sec->indirects[i - 1] = id;
            return findEntry(ptr, desc, i, create);
        }
    }
    throw std::runtime_error("dir entry is full");
}

DirSectionDescBlock::Entry *BlockManager::findEntry(void *blk, Block::desc_t desc, int level, bool create)
{
    if (level == 1)
    {
        return findEntry(Block::cast<DescIndirectBlock>(blk), desc, create);
    }
    else
    {
        return findEntry(Block::cast<NoDescIndirectBlock>(blk), desc, level, create);
    }
}
DirSectionDescBlock::Entry *BlockManager::findEntry(DescIndirectBlock *blk, Block::desc_t desc, bool create)
{
    for (auto &entry : blk->direct)
    {
        if (entry.desc == desc)
        {
            return &entry;
        }
        else if (entry.desc == 0)
        {
            return create ? &entry : nullptr;
        }
    }
    return nullptr;
}

DirSectionDescBlock::Entry *
BlockManager::findEntry(NoDescIndirectBlock *blk, Block::desc_t desc, int level, bool create)
{
    assert(level >= 2);
    // TODO: add create new entry function
    assert(blk);
    for (auto &blockId : blk->blockIds)
    {
        if (blockId == 0)
        {
            if (!create)
            {
                return nullptr;
            }
            auto indirectBlockPtr = allocateIndirectBlock(blockId, level - 1);
            return indirectBlockPtr->direct;
        }
        void *subBlock = driver.loadBlock(blockId);
        if (level == 2)
        {
            auto res = findEntry(Block::cast<DescIndirectBlock>(subBlock), desc, create);
            if (res)
            {
                return res;
            }
        }
        else
        {
            auto res = findEntry(Block::cast<NoDescIndirectBlock>(subBlock), desc, level - 1, create);
            if (res)
            {
                return res;
            }
        }
    }
    return nullptr;
}

DescIndirectBlock *BlockManager::allocateIndirectBlock(Block::block_id_t &blockId, Block::desc_t level)
{
    if (level == 1)
    {
        auto [ptr, id] = allocate<DescIndirectBlock>();
        blockId = id;
        return ptr;
    }

    auto [ptr, id] = allocate<NoDescIndirectBlock>();
    blockId = id;
    return allocateIndirectBlock(ptr->blockIds[0], level - 1);
}
char *BlockManager::allocateDataBlock(Block::block_id_t &blockId, Block::desc_t level)
{
    if (level == 0)
    {
        auto [ptr, id] = allocateDataBlock();
        blockId = id;
        return (char *)ptr;
    }
    auto [ptr, id] = allocate<NoDescIndirectBlock>();
    blockId = id;
    return allocateDataBlock(ptr->blockIds[0], level - 1);
}

Block::block_id_t *BlockManager::getDataBlockId(NoDescIndirectBlock *sec, size_t entry_id, int level)
{
    if (level == 1)
    {
        return &sec->blockIds[entry_id];
    }
    else
    {
        auto topEntryId = entry_id / detail::power(NoDescIndirectBlock::ENTRY_NUM, level - 1);
        auto subEntryId = entry_id % detail::power(NoDescIndirectBlock::ENTRY_NUM, level - 1);
        spdlog::debug("top entry id: {}; sub entry id: {}", topEntryId, subEntryId);
        if (!sec->blockIds[topEntryId])
        {
            auto [ptr, id] = allocate<NoDescIndirectBlock>();
            sec->blockIds[topEntryId] = id;
            return getDataBlockId(ptr, subEntryId, level - 1);
        }
        else
        {
            auto ptr = Block::cast<NoDescIndirectBlock>(driver.loadBlock(sec->blockIds[topEntryId]));
            return getDataBlockId(ptr, subEntryId, level - 1);
        }
    }
}
Block::block_id_t *BlockManager::getDataBlockId(DataSectionDescBlock *sec, size_t offset)
{
    Block::block_id_t entry_id = offset / BLOCK_SIZE;
    if (entry_id < DataSectionDescBlock::ENTRY_NUM)
    {
        return &sec->direct[entry_id];
    }
    for (auto i = 0; auto num : DataSectionDescBlock::LEVELS_ENTRY_SUM)
    {
        if (entry_id < num)
        {
            spdlog::debug("create data block entry at level {} for {}", i, entry_id);
            if (!sec->indirects[i - 1])
            {
                auto [ptr, id] = allocate<NoDescIndirectBlock>();
                sec->indirects[i - 1] = id;
                return getDataBlockId(ptr, entry_id - DataSectionDescBlock::LEVELS_ENTRY_SUM[i - 1], i);
            }
            else
            {
                auto ptr = Block::cast<NoDescIndirectBlock>(driver.loadBlock(sec->indirects[i - 1]));
                auto res = getDataBlockId(ptr, entry_id - DataSectionDescBlock::LEVELS_ENTRY_SUM[i - 1], i);
                if (res)
                {
                    return res;
                }
            }
        }
        i++;
    }
    throw std::runtime_error("data block entry is full");
    return nullptr;
}
char *BlockManager::loadDataBlock(DataSectionDescBlock *sec, size_t offset)
{
    // spdlog::debug("load data block for offset {}", offset);

    auto entry = getDataBlockId(sec, offset);
    if (*entry == 0)
    {
        auto [dataPtr, dataBlockId] = allocateDataBlock();
        *entry = dataBlockId;
        return (char *)dataPtr;
    }
    else
    {
        return (char *)driver.loadBlock(*entry);
    }

    return nullptr;
}

std::pair<char *, size_t> BlockManager::loadDataBlock(DataSectionDescBlock *sec, size_t offset, size_t blockNum)
{
    utils::SpinLock lock(sec->atomic_lock);
    std::lock_guard guard(lock);
    auto entry = getDataBlockId(sec, offset);
    if (*entry == 0)
    {
        auto [dataPtr, dataBlockId, continuousNum] = allocateDataBlock(blockNum);
        *entry = dataBlockId;
        for (size_t i = 1; i < blockNum; ++i)
        {
            auto _entry = getDataBlockId(sec, offset + BLOCK_SIZE * i);
            *_entry = dataBlockId + i;
        }
        return {(char *)dataPtr, continuousNum};
    }

    auto resPtr = loadDataBlock(sec, offset);
    for (size_t i = 1; i < blockNum; ++i)
    {
        auto next_entry = getDataBlockId(sec, offset + i * BLOCK_SIZE);
        if (*next_entry != *entry + i)
        {
            return {resPtr, i};
        }
    }
    return {resPtr, blockNum};
}
} // namespace fsl
} // namespace pse