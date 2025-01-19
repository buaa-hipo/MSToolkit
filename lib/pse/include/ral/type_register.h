#ifndef RAL_TYPE_REGISTER_H
#define RAL_TYPE_REGISTER_H

#pragma once
#include <type_traits>
#include <functional>
#include <unordered_map>
#include <memory>
namespace pse
{

namespace ral
{

#ifndef RAL_DECLARE_RECORD_ENUM
#define RAL_DECLARE_RECORD_ENUM(TYPE)                                                                                  \
    class DataSectionBase;                                                                                             \
    class DirSection;                                                                                                  \
    using RecordEnum_t = TYPE;                                                                                         \
    template <auto val>                                                                                                \
    class EnumRegister;                                                                                                \
    inline std::unordered_map<RecordEnum_t,                                                                            \
                              std::function<std::unique_ptr<pse::ral::DataSectionBase>(pse::ral::DirSection & dir)>>   \
        RecordEnumMap;
#endif
class DataSectionBase;
class DirSection;
template <auto val>
std::unique_ptr<DataSectionBase> _openDataStaticUnique(DirSection &dir);

#ifndef RAL_REGISTER_ENUM
#define RAL_REGISTER_ENUM(ENUM_VAL, RECORD_T)                                                                          \
    template <>                                                                                                        \
    class EnumRegister<ENUM_VAL>                                                                                       \
    {                                                                                                                  \
    public:                                                                                                            \
        static constexpr decltype(ENUM_VAL) value = ENUM_VAL;                                                          \
        using type = decltype(ENUM_VAL);                                                                               \
        using record_t = RECORD_T;                                                                                     \
        EnumRegister()                                                                                                 \
        {                                                                                                              \
            RecordEnumMap[ENUM_VAL] = [](pse::ral::DirSection &dir) -> std::unique_ptr<pse::ral::DataSectionBase> {    \
                return pse::ral::_openDataStaticUnique<ENUM_VAL>(dir);                                                 \
            };                                                                                                         \
        }                                                                                                              \
        static EnumRegister<ENUM_VAL> instance;                                                                        \
    };                                                                                                                 \
    inline EnumRegister<ENUM_VAL> EnumRegister<ENUM_VAL>::instance;
#endif
} // namespace ral

} // namespace pse

#endif