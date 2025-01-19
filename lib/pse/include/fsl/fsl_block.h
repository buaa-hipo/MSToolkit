#pragma once
#include <stdint.h>
#include <assert.h>
#include <type_traits>
#include <stddef.h>
#include <string.h>
#include <array>
#include <exception>
#include <fmt/format.h>
#include <ral/section_declare.h>

namespace pse
{
namespace fsl
{

namespace detail
{
constexpr size_t power(size_t a, size_t b)
{
    size_t ret = 1;
    while (b)
    {
        if (b & 1)
        {
            ret *= a;
        }
        a *= a;
        b >>= 1;
    }
    return ret;
}

#pragma pack(push)
#pragma pack(1)
struct uint48_t
{
    uint16_t low;
    uint32_t high;

    uint48_t() = default;
    uint48_t(uint64_t val)
    : low(val & 0xffff)
    , high(val >> 16)
    {
    }
    operator uint64_t() const { return (uint64_t(high) << 16) | low; }
    uint48_t &operator=(uint64_t val)
    {
        low = val & 0xffff;
        high = val >> 16;
        return *this;
    }
    uint48_t operator+(uint64_t val)
    {
        uint48_t res(*this);
        res += val;
        return res;
    }
    uint48_t operator-(uint64_t val)
    {
        uint48_t res(*this);
        res -= val;
        return res;
    }
    uint48_t operator+(uint32_t val)
    {
        uint48_t res(*this);
        res += val;
        return res;
    }
    uint48_t operator-(uint32_t val)
    {
        uint48_t res(*this);
        res -= val;
        return res;
    }
    uint48_t operator+(int32_t val)
    {
        uint48_t res(*this);
        res += val;
        return res;
    }
    uint48_t operator-(int32_t val)
    {
        uint48_t res(*this);
        res -= val;
        return res;
    }
    uint48_t &operator+=(uint64_t val)
    {
        *this = uint64_t(*this) + val;
        return *this;
    }
    uint48_t &operator-=(uint64_t val)
    {
        *this = uint64_t(*this) - val;
        return *this;
    }
};
#pragma pack(pop)
static_assert(sizeof(uint48_t) == 6);
} // namespace detail

constexpr const static size_t PAGE_SIZE = 4096L << 3;
constexpr const static size_t BLOCK_SIZE = 512;
constexpr const static size_t BLOCKS_PER_PAGE = PAGE_SIZE / BLOCK_SIZE;

#define FSL_CHECK_BLOCK(BLOCK_TYPE)                                                                                    \
    static_assert(sizeof(BLOCK_TYPE) == BLOCK_SIZE, "Size of " #BLOCK_TYPE " type doesn't match");                     \
    static_assert(std::is_standard_layout_v<BLOCK_TYPE> && std::is_trivial_v<BLOCK_TYPE>,                              \
                  #BLOCK_TYPE " is not standard of trival");                                                           \
    static_assert(std::is_base_of_v<BlockBase<BLOCK_TYPE>, BLOCK_TYPE>,                                                \
                  #BLOCK_TYPE " is not child of BLOCK<" #BLOCK_TYPE ">");

/**
 * @brief Base class for import BlockType enum for all block implementation.
 *
 */
#pragma pack(push)
#pragma pack(1)
struct Block
{
    using desc_t = ral::desc_t;
    using block_id_t = detail::uint48_t;
    struct Entry
    {
        desc_t desc;
        block_id_t blockId;
    };

    enum BLOCK_TYPE_ENUM
    {
        SUPER = 1,
        B_PLUS,
        DATA_SECTION_DESC,
        DIR_SECTION_DESC,
        NO_DESC_INDIRECT,
        DESC_INDIRECT,

    };
    template <typename BLOCK_TYPE>
    static BLOCK_TYPE *cast(void *ptr)
    {
        auto res = reinterpret_cast<BLOCK_TYPE *>(ptr);
        if (res->blockTypeId != BLOCK_TYPE::BLOCK_TYPE_ID)
        {
            throw std::runtime_error(
                fmt::format("cast failed for block {:x}, block type id not match, expect {}, got {}",
                            (size_t)ptr,
                            BLOCK_TYPE::BLOCK_TYPE_ID,
                            res->blockTypeId));
        }
        return res;
    }
    template <typename BLOCK_TYPE>
    static bool isa(void *ptr)
    {
        auto res = reinterpret_cast<BLOCK_TYPE *>(ptr);
        return (res->blockTypeId == BLOCK_TYPE::BLOCK_TYPE_ID);
    }
    template <typename BLOCK_TYPE, typename BLOCK_TYPE2, typename... OTHER_BLOCK_TYPES>
    static bool isa(void *ptr)
    {
        return isa<BLOCK_TYPE>(ptr) || isa<BLOCK_TYPE2, OTHER_BLOCK_TYPES...>(ptr);
    }
    template <typename BLOCK_TYPE>
    static BLOCK_TYPE *dyn_cast(void *ptr)
    {
        auto res = reinterpret_cast<BLOCK_TYPE *>(ptr);
        return isa<BLOCK_TYPE>(ptr) ? res : nullptr;
    }
    template <typename BLOCK_TYPE>
    static BLOCK_TYPE *nullable_cast(void *ptr)
    {
        if (!ptr)
        {
            return nullptr;
        }
        return cast<BLOCK_TYPE>(ptr);
    }
};

/**
 * @brief CRTP base class for all block classes.
 *
 * @tparam BLOCK_TYPE
 */
template <typename BLOCK_TYPE>
struct BlockBase : public Block
{
    void init()
    {
        memset(this, 0, sizeof(BLOCK_TYPE));
        static_cast<BLOCK_TYPE *>(this)->blockTypeId = BLOCK_TYPE::BLOCK_TYPE_ID;
        static_cast<BLOCK_TYPE *>(this)->_init();
    }
};
struct SuperBlock : public BlockBase<SuperBlock>
{
    friend BlockBase<SuperBlock>;
    static constexpr int MAGIC = 0x114514;
    static constexpr int BLOCK_TYPE_ID = SUPER;

    int blockTypeId;
    int magic;
    int flag;
    int atomic_lock;
    size_t blockAllocated;
    block_id_t rootId;
    char padding[BLOCK_SIZE - 4 * sizeof(int) - sizeof(block_id_t) - sizeof(size_t)];

private:
    void _init();
};
FSL_CHECK_BLOCK(SuperBlock);

/**
 * @brief Indirect index block without description for search.
 */
struct NoDescIndirectBlock : public BlockBase<NoDescIndirectBlock>
{
    friend BlockBase<NoDescIndirectBlock>;
    constexpr static int BLOCK_TYPE_ID = NO_DESC_INDIRECT;
    constexpr static size_t ENTRY_NUM = (BLOCK_SIZE - sizeof(int) * 3) / sizeof(block_id_t) - 1;
    int blockTypeId;
    int flag;
    int level;
    block_id_t blockIds[ENTRY_NUM];
    char padding[BLOCK_SIZE - sizeof(block_id_t) * ENTRY_NUM - sizeof(int) * 3];

private:
    void _init() { }
};
FSL_CHECK_BLOCK(NoDescIndirectBlock);
/**
 * @brief Indirect index block with description for search.
 */
struct DescIndirectBlock : public BlockBase<DescIndirectBlock>
{
    friend BlockBase<DescIndirectBlock>;
    constexpr static int BLOCK_TYPE_ID = DESC_INDIRECT;
    constexpr static int ENTRY_NUM = (BLOCK_SIZE - 2 * sizeof(int)) / sizeof(Entry) - 1;

    int blockTypeId;
    int flag;
    Entry direct[ENTRY_NUM];
    char padding[BLOCK_SIZE - sizeof(direct) - 2 * sizeof(int)];

private:
    void _init() { }
};
FSL_CHECK_BLOCK(DescIndirectBlock);
/**
 * @brief Description block for a section.
 * @brief Data Section consists of some data blocks.
 * Sub-block of Directory Section must be a section block.
 * Level1 to level3 are blockIds for IndirectBlock, which can contain more entry for sub-blocks.
 */
struct DataSectionDescBlock : public BlockBase<DataSectionDescBlock>
{
    friend BlockBase<SuperBlock>;
    static constexpr int ENTRY_NUM = 105 * 4 / sizeof(block_id_t);
    static constexpr int BLOCK_TYPE_ID = DATA_SECTION_DESC;
    constexpr static size_t INDIRECT_LEVEL = 5;
    constexpr static size_t LEVEL1_ENTRY_SUM = ENTRY_NUM + NoDescIndirectBlock::ENTRY_NUM;
    constexpr static size_t LEVEL2_ENTRY_SUM = LEVEL1_ENTRY_SUM + detail::power(NoDescIndirectBlock::ENTRY_NUM, 2);
    constexpr static size_t LEVEL3_ENTRY_SUM = LEVEL2_ENTRY_SUM + detail::power(NoDescIndirectBlock::ENTRY_NUM, 3);
    constexpr static size_t LEVEL4_ENTRY_SUM = LEVEL3_ENTRY_SUM + detail::power(NoDescIndirectBlock::ENTRY_NUM, 4);
    constexpr static auto LEVELS_ENTRY_SUM = []() {
        std::array<size_t, INDIRECT_LEVEL + 1> res;
        res[0] = DataSectionDescBlock::ENTRY_NUM;

        for (size_t i = 1; i < res.size(); ++i)
        {
            res[i] = res[i - 1] + detail::power(NoDescIndirectBlock::ENTRY_NUM, i);
        }
        return res;
    }();
    constexpr static size_t MAX_DATA_SECTION_SIZE =
        LEVELS_ENTRY_SUM[LEVELS_ENTRY_SUM.size() - 1] * BLOCK_SIZE / 1024 / 1024 / 1024;
    int blockTypeId;
    int flag;
    int atomic_lock;
    int recordVariableLength;
    int dataSectionType;
    desc_t self_desc;
    size_t sectionSize;
    block_id_t index_block_id;
    block_id_t direct[ENTRY_NUM];
    block_id_t indirects[INDIRECT_LEVEL];
    char padding[(BLOCK_SIZE - sizeof(block_id_t) * (INDIRECT_LEVEL + 1 + ENTRY_NUM) - sizeof(int) * 5 -
                  sizeof(size_t) - sizeof(desc_t))];
    void _init() { }
};
FSL_CHECK_BLOCK(DataSectionDescBlock);
/**
 * @brief Directory Section consists of some sub-sections.
 * Sub-block of Directory Section must be a section block.
 * Level1 to level3 are blockIds for IndirectBlock, which can contain more entry for sub-blocks.
 */

struct DirSectionDescBlock : public BlockBase<DirSectionDescBlock>
{
    friend BlockBase<SuperBlock>;
    constexpr static size_t ENTRY_NUM = (BLOCK_SIZE - 3 * sizeof(int) - 3 * sizeof(block_id_t)) / sizeof(Entry) - 2;

    constexpr static size_t LEVEL1_ENTRY_SUM = ENTRY_NUM + DescIndirectBlock::ENTRY_NUM;
    constexpr static size_t LEVEL2_ENTRY_SUM =
        LEVEL1_ENTRY_SUM + NoDescIndirectBlock::ENTRY_NUM * DescIndirectBlock::ENTRY_NUM;
    constexpr static size_t LEVEL3_ENTRY_SUM =
        LEVEL2_ENTRY_SUM + detail::power(NoDescIndirectBlock::ENTRY_NUM, 2) * DescIndirectBlock::ENTRY_NUM;
    constexpr static auto LEVELS_ENTRY_SUM = []() {
        std::array<size_t, 4> res;
        res[0] = DirSectionDescBlock::ENTRY_NUM;

        for (int i = 1; i < 4; ++i)
        {
            res[i] = res[i - 1] + detail::power(NoDescIndirectBlock::ENTRY_NUM, i - 1) * DescIndirectBlock::ENTRY_NUM;
        }
        return res;
    }();

    constexpr static int BLOCK_TYPE_ID = DIR_SECTION_DESC;
    int blockTypeId;
    int atomic_lock;
    desc_t self_desc;
    Entry direct[ENTRY_NUM];
    block_id_t indirects[3];
    char padding[BLOCK_SIZE - sizeof(direct) - 3 * sizeof(block_id_t) - 2 * sizeof(int) - sizeof(desc_t)];
    void _init() { }

    static_assert(LEVELS_ENTRY_SUM[0] == ENTRY_NUM);
    static_assert(LEVELS_ENTRY_SUM[1] == LEVEL1_ENTRY_SUM);
    static_assert(LEVELS_ENTRY_SUM[2] == LEVEL2_ENTRY_SUM);
    static_assert(LEVELS_ENTRY_SUM[3] == LEVEL3_ENTRY_SUM);
};
FSL_CHECK_BLOCK(DirSectionDescBlock);

#undef FSL_CHECK_BLOCK
#pragma pack(pop)

} // namespace fsl

} // namespace pse

namespace fmt
{
template <>
struct formatter<pse::fsl::detail::uint48_t>
{
    constexpr auto parse(format_parse_context &ctx) -> format_parse_context::iterator
    {
        auto it = ctx.begin(), end = ctx.end();

        // Check if reached the end of the range:
        if (it != end && *it != '}')
        {
            fmt::detail::throw_format_error("invalid format");
        }

        // Return an iterator past the end of the parsed range:
        return it;
    }

    auto format(const pse::fsl::detail::uint48_t &p, fmt::format_context &ctx) const -> fmt::format_context::iterator
    {
        return fmt::format_to(ctx.out(), "{:x}", uint64_t(p));
    }
};

} // namespace fmt