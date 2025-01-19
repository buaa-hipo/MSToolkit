#include "ral/backend.h"
#include <exception>

namespace pse
{
namespace ral
{

namespace
{
struct make_shared_enabler : public Backend
{
    template <typename... Args>
    make_shared_enabler(Args &&...args)
    : Backend(std::forward<Args>(args)...)
    {
    }
};
} // namespace
std::shared_ptr<Backend> pse::ral::Backend::open(std::string_view name, BackendMode mode, RWMode rwMode)
{
    return std::make_shared<make_shared_enabler>(name, mode, rwMode);
}

DirSection Backend::openRootSection()
{
    DirSection dir;
    std::visit([&, this](auto &&val) { dir.open(this->shared_from_this(), val->openRootSec()); }, _backend);
    return dir;
}
DirSection pse::ral::Backend::openDirSection(DirSection &dir, fsl::Block::desc_t desc, bool create)
{
    DirSection res;
    std::visit(
        [&, this](auto &&val) {
            using backend_t = std::remove_cvref_t<decltype(*val.get())>;
            static_assert(std::is_same_v<backend_t, fsl::FSL_Backend> || std::is_same_v<backend_t, sql::SQLBackend>);
            using section_unique_ptr_t = std::unique_ptr<typename backend_t::DirSectionImpl_t>;
            res.open(shared_from_this(), val->openDirSec(std::get<section_unique_ptr_t>(dir._sec).get(), desc, create));
        },
        _backend);
    return res;
}
pse::ral::Backend::Backend(std::string_view name, BackendMode mode, RWMode rwMode)
{
    if (mode == SECTION_FILE)
    {
        _backend = fsl::FSL_Backend::create(name, rwMode);
    }
    else if (mode == SQLITE)
    {
#ifndef PSE_ENABLE_SQLITE_BACKEND
        throw std::invalid_argument("SQLite backend not enabled");
#endif
        _backend = sql::SQLBackend::create(name, rwMode);
    }
}
} // namespace ral

} // namespace pse