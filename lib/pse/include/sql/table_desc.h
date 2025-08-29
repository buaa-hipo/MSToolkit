#pragma once
#include <type_traits>
#include <string.h>
#include <boost/pfr.hpp>
#include <assert.h>
#include <spdlog/spdlog.h>
#include <typeindex>
namespace pse
{

namespace TableDesc
{

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
 * fieldTypes[i] is SQL type of i-th field.
 * @tparam T record struct
 * @return tuple<array<size_t>, array<char>, array<const char*>>
 */
template <typename T>
    requires std::is_aggregate_v<T>
consteval const auto getTableDesc()
{
    constexpr int N = getFieldNum<T>();
    constexpr auto fieldNames = boost::pfr::names_as_array<T>();
    // constexpr int fieldCount = getFieldNum<T>();
    constexpr size_t allFieldsLen = getAllFieldsLen<T>();
    std::array<size_t, N + 1> fieldPtr;
    fieldPtr[0] = 0;
    std::array<char, allFieldsLen> fieldBuf;
    std::array<const char *, N> fieldTypes;
    size_t offset = 0;

    int counter = 1;

    boost::pfr::for_each_field(T{}, [&](const auto &field, auto _idx) {
        constexpr int idx = _idx.value;
        using field_t = std::remove_cvref_t<decltype(field)>;
        constexpr auto topFieldName = fieldNames[idx];

        if constexpr (std::is_class_v<field_t>)
        {
            auto [subFieldPtr, subFieldBuf, subFieldTypes] = getTableDesc<field_t>();
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
                fieldPtr[counter++] = offset;
            }
        }
        else
        {
            if (std::is_integral_v<field_t>)
            {
                std::copy(topFieldName.begin(), topFieldName.end(), fieldBuf.begin() + offset);
                offset += topFieldName.size();
                fieldTypes[counter - 1] = "Integer";
                fieldPtr[counter++] = offset;
            }
            else if (std::is_floating_point_v<field_t>)
            {
                std::copy(topFieldName.begin(), topFieldName.end(), fieldBuf.begin() + offset);
                offset += topFieldName.size();
                fieldTypes[counter - 1] = "Real";
                fieldPtr[counter++] = offset;
            }
            else
            {
                assert(0 && "not supported");
            }
        }
    });
    return std::tuple{fieldPtr, fieldBuf, fieldTypes};
}
}; // namespace TableDesc

} // namespace pse
