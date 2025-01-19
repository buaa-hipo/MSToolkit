#include "utils/compile_time.h"
#include <spdlog/spdlog.h>
#include "magic_enum.hpp"
using namespace pse::utils;

enum E1
{
    E1_a,
    E1_b,
    E1_c
};
enum E2
{
    E2_x,
    E2_y,
    E2_z
};

template <auto Fn>
struct Str2Array
{
    static constexpr auto str = Fn();
    static constexpr auto size = str.size();
    static constexpr std::array<char, size + 1> arr = [] {
        std::array<char, size + 1> res{};
        for (int i = 0; i < size; ++i)
        {
            res[i] = str[i];
        }
        res[size] = '\0';
        return res;
    }();
};

template <int N>
consteval std::array<char, N> replace(std::array<char, N> arr, char from, char to)
{
    for (int i = 0; i < N; ++i)
    {
        if (arr[i] == from)
        {
            arr[i] = to;
        }
    }
    return arr;
}

template <std::size_t N>
consteval std::string_view make_string_view(const std::array<char, N> &arr)
{
    return std::string_view(arr.data(), arr.size() - 1);
}
template <E1 e1>
consteval E2 transform()
{
    constexpr auto name1 = magic_enum::enum_name<E1>(e1);
    constexpr auto name1_array = Str2Array<[]() { return magic_enum::enum_name<E1>(e1); }>::arr;
    constexpr auto name2_array = replace<name1_array.size()>(name1_array, '1', '2');
    constexpr auto name3_array = replace<name2_array.size()>(name2_array, 'a', 'x');
    std::string_view name1_sv = make_string_view(name1_array);
    auto name3_sv = make_string_view(name3_array);

    return magic_enum::enum_cast<E2>(name3_sv).value_or(E2::E2_z);
}

struct R1
{
    int a;
    int b;
    uint64_t time;
};
struct R2
{
    int c;
    R1 r1;
};
struct R3
{
    int d;
    R2 r2;
};
using namespace std::literals;

constexpr const char R1_field[] = "time";
int main()
{
    spdlog::info("{}", DetectField<R1>::has_field(R1_field));
    spdlog::info("{}", DetectField<R1>::get_field_idx(R1_field));
    spdlog::info("{}", DetectField<R1>::get_field_idx("xxx"));
    R3 r3;
    auto &time = GetFieldHelper<R3, []() { return "r2.r1.time"sv; }>::get_field(r3);
    time = 2;
    spdlog::info("{}", r3.r2.r1.time);
    spdlog::info("{}", GetFieldHelper<R3, []() { return "r2.r1.time"sv; }>::has_field());
    spdlog::info("{}", GetFieldHelper<R3, []() { return "r1.t"sv; }>::has_field());
    auto res = DetectField<R3>::get_field_idx("r1");
    spdlog::info("{}", res);
    spdlog::info("{}", DetectField<R3>::has_field("r1.time"));
    R2 r2;
    auto &time2 = GetField<R3, []() { return "r2.r1.time"sv; }, [] { return "r2.time"sv; }>::get_field(r3);
    time2 = 3;
    auto &time3 = GetField<R2, []() { return "r2.r1.time"sv; }, []() { return "r1.time"sv; }>::get_field(r2);
    spdlog::info("{}", magic_enum::enum_name<E2>(transform<E1::E1_a>()));

    R2 r;
    r.c = 1000;
    spdlog::info("{}", PSE_GETFIELD_V_OR(r, "c"sv, 0));
    spdlog::info("{}", PSE_GETFIELD_V_OR(r, "d"sv, 0));
}