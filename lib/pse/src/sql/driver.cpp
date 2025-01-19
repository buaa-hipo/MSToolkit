#include <sql/driver.h>
#include <sqlite3.h>
#include <boost/pfr.hpp>
#include <spdlog/spdlog.h>

// std::unordered_map<std::string, TableDesc> TableDesc::typeMap;
namespace pse
{
namespace sql
{
SQLiteHandle::SQLiteHandle(const std::string &dbName)
{
    auto res = sqlite3_open(dbName.c_str(), &pdb);
    if (res != SQLITE_OK)
    {
        spdlog::error("failed to open db {} with error code {}", dbName, res);
        throw std::runtime_error("failed to open db");
    }
}

SQLiteHandle::SQLiteHandle(SQLiteHandle &&handle) { move_from(std::move(handle)); }
SQLiteHandle &SQLiteHandle::operator=(SQLiteHandle &&handle)
{
    move_from(std::move(handle));
    return *this;
}
SQLiteHandle::~SQLiteHandle() { cleanup(); }

void SQLiteHandle::move_from(SQLiteHandle &&handle)
{
    this->pdb = handle.pdb;
    handle.pdb = nullptr;
    this->stmts = std::move(handle.stmts);
    this->tableTypes = std::move(handle.tableTypes);
}
} // namespace sql
void sql::SQLiteHandle::cleanup()
{
    sqlite3_exec(pdb, "COMMIT;", nullptr, nullptr, nullptr);
    for (auto &[tableName, stmt] : stmts)
    {
        sqlite3_finalize(stmt);
    }
    sqlite3_close(pdb);
}
} // namespace pse