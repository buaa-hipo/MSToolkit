#pragma once
#include <string_view>
#include <source_location>
#include <span>
#include <boost/pfr.hpp>
#include <type_traits>
namespace pse
{

namespace utils
{

enum PrimitiveType
{
    INT8 = 0,
    INT16 = 1,
    INT32 = 2,
    INT64 = 3,
    FLOAT = 4,
    DOUBLE = 5,
};

template <typename T>
consteval int primitiveTypeEncode()
{
    if constexpr (std::is_integral_v<T>)
    {
        if constexpr (sizeof(T) == 1)
        {
            return PrimitiveType::INT8;
        }
        else if constexpr (sizeof(T) == 2)
        {
            return PrimitiveType::INT16;
        }
        else if constexpr (sizeof(T) == 4)
        {
            return PrimitiveType::INT32;
        }
        else if constexpr (sizeof(T) == 8)
        {
            return PrimitiveType::INT64;
        }
    }
    else if constexpr (std::is_same_v<T, float>)
    {
        return PrimitiveType::FLOAT;
    }
    else if constexpr (std::is_same_v<T, double>)
    {
        return PrimitiveType::DOUBLE;
    }
    else if constexpr (std::is_pointer_v<T>)
    {
        return PrimitiveType::INT64;
    }
}

constexpr size_t primitiveTypeSize(int type)
{
    switch ((PrimitiveType)type)
    {
    case PrimitiveType::INT8:
        return 1;
    case PrimitiveType::INT16:
        return 2;
    case PrimitiveType::INT32:
        return 4;
    case PrimitiveType::INT64:
        return 8;
    case PrimitiveType::FLOAT:
        return 4;
    case PrimitiveType::DOUBLE:
        return 8;
    default:
        return 0;
    }
};

/**
 * @brief Get number of all primitive fields of a struct 
 * 
 * @tparam T record type
 * @return int
 */
template <typename T>
    requires std::is_aggregate_v<T>
consteval int getFieldNum()
{
    int counter = 0;
    boost::pfr::for_each_field(T{}, [&](const auto &field, std::size_t idx) {
        using field_t = std::remove_cvref_t<decltype(field)>;
        if constexpr (std::is_class_v<field_t>)
        {
            counter += getFieldNum<field_t>();
        }
        else
        {
            counter += 1;
        }
    });
    return counter;
}

/**
 * @brief Get summary of length of all primitive fields.
 * For nested struct, real field name is all level fields joined by "_"
 * @tparam T record type
 * @return std::size_t
 */
template <typename T>
    requires std::is_aggregate_v<T>
consteval std::size_t getAllFieldsLen()
{
    constexpr auto fieldNames = boost::pfr::names_as_array<T>();
    int len = 0;
    boost::pfr::for_each_field(T{}, [&](const auto &field, std::size_t idx) {
        using field_t = std::remove_cvref_t<decltype(field)>;
        if constexpr (std::is_class_v<field_t>)
        {
            len += getAllFieldsLen<field_t>() + getFieldNum<field_t>() * (1 + fieldNames[idx].size());
        }
        else
        {
            len += fieldNames[idx].size();
        }
    });
    return len;
}

/**
 * @brief Get the Table Desc for a record struct.
 * Get tuple of `fieldPtr`, `fieldBuf`, `fieldTypes`.
 * For a struct with N primitive types, fieldPtr is a std::array<size_t, N+1>,
 * fieldBuf is a std::array<char, L>, fieldTypes is a std::array<const char*, N>.
 * 
 * string_view from fieldBuf[fieldPtr[i]] to fieldBuf[fieldPtr[i+1]] is fieldName for i-th field.
 * fieldTypes[i] is a integer representing primitive type of i-th field.
 * @tparam T record struct
 * @return tuple<array<size_t>, array<char>, array<const char*>>
 */
template <typename T>
    requires std::is_aggregate_v<T>
consteval const auto getStructInfo()
{
    constexpr int N = getFieldNum<T>();
    constexpr auto fieldNames = boost::pfr::names_as_array<T>();
    // constexpr int fieldCount = getFieldNum<T>();
    constexpr size_t allFieldsLen = getAllFieldsLen<T>();
    std::array<int, N + 1> fieldPtr;
    fieldPtr[0] = 0;
    std::array<char, allFieldsLen> fieldBuf;
    std::array<int, N> fieldTypes;
    std::array<int, N> fieldOffsets;
    size_t offset = 0;
    size_t fieldOffset = 0;

    int counter = 1;
    constexpr T t{};

    boost::pfr::for_each_field(t, [&](const auto &field, auto _idx) {
        constexpr int idx = _idx.value;
        using field_t = std::remove_cvref_t<decltype(field)>;
        constexpr auto topFieldName = fieldNames[idx];

        if constexpr (std::is_class_v<field_t>)
        {
            auto [subFieldPtr, subFieldBuf, subFieldTypes, subFieldOffsets] = getStructInfo<field_t>();
            for (size_t i = 0; i < subFieldPtr.size() - 1; ++i)
            {
                std::copy(topFieldName.begin(), topFieldName.end(), fieldBuf.begin() + offset);
                offset += topFieldName.size();
                fieldBuf[offset] = '_';
                offset++;
                std::copy(subFieldBuf.begin() + subFieldPtr[i],
                          subFieldBuf.begin() + subFieldPtr[i + 1],
                          fieldBuf.begin() + offset);
                offset += subFieldPtr[i + 1] - subFieldPtr[i];
                fieldTypes[counter - 1] = subFieldTypes[i];
                fieldOffsets[counter - 1] = fieldOffset + subFieldOffsets[i];
                fieldPtr[counter++] = offset;
            }
            fieldOffset += sizeof(field_t);
        }
        else
        {
            fieldTypes[counter - 1] = primitiveTypeEncode<field_t>();
            std::copy(topFieldName.begin(), topFieldName.end(), fieldBuf.begin() + offset);
            offset += topFieldName.size();
            fieldOffsets[counter - 1] = fieldOffset;
            fieldPtr[counter++] = offset;
            fieldOffset += sizeof(field_t);
        }
    });
    return std::tuple{fieldPtr, fieldBuf, fieldTypes, fieldOffsets};
}

template <auto T>
struct namestr_helper
{
    consteval static std::string_view raw_value() { return std::source_location::current().function_name(); }
    consteval static std::string_view value()
    {
        constexpr auto raw_type = raw_value();

        constexpr auto i1 = raw_type.find_first_of("=");
        constexpr auto i2 = raw_type.find_first_of(";");
        static_assert(i1 != -1);
        static_assert(i2 != -1);
        static_assert(i1 + 2 <= i2);
        return std::string_view(raw_type.begin() + i1 + 2, raw_type.begin() + i2);
        // return raw_type;
    }
};

template <typename T>
struct typestr_helper
{
    consteval static std::string_view raw_value() { return std::source_location::current().function_name(); }
    consteval static std::string_view value()
    {
        constexpr auto raw_type = raw_value();

        constexpr auto i1 = raw_type.find_first_of("=");
        constexpr auto i2 = raw_type.find_first_of(";");
        static_assert(i1 != -1);
        static_assert(i2 != -1);
        static_assert(i1 + 2 <= i2);
        return std::string_view(raw_type.begin() + i1 + 2, raw_type.begin() + i2);
        // return raw_type;
    }
};
template <typename T>
constexpr inline std::string_view typesv = typestr_helper<std::remove_cvref_t<T>>::value();
template <auto val>
constexpr inline std::string_view namesv = namestr_helper<val>::value();

template <typename T>
struct DetectField
{
    consteval static bool has_field(std::string_view fieldName)
    {
        constexpr auto fieldNames = boost::pfr::names_as_array<T>();
        bool has = false;
        boost::pfr::for_each_field(T{}, [&](const auto &field, auto _idx) {
            constexpr int idx = _idx.value;
            if (fieldNames[idx] == fieldName)
            {
                has = true;
            }
        });
        return has;
    }
    constexpr static int get_field_idx(std::string_view fieldName)
    {
        constexpr auto fieldNames = boost::pfr::names_as_array<T>();
        int ret_idx = -1;
        boost::pfr::for_each_field(T{}, [&](const auto &field, auto _idx) {
            constexpr int idx = _idx.value;
            if (fieldNames[idx] == fieldName)
            {
                ret_idx = idx;
            }
        });
        return ret_idx;
    }
};

template <typename T, auto FieldNameFn>
struct GetFieldHelper
{
    consteval static bool has_field()
    {
        constexpr auto fieldName = FieldNameFn();
        constexpr auto pos = fieldName.find('.');
        constexpr std::string_view firstFieldName =
            (pos == std::string_view::npos) ? fieldName : fieldName.substr(0, pos);
        constexpr auto idx = DetectField<T>::get_field_idx(firstFieldName);
        if constexpr (idx == -1)
        {
            return false;
        }
        else
        {
            if constexpr (pos == std::string_view::npos)
            {
                return true;
            }
            else
            {
                constexpr std::string_view subFieldName = fieldName.substr(pos + 1);
                return GetFieldHelper<boost::pfr::tuple_element_t<idx, T>, [=]() { return subFieldName; }>::has_field();
            }
        }
    }
    static auto &get_field(T &t)
    {
        constexpr auto fieldName = FieldNameFn();
        constexpr auto pos = fieldName.find('.');
        constexpr std::string_view firstFieldName =
            (pos == std::string_view::npos) ? fieldName : fieldName.substr(0, pos);
        constexpr auto idx = DetectField<T>::get_field_idx(firstFieldName);
        static_assert(idx != -1);
        if constexpr (pos == std::string_view::npos)
        {
            return boost::pfr::get<idx>(t);
        }
        else
        {
            constexpr std::string_view subFieldName = fieldName.substr(pos + 1);
            return GetFieldHelper<boost::pfr::tuple_element_t<idx, T>, [=]() { return subFieldName; }>::get_field(
                boost::pfr::get<idx>(t));
        }
    }
    static auto get_field_v(const T &t) { return get_field(const_cast<T &>(t)); }
};

template <typename T, auto FieldNameFn, auto... OtherFieldNameFn>
struct GetField
{
    consteval static bool has_field()
    {
        return GetFieldHelper<T, FieldNameFn>::has_field() || (GetFieldHelper<T, OtherFieldNameFn>::has_field() || ...);
    }
    static auto &get_field(T &t)
    {
        static_assert(has_field(), "Field not found");
        if constexpr (GetFieldHelper<T, FieldNameFn>::has_field())
        {
            return GetFieldHelper<T, FieldNameFn>::get_field(t);
        }
        else
        {
            return GetField<T, OtherFieldNameFn...>::get_field(t);
        }
    }
    static auto get_field_v(const T &t)
    {
        static_assert(has_field(), "Field not found");
        if constexpr (GetFieldHelper<T, FieldNameFn>::has_field())
        {
            return GetFieldHelper<T, FieldNameFn>::get_field_v(t);
        }
        else
        {
            return GetField<T, OtherFieldNameFn...>::get_field_v(t);
        }
    }
    static auto get_field_v_or(const T &t, auto val)
    {
        if constexpr (has_field())
        {
            if constexpr (GetFieldHelper<T, FieldNameFn>::has_field())
            {
                return GetFieldHelper<T, FieldNameFn>::get_field_v(t);
            }
            else
            {
                return GetField<T, OtherFieldNameFn...>::get_field_v(t);
            }
        }
        else
        {
            return val;
        }
    }
};
#define PSE_VALUE_WRAP(sv) []() { return sv; }

#define PSE_GETFIELD_V(t, sv)                                                                                          \
    pse::utils::GetField<std::remove_cvref_t<decltype(t)>, []() { return sv; }>::get_field_v(t)
#define PSE_GETFIELD_V_OR(t, sv, val)                                                                                  \
    pse::utils::GetField<std::remove_cvref_t<decltype(t)>, []() { return sv; }>::get_field_v_or(t, val)
#define PSE_GETFIELD(t, sv) pse::utils::GetField<std::remove_cvref_t<decltype(t)>, []() { return sv; }>::get_field(t)
#define PSE_GETFIELD_V2(t, ...) pse::utils::GetField<std::remove_cvref_t<decltype(t)>, __VA_ARGS__>::get_field_v(t)

struct EncodedStruct
{
    const std::span<const int> field_ptrs;
    const std::span<const int> field_types;
    const std::span<const int> field_offsets;
    const std::string_view field_names;
    const int variable_field_offset;
    const unsigned int size;
    constexpr int get_field_type(int idx) const { return field_types[idx]; }
    constexpr std::string_view get_field_name(int idx) const
    {
        int start = field_ptrs[idx];
        int end = field_ptrs[idx + 1];
        return field_names.substr(start, end - start);
    }
    constexpr int get_field_count() const { return field_ptrs.size() - 1; }
    EncodedStruct add_size(unsigned int size) const { return {field_ptrs, field_types, field_offsets, field_names, variable_field_offset, this->size + size}; }
};
template <typename T>
struct StructEncoder
{
    static constexpr auto info = getStructInfo<T>();
    static consteval std::span<const int> get_field_ptrs()
    {
        return std::span<const int>(std::get<0>(info).data(), std::get<0>(info).size());
    }
    static consteval std::span<const int> get_field_types()
    {
        return std::span<const int>(std::get<2>(info).data(), std::get<2>(info).size());
    }
    static consteval std::span<const int> get_field_offsets()
    {
        return std::span<const int>(std::get<3>(info).data(), std::get<3>(info).size());
    }
    static consteval std::string_view get_field_names()
    {
        return std::string_view(std::get<1>(info).data(), std::get<1>(info).size());
    }
    static constexpr EncodedStruct get_encoded_struct(unsigned variable_field_length = 0)
    {
        return {get_field_ptrs(),
                get_field_types(),
                get_field_offsets(),
                get_field_names(),
                sizeof(T),
                (unsigned)sizeof(T) + variable_field_length};
    };
};

} // namespace utils

} // namespace pse
