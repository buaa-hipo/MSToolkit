#pragma once
#include <type_traits>
#include <string.h>
#include <unordered_map>
#include <boost/pfr.hpp>
#include <assert.h>
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <typeindex>
#include <sql/table_desc.h>
#include <exception>

namespace pse
{

namespace sql
{
namespace detail
{

consteval auto ct_strlen(const char *str)
{
    std::size_t len = 0;
    while (*str++)
    {
        len++;
    }
    return len;
}
template <typename T>
consteval auto ct_all_strlen(const T &list)
{
    std::size_t len = 0;
    for (auto val : list)
    {
        len += ct_strlen(val);
    }
    return len;
}
} // namespace detail

class SQLiteHandle
{
    sqlite3 *pdb;
    SQLiteHandle &operator=(const SQLiteHandle &) = delete;

    std::unordered_map<std::string, sqlite3_stmt *> stmts;
    std::unordered_map<std::string, std::type_index> tableTypes;

public:
    SQLiteHandle(const std::string &name);
    SQLiteHandle(SQLiteHandle &&handle);
    SQLiteHandle &operator=(SQLiteHandle &&);
    ~SQLiteHandle();

    bool valid() { return pdb != nullptr; }
    bool tableExists(const std::string &tableName)
    {
        using namespace std::literals;
        if (!valid())
        {
            return false;
        }

        auto res = sqlite3_exec(pdb, ("SELECT 1 FROM "s + tableName).c_str(), nullptr, nullptr, nullptr);
        return res == SQLITE_OK;
    }
    template <typename T>
        requires std::is_aggregate_v<T>
    void createTable(const std::string &tableName)
    {
        tableTypes.try_emplace(tableName, std::type_index(typeid(T)));
        constexpr auto &suffix = CreateSuffixSQL_v<T>;
        auto sql = std::string("CREATE TABLE IF NOT EXISTS ") + tableName;
        auto iter = std::back_insert_iterator(sql);
        std::copy(suffix.begin(), suffix.end(), iter);
        auto code = sqlite3_exec(pdb, sql.c_str(), nullptr, nullptr, nullptr);
        assert(code == SQLITE_OK && "failed to create table");
        code = sqlite3_exec(pdb, "BEGIN;", nullptr, nullptr, nullptr);
        assert(code == SQLITE_OK && "failed to begin TRANSACTION");
    }

    template <typename T>
        requires std::is_aggregate_v<T>
    void insert(const std::string &tableName, const T &data)
    {
        auto tableIter = tableTypes.find(tableName);
        if (tableIter == tableTypes.end())
        {
            spdlog::error("Try to insert values into a not existed table {}", tableName);
            throw std::invalid_argument("invalid table name");
        }
        if (std::type_index(typeid(T)) != tableIter->second)
        {
            spdlog::error("Try to insert values into table {} with unmatched type: {} -> {}",
                          tableName,
                          typeid(T).name(),
                          tableIter->second.name());
            throw std::invalid_argument("invalid record type for table");
        }
        auto iter = stmts.find(tableName);
        sqlite3_stmt *stmt = nullptr;
        if (iter != stmts.end())
        {
            stmt = iter->second;
        }
        else
        {
            auto &suffix = InsertSuffixSQL_v<T>;
            auto insertSQL = std::string("INSERT INTO ") + tableName;
            auto pos = std::back_insert_iterator(insertSQL);
            std::copy_n(suffix.begin(), suffix.size(), pos);
            auto code = sqlite3_prepare_v2(pdb, insertSQL.c_str(), insertSQL.size(), &stmt, NULL);
            if (code != SQLITE_OK)
            {
                spdlog::error("can not compile sql statement {}: {}", insertSQL, sqlite3_errmsg(pdb));
                throw std::invalid_argument("invalid sql statement");
            }
            stmts.emplace(tableName, stmt);
        }
        auto count = bind(stmt, data);
        spdlog::debug("bind count: {}", count);
        auto code = sqlite3_step(stmt);
        if (code != SQLITE_DONE && code != SQLITE_ROW)
        {
            spdlog::error("can not step sql for table {}: {}", tableName, sqlite3_errmsg(pdb));
            throw std::runtime_error("failed to step sql");
        }
        code = sqlite3_reset(stmt);
        if (code != SQLITE_OK)
        {
            spdlog::error("failed to reset sqlite3 stmt");
            throw std::runtime_error("failed to reset sql stmt");
        }
    }

private:
    template <typename T>
        requires std::is_aggregate_v<T>
    int bind(sqlite3_stmt *stmt, const T &data, int count = 1)
    {
        boost::pfr::for_each_field(data, [&](const auto &field, std::size_t idx) {
            using field_t = std::remove_cvref_t<decltype(field)>;
            if constexpr (std::is_integral_v<field_t> && sizeof(field_t) <= sizeof(int))
            {
                sqlite3_bind_int(stmt, count++, field);
            }
            else if constexpr (std::is_integral_v<field_t>)
            {
                sqlite3_bind_int64(stmt, count++, field);
            }
            else if constexpr (std::is_floating_point_v<field_t>)
            {
                sqlite3_bind_double(stmt, count++, field);
            }
            else if constexpr (std::is_class_v<field_t>)
            {
                count = bind<field_t>(stmt, field, count);
            }
            static_assert(
                (std::is_integral_v<field_t> || std::is_floating_point_v<field_t> || std::is_class_v<field_t>) &&
                "record type not supported");
        });
        return count;
    }

    template <typename T>
        requires std::is_aggregate_v<T>
    int column_get(sqlite3_stmt *stmt, T &data, int count = 0)
    {
        boost::pfr::for_each_field(data, [&](auto &field, std::size_t idx) {
            using field_t = std::remove_cvref_t<decltype(field)>;
            if constexpr (std::is_integral_v<field_t> && sizeof(field_t) <= sizeof(int))
            {
                field = sqlite3_column_int(stmt, count++);
            }
            else if constexpr (std::is_integral_v<field_t>)
            {
                field = sqlite3_column_int64(stmt, count++);
            }
            else if constexpr (std::is_floating_point_v<field_t>)
            {
                field = sqlite3_column_double(stmt, count++);
            }
            else if constexpr (std::is_class_v<field_t>)
            {
                count = column_get<field_t>(stmt, field, count);
            }
            static_assert(
                (std::is_integral_v<field_t> || std::is_floating_point_v<field_t> || std::is_class_v<field_t>) &&
                "record type not supported");
        });
        return count;
    }

public:
    template <typename T>
        requires std::is_aggregate_v<T>
    void query(std::string_view tableName, size_t offset, T &data)
    {
        std::string sql = std::string("SELECT * FROM ").append(tableName) + " LIMIT 1 OFFSET " + std::to_string(offset);
        sqlite3_stmt *stmt;
        auto code = sqlite3_prepare_v2(pdb, sql.c_str(), sql.size(), &stmt, nullptr);
        if (code != SQLITE_OK)
        {
            spdlog::error("failed to compile sql {}: {}", sql, sqlite3_errmsg(pdb));
            throw std::runtime_error("sql error");
        }
        code = sqlite3_step(stmt);
        if (code != SQLITE_ROW)
        {
            spdlog::error("failed to step sql {}: {}", sql, sqlite3_errmsg(pdb));
            throw std::runtime_error("sql error");
        }
        column_get<T>(stmt, data);
        sqlite3_finalize(stmt);
    }

private:
    /**
     * @brief Build compile-time sql statement.
     * For a insert statement like "INSERT INTO TABLE T1 (F1, F2) VALUES(V1, V2)",
     * this function return a std::array of "F1, F2"
     * @tparam T 
     * @return std::array<char, N>
     */
    template <typename T>
    consteval static auto buildInsertTableFieldSQL()
    {
        constexpr auto tableDesc = TableDesc::getTableDesc<T>();
        // auto &[fieldPtr, fieldBuf, fieldTypes] = tableDesc;
        constexpr auto fieldPtr = std::get<0>(tableDesc);
        constexpr auto fieldBuf = std::get<1>(tableDesc);
        constexpr auto fieldTypes = std::get<2>(tableDesc);
        constexpr auto len = fieldBuf.size() + fieldTypes.size() - 1;
        std::array<char, len> res;
        std::size_t offset = 0;
        for (size_t i = 0; i < fieldTypes.size(); ++i)
        {
            auto copyLen = fieldPtr[i + 1] - fieldPtr[i];
            std::copy_n(fieldBuf.begin() + fieldPtr[i], copyLen, res.begin() + offset);
            offset += copyLen;
            if (i < fieldTypes.size() - 1)
            {
                res[offset++] = ',';
            }
        }
        return res;
    }
    /**
     * @brief Build compile-time sql statement.
     * For a sql statement "INSERT INTO TABLE T1 (F1, F2) VALUES(?,?)",
     * return std::array of "?,?"
     * @tparam T 
     * @return consteval 
     */
    template <typename T>
    consteval static auto buildInsertValuesSQL()
    {
        constexpr auto fieldNum = TableDesc::getFieldNum<T>();
        std::array<char, fieldNum * 2 - 1> res;
        for (int i = 0; i < fieldNum; ++i)
        {
            res[2 * i] = '?';
            if (i < fieldNum - 1)
            {
                res[2 * i + 1] = ',';
            }
        }
        return res;
    }
    /**
     * @brief build compile-time sql statement.
     * For a sql like "INSERT INTO TABLE T1 (F1,F2)VALUES(?,?)",
     * return a std::array of "(F1,F2)VALUES(?,?)"
     * @tparam T 
     * @return std::array<char, N>
     */
    template <typename T>
    consteval static auto buildInsertSuffixSQL()
    {
        constexpr auto fields = buildInsertTableFieldSQL<T>();
        constexpr auto values = buildInsertValuesSQL<T>();
        std::array<char, fields.size() + values.size() + 10> res;
        res[0] = '(';
        std::size_t offset = 1;
        std::copy_n(fields.begin(), fields.size(), res.begin() + offset);
        offset += fields.size();
        std::copy_n(")VALUES", 7, res.begin() + offset);
        offset += 7;
        res[offset++] = '(';
        std::copy_n(values.begin(), values.size(), res.begin() + offset);
        offset += values.size();
        res[offset++] = ')';
        return res;
    }

    template <typename T>
    struct InsertSuffixSQL
    {
        constexpr static auto value = SQLiteHandle::buildInsertSuffixSQL<T>();
    };

    template <typename T>
    inline static auto InsertSuffixSQL_v = InsertSuffixSQL<T>::value;

    /**
     * @brief build compile-time sql statement
     * For a statement "CREATE TABLE IF NOT EXIST T1(F1 T1, F2 T2)",
     * return std::array of "F1 T1, F2 T2"
     * @tparam T 
     * @return std::array<char, N> 
     */
    template <typename T>
    consteval static auto buildCreateTableFieldSQL()
    {
        constexpr auto tableDesc = TableDesc::getTableDesc<T>();
        // auto &[fieldPtr, fieldBuf, fieldTypes] = tableDesc;
        constexpr auto fieldPtr = std::get<0>(tableDesc);
        constexpr auto fieldBuf = std::get<1>(tableDesc);
        constexpr auto fieldTypes = std::get<2>(tableDesc);
        constexpr auto len =
            TableDesc::getAllFieldsLen<T>() + detail::ct_all_strlen(fieldTypes) + fieldTypes.size() * 2 - 1;
        std::array<char, len> res{};
        std::size_t offset = 0;
        for (size_t i = 0; i < fieldTypes.size(); ++i)
        {
            auto copyLen = fieldPtr[i + 1] - fieldPtr[i];
            std::copy_n(fieldBuf.begin() + fieldPtr[i], copyLen, res.begin() + offset);
            offset += copyLen;

            res[offset++] = ' ';
            copyLen = detail::ct_strlen(fieldTypes[i]);
            std::copy_n(fieldTypes[i], copyLen, res.begin() + offset);
            offset += copyLen;
            if (i < fieldTypes.size() - 1)
            {
                res[offset++] = ',';
            }
        }
        return res;
    }

    /**
     * @brief build compile-time sql statement
     * For a statement "CREATE TABLE IF NOT EXIST T1(F1 T1, F2 T2)",
     * return std::array of "(F1 T1, F2 T2)"
     * @tparam T 
     * @return std::array<char, N> 
     */
    template <typename T>
    consteval static auto buildCreateSuffixSQL()
    {
        constexpr auto fields = buildCreateTableFieldSQL<T>();
        constexpr std::size_t len = 2 + fields.size();
        std::array<char, len> res;
        res[0] = '(';
        std::copy(fields.begin(), fields.end(), res.begin() + 1);
        res[len - 1] = ')';
        return res;
    }

    template <typename T>
    struct CreateSuffixSQL
    {
        constexpr static auto value = SQLiteHandle::buildCreateSuffixSQL<T>();
    };
    template <typename T>
    inline static auto CreateSuffixSQL_v = CreateSuffixSQL<T>::value;

private:
    void move_from(SQLiteHandle &&handle);
    void cleanup();
};
} // namespace sql
} // namespace pse
