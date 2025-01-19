#pragma once
#include <variant>
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <unordered_map>
#include <memory>
#include <atomic>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fsl/fsl_block.h>
// #include <fsl/fsl_section.h>
#include <spdlog/spdlog.h>

#include <utils/compile_time.h>
#include <utils/lock.h>

namespace pse
{
namespace fsl
{

class BlockManager;
namespace detail
{
inline size_t blockId2Offset(size_t id) { return id * BLOCK_SIZE; }
inline size_t offset2BlockOffset(size_t offset) { return offset % BLOCK_SIZE; }
inline size_t blockIdInPage(size_t blockOffset) { return blockOffset % PAGE_SIZE / BLOCK_SIZE; }
inline size_t upperBlockNum(size_t len) { return (len + BLOCK_SIZE - 1) / BLOCK_SIZE; }

} // namespace detail

class DirSectionIterator;
class FileSectionLayerDriver
{
    friend class DirSectionIterator;
    static constexpr size_t DEFAULT_INIT_MMAP_SIZE = 1L << 40;
    char *mmap_ptr;
    std::atomic<size_t> mapedEnd{0};
    friend BlockManager;
    std::string _filename;
    std::unordered_map<size_t, void *> id2ptr;
    std::unordered_map<void *, size_t> ptr2id;
    int fd;
    size_t fileSize();
    std::atomic_ref<size_t> *blockAllocated;
    utils::SpinLock<int> *driver_atomic_lock;
    // Block::block_id_t blockAllocated;
    void preserve(size_t len);
    void *load(size_t offset);
    void unload(void *ptr);
    void *loadBlock(size_t blockId);
    std::pair<void *, Block::block_id_t> allocateBlock();
    std::tuple<void *, Block::block_id_t, size_t> allocateBlock(size_t blockNum);
    void clear();
    bool blockExists(size_t blockId);
    void *tryLoad(size_t offset);
    bool is_creator;

public:
    FileSectionLayerDriver(std::string_view filename);
    ~FileSectionLayerDriver();

    static size_t pageAlign(size_t val) { return (val & (~(PAGE_SIZE - 1))); }
};

class BlockManager : public std::enable_shared_from_this<BlockManager>
{
    friend class DirSectionIterator;
    FileSectionLayerDriver driver;
    int _mode;
    SuperBlock *super;
    DirSectionDescBlock *root;

    BlockManager(const BlockManager &) = delete;
    BlockManager &operator=(const BlockManager &) = delete;

public:
    enum
    {
        NO_CREATE = 0,
        CREATE_DIR_SECTION,
        CREATE_DATA_SECTION
    };

    bool isLeader() { return driver.is_creator; }

    BlockManager(std::string_view name, ral::RWMode mode);
    DirSectionDescBlock *openRootSection();

    /**
     * @brief Open sub-section in an directory section.
     *
     * @param parent
     * @param desc
     * @param createMode
     * @param force ignore atomic lock
     * @param recordVariableLength variable pmu size for record
     * NO_CREATE: do not create new section;
     * CREATE_DIR_SECTION: create new directory section;
     * CREATE_DATA_SECTION: create new data section;
     * @return void*
     */
    void *
    openSection(DirSectionDescBlock *parent, Block::desc_t desc, int createMode, bool force, int recordVariableLength);
    DirSectionDescBlock *openDirSection(DirSectionDescBlock *sec, Block::desc_t desc, bool create, bool force = false);
    DataSectionDescBlock *openDataSection(
        DirSectionDescBlock *sec, Block::desc_t desc, bool create, bool force = false, int recordVariableLength = 0);

    /**
     * @brief write data to a data section
     * If offset is -1, than write data at the end of data section.
     * @param sec data section
     * @param offset offset of data section to write
     * @param buf data for write
     * @param len length of data for write
     */
    void writeDataSection(DataSectionDescBlock *sec, size_t offset, const void *buf, size_t len);
    void writeDataSectionAtEnd(DataSectionDescBlock *sec, const void *buf, size_t len);
    size_t allocateAtDataSection(DataSectionDescBlock *sec, size_t len);
    size_t readDataSection(DataSectionDescBlock *sec, size_t offset, void *buf, size_t len);

private:
    /**
     * @brief find sub-section in directory section.
     * return value entry_ptr
     * If desc exists, entry_ptr is the pointer to that entry;
     * if desc doesn't exists and create is true, entry_ptr is the pointer to a new entry.
     * if desc doesn't exists and create is false, entry_ptr is nullptr.
     * @param sec section block ptr
     * @param desc description of sub-section
     * @param create whether create new entry if not exist.
     * @return  DirSectionDescBlock::Entry* entry_ptr
     */
    DirSectionDescBlock::Entry *findEntry(DirSectionDescBlock *sec, Block::desc_t desc, bool create);

    /**
     * @brief Find sub-section in indirect blocks consisting of sub-block descriptions and blockIds.
     *
     * @param blk
     * @param desc
     * @return DirSectionDescBlock::Entry*
     */
    DirSectionDescBlock::Entry *findEntry(void *blk, Block::desc_t desc, int level, bool create);
    DirSectionDescBlock::Entry *findEntry(DescIndirectBlock *blk, Block::desc_t desc, bool create);

    /**
     * @brief Find sub-section in indirect blocks consisting of other indirect blocks.
     *
     * @param blk
     * @param desc
     * @return DirSectionDescBlock::Entry*
     */
    DirSectionDescBlock::Entry *findEntry(NoDescIndirectBlock *blk, Block::desc_t desc, int level, bool create);
    DescIndirectBlock *allocateIndirectBlock(Block::block_id_t &blockId, Block::desc_t level);
    char *allocateDataBlock(Block::block_id_t &blockId, Block::desc_t level);
    Block::block_id_t *getDataBlockId(NoDescIndirectBlock *sec, size_t offset, int level);
    Block::block_id_t *getDataBlockId(DataSectionDescBlock *sec, size_t offset);

    /**
     * @brief Load data block contains data at offset for a data section.
     *
     * @param sec
     * @param offset data offset at data section
     * @return char* data block
     */
    char *loadDataBlock(DataSectionDescBlock *sec, size_t offset);
    std::pair<char *, size_t> loadDataBlock(DataSectionDescBlock *sec, size_t offset, size_t blockNum);

    /**
     * @brief Allocate a new block safely
     *
     * @tparam T Block type
     * @return T*
     */
    template <typename T>
    std::pair<T *, Block::block_id_t> allocate()
    {
        auto [_ptr, id] = allocateRaw();
        T *ptr = (T *)_ptr;
        ptr->init();
        spdlog::debug("allocate block type {} at {}", utils::namesv<(Block::BLOCK_TYPE_ENUM)T::BLOCK_TYPE_ID>, id);
        assert(id > 1);
        return {ptr, id};
    }
    std::pair<void *, Block::block_id_t> allocateRaw()
    {
        auto [_ptr, id] = driver.allocateBlock();
        return {_ptr, id};
    }
    std::tuple<void *, Block::block_id_t, size_t> allocateRaw(size_t num)
    {
        auto [_ptr, id, continuousNum] = driver.allocateBlock(num);
        return {_ptr, id, continuousNum};
    }
    std::pair<void *, Block::block_id_t> allocateDataBlock()
    {
        auto [_ptr, id] = allocateRaw();
        spdlog::debug("allocate {} data block at {}", 1, id);
        return {_ptr, id};
    }
    std::tuple<void *, Block::block_id_t, size_t> allocateDataBlock(size_t num)
    {
        auto [_ptr, id, continuousNum] = allocateRaw(num);
        spdlog::debug("allocate {} data block at {} with {} continuous block", num, id, continuousNum);
        return {_ptr, id, continuousNum};
    }

public:
    static std::shared_ptr<BlockManager> create(std::string_view name, ral::RWMode mode)
    {
        return std::make_shared<BlockManager>(name, mode);
    }

    size_t getDataSectionSize(DataSectionDescBlock *sec) { return sec->sectionSize; }
};

class DirSectionIterator
{
    int idx;
    DirSectionDescBlock *sec;
    std::shared_ptr<BlockManager> manager;

public:
    std::shared_ptr<BlockManager> get_manager() { return manager; }
    DirSectionIterator(int idx, DirSectionDescBlock *sec, std::shared_ptr<BlockManager> manager)
    : idx(idx)
    , sec(sec)
    , manager(manager)
    {
    }
    DirSectionIterator &operator++()
    {
        idx++;
        return *this;
    }
    DirSectionIterator operator++(int)
    {
        auto res = *this;
        idx++;
        return res;
    }
    ral::desc_t getDesc() const { return get()->desc; }

    DirSectionDescBlock::Entry *get() const
    {
        if (idx < DirSectionDescBlock::ENTRY_NUM)
        {
            return &sec->direct[idx];
        }
        if (idx < DirSectionDescBlock::LEVEL1_ENTRY_SUM)
        {
            if (sec->indirects[0])
            {
                auto indirect0 = Block::cast<DescIndirectBlock>(manager->driver.loadBlock(sec->indirects[0]));
                auto res = &indirect0->direct[idx - DirSectionDescBlock::ENTRY_NUM];
                if (res->desc)
                {
                    return res;
                }
                else
                {
                    return nullptr;
                }
            }
            else
            {
                return nullptr;
            }
        }

        if (idx < DirSectionDescBlock::LEVEL2_ENTRY_SUM)
        {
            if (sec->indirects[1])
            {
                auto indirect1 = Block::cast<NoDescIndirectBlock>(manager->driver.loadBlock(sec->indirects[1]));
                auto idx1 = idx - DirSectionDescBlock::LEVEL1_ENTRY_SUM;
                auto idx1_1 = idx1 / DescIndirectBlock::ENTRY_NUM;
                if (indirect1->blockIds[idx1_1] != 0)
                {
                    auto indirect0 =
                        Block::cast<DescIndirectBlock>(manager->driver.loadBlock(indirect1->blockIds[idx1_1]));
                    auto res = &indirect0->direct[idx1 % DescIndirectBlock::ENTRY_NUM];
                    if (res->desc)
                    {
                        return res;
                    }
                    else
                    {
                        return nullptr;
                    }
                }
            }
            return nullptr;
        }
        if (idx < DirSectionDescBlock::LEVEL3_ENTRY_SUM)
        {
            if (sec->indirects[2])
            {
                auto indirect2 = Block::cast<NoDescIndirectBlock>(manager->driver.loadBlock(sec->indirects[2]));
                auto idx2 = idx - DirSectionDescBlock::LEVEL2_ENTRY_SUM;
                auto idx2_1 = idx2 / (NoDescIndirectBlock::ENTRY_NUM * DescIndirectBlock::ENTRY_NUM);
                auto idx2_2 = idx2 % (NoDescIndirectBlock::ENTRY_NUM * DescIndirectBlock::ENTRY_NUM);
                if (indirect2->blockIds[idx2_1] != 0)
                {
                    auto indirect1 =
                        Block::cast<NoDescIndirectBlock>(manager->driver.loadBlock(indirect2->blockIds[idx2_1]));
                    auto idx1 = idx2_2 / DescIndirectBlock::ENTRY_NUM;
                    if (indirect1->blockIds[idx1] != 0)
                    {
                        auto indirect0 =
                            Block::cast<DescIndirectBlock>(manager->driver.loadBlock(indirect1->blockIds[idx1]));
                        auto res = &indirect0->direct[idx2_2 % DescIndirectBlock::ENTRY_NUM];
                        if (res->desc)
                        {
                            return res;
                        }
                        else
                        {
                            return nullptr;
                        }
                    }
                }
            }
            return nullptr;
        }
        return nullptr;
    }

    Block *operator*() const
    {
        if (!sec)
        {
            return nullptr;
        }
        auto entry = get();
        if (entry && entry->desc)
        {
            // spdlog::info("entry desc: {}; blockId: {}", entry->desc, entry->blockId);
            return static_cast<Block *>(manager->driver.loadBlock(entry->blockId));
        }
        return nullptr;
    }
};
} // namespace fsl
} // namespace pse