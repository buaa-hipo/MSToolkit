#pragma once
namespace pse
{

namespace ral
{
using desc_t = int;
constexpr desc_t INIT_ALLOCATE_ID = 10000000;
constexpr desc_t INIT_STATIC_ID = 20000000;
// In linux, PID_MAX_LIMIT is 1<<22, thus we reserve id larger than 1<<22 for internal use.
constexpr desc_t INIT_RESERVED_ID = (1 << 22) + 1;
static_assert(INIT_ALLOCATE_ID > INIT_RESERVED_ID);
static_assert(INIT_STATIC_ID > INIT_ALLOCATE_ID);

enum class ReservedSection
{
    // a section store a map from string to desc_t
    SECTION_MAP = INIT_RESERVED_ID,
    SECTION_MAP_STRING_STORE,
};
enum RWMode
{
    READ = 1,
    WRITE = 2,
    APPEND = 4
};

template <typename record_t>
class SectionIteratorBase;

class DuckTypeSectionIteratorBase;

} // namespace ral

} // namespace pse
