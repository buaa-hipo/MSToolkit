#pragma once
#include <ral/backend.h>
#include <magic_enum.hpp>
namespace pse
{
namespace ral
{

inline DirSection pse::ral::DirSection::openDir(desc_t desc, bool create)
{
    return _backend->openDirSection(*this, desc, create);
}
template <typename record_t>
inline DataSection<record_t> pse::ral::DirSection::openData(std::string_view sectionName, bool create)
{
    desc_t desc = secMap.get<record_t>(sectionName, create);
    if (!desc)
    {
        return {};
    }
    return _backend->openDataSection<record_t>(*this, desc);
}
template <typename record_t>
inline DataSection<record_t> pse::ral::DirSection::openData(bool create)
{
    desc_t desc = secMap.get<record_t>(create);
    if (!desc)
    {
        return {};
    }
    return _backend->openDataSection<record_t>(*this, desc, create);
}
#ifdef RAL_TYPE_REGISTER_H
template <RecordEnum_t e>
inline auto DirSection::openDataStatic(bool create)
{
    using record_t = typename EnumRegister<e>::record_t;
    return _backend->openDataSection<record_t>(*this, (desc_t)e + INIT_STATIC_ID, create);
}
template <RecordEnum_t e>
inline auto DirSection::openDataStaticUnique(bool create)
{
    using record_t = typename EnumRegister<e>::record_t;
    return std::make_unique<DataSection<record_t>>(
        _backend->openDataSection<record_t>(*this, (desc_t)e + INIT_STATIC_ID, create));
}
// template <typename ENUM>
// inline auto DirSection::openDataStatic(std::string_view sectionName, bool create)
// {
//     static_assert(std::is_same_v<ENUM, RecordEnum_t>);
//     using record_t = typename EnumRegister<RecordEnum_t, val>::record_t;
//     auto enum_val = magic_enum::enum_cast<ENUM>(sectionName);
//     if (!enum_val.has_value())
//     {
//         return {};
//     }
//     return _backend->openDataSection((desc_t)enum_val.value() + INIT_STATIC_ID, create);
// }
template <auto val>
std::unique_ptr<DataSectionBase> _openDataStaticUnique(DirSection &dir)
{
    return dir.openDataStaticUnique<val>(true);
}

#endif
} // namespace ral

} // namespace pse
