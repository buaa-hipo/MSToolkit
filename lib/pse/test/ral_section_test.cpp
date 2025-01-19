#include <ral/type_register.h>

enum class RecordEnum
{
    R1,
    R2,
    R3
};
struct R1
{
    int a;
    float b;
    long time;
};
namespace pse
{
namespace ral
{
RAL_DECLARE_RECORD_ENUM(::RecordEnum);
RAL_REGISTER_ENUM(::RecordEnum::R1, R1);
// template <>
// class EnumRegister<RecordEnum::R1>
// {
// public:
//     static constexpr decltype(RecordEnum::R1) value = RecordEnum::R1;
//     using type = decltype(RecordEnum::R1);
//     using record_t = R1;
//     EnumRegister()
//     {
//         RecordEnumMap[RecordEnum::R1] = [](pse::ral::DirSection &dir) -> std::unique_ptr<pse::ral::DataSectionBase> {
//             return pse::ral::_openDataStaticUnique<RecordEnum::R1>(dir);
//         };
//     }
//     static EnumRegister<RecordEnum::R1> R1_register;
// };
// inline EnumRegister<RecordEnum::R1> EnumRegister<RecordEnum::R1>::R1_register;

} // namespace ral
} // namespace pse

#include <ral/section.hpp>
#include <filesystem>
using namespace pse::ral;
constexpr int len = 20000;

auto write_test(Backend::BackendMode mode)
{
    auto backend = Backend::open("c.data", mode, RWMode::WRITE);
    auto root = backend->openRootSection();
    auto dir1 = root.openDir(1, true);
    auto dir2 = dir1.openDir(2, true);
    auto data_section = dir2.openDataStatic<RecordEnum::R1>(true);
    for (int i = 0; i < len; ++i)
    {
        R1 r1{i, (float)i + 1, (long)i + 2};
        data_section.writeRecord(&r1);
    }
    return dir2.secMap;
}
void read_test(auto secMap, Backend::BackendMode mode)
{
    auto backend = Backend::open("c.data", mode, RWMode::READ);
    auto root = backend->openRootSection();
    auto dir1 = root.openDir(1, false);
    auto dir2 = dir1.openDir(2, false);
    dir2.secMap = secMap;
    auto data_section = dir2.openDataStatic<RecordEnum::R1>(false);
    auto data_section2 = dir1.openDataStatic<RecordEnum::R1>(false);
    if (data_section2.valid())
    {
        spdlog::error("failed: data sec2 valid");
        exit(-1);
    }
    if (!data_section.valid())
    {
        spdlog::error("failed: data sec invalid");
        exit(-1);
    }
    for (int i = 0; i < len; ++i)
    {
        R1 r;
        data_section.readRecord(&r);
        if (r.a != i || r.b != i + 1 || r.time != i + 2)
        {
            spdlog::error("failed at cmp {}", i);
            exit(-1);
        }
    }
}
int main()
{
    std::filesystem::remove("c.data");
    std::filesystem::remove("c.data.lock");
    auto res = write_test(Backend::SECTION_FILE);
    read_test(res, Backend::SECTION_FILE);
    std::filesystem::remove("c.data");
    res = write_test(Backend::SQLITE);
    read_test(res, Backend::SQLITE);
    spdlog::info("pass");
}