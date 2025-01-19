#include <sql/driver.h>
#include <iostream>

struct R1
{
    int a;
    float b;
    uint64_t time;
};

struct R2
{
    int m;
    int b;
    int c;
    R1 r1;
    uint64_t time;
};
using namespace pse;
using namespace pse::sql;
int main()
{
    // constexpr auto table = TableDesc::getTableDesc<R1>();
    constexpr auto res = TableDesc::getTableDesc<R2>();
    constexpr auto allFieldsLen = TableDesc::getAllFieldsLen<R2>();
    std::cout << sizeof(res) << std::endl;
    const auto &fieldPtr = std::get<0>(res);
    const auto &fieldBuf = std::get<1>(res);
    const auto &fieldTypes = std::get<2>(res);
    std::cout << fieldBuf.size() << std::endl;
    std::cout << allFieldsLen << std::endl;
    std::cout << TableDesc::getAllFieldsLen<R2>() << std::endl;
    for (size_t i = 0; i < fieldPtr.size() - 1; ++i)
    {
        std::cout << fieldPtr[i + 1] << "\n";
        std::cout << std::string_view(fieldBuf.data() + fieldPtr[i], fieldPtr[i + 1] - fieldPtr[i]) << ": "
                  << fieldTypes[i] << "\n";
    }

    SQLiteHandle handle("test.db");
    handle.createTable<R2>("R2_table");
    R2 r2;
    r2.b = 1;
    r2.c = 2;
    r2.m = 3;
    r2.r1.a = 4;
    r2.r1.b = 5;
    r2.r1.time = 6;
    r2.time = 7;
    handle.insert<R1>("R2_table", r2.r1);
    return 0;
    // for (const auto &[fieldName, fieldType] : table)
    // {
    //     std::cout << fieldName << ": " << fieldType << std::endl;
    // }
}