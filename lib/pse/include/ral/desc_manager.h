#pragma once
#include <string_view>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <ral/section_declare.h>
#include <source_location>
#include <utils/compile_time.h>

namespace pse
{
namespace ral
{

class SectionDescMap
{
    std::unordered_map<std::string, desc_t> typeMap;
    desc_t nextId = INIT_ALLOCATE_ID;

private:
    template <bool is_named>
    desc_t _get(std::string_view name, bool create)
    {
        using namespace std::literals;

        std::string realName;
        if constexpr (is_named)
        {
            realName = name;
        }
        else
        {
            realName = "@"s;
            realName.append(name);
        }
        if (create)
        {
            const auto [iter, inserted] = typeMap.try_emplace(std::move(realName), nextId);
            if (inserted)
            {
                nextId++;
            }
            return iter->second;
        }
        else
        {
            const auto iter = typeMap.find(realName);
            if (iter == typeMap.end())
            {
                return 0;
            }
            else
            {
                return iter->second;
            }
        }
    }

public:
    template <typename record_t>
        requires std::is_aggregate_v<record_t>
    desc_t get(bool create)
    {
        return _get<false>(utils::typesv<record_t>, create);
    }

    desc_t get(std::string_view sectionName, bool create) { return _get<true>(sectionName, create); }
};

} // namespace ral

} // namespace pse
