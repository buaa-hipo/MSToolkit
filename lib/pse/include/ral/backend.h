#pragma once
#include <string>
#include <memory>
#include <ral/section.h>
#include <fsl/fsl_section.hpp>
#include <sql/driver.h>
#include <sql/sql_section.h>
#include <ral/section_declare.h>
#include <type_traits>

namespace pse
{
namespace ral
{
template <typename BackendImpl_t>
class BackendMixin
{
public:
    auto openRootSection()
    {
        auto self = static_cast<BackendImpl_t *>(this);
        return self->openRootSection();
    }
    auto isLeader()
    {
        auto self = static_cast<BackendImpl_t *>(this);
        return self->isLeader();
    }
};

class BackendInterface
{
public:
    virtual std::unique_ptr<DirSectionInterface> openRootSection() = 0;
    virtual bool isLeader() = 0;
    virtual ~BackendInterface() = default;
};
template <typename T>
    requires std::is_base_of_v<BackendMixin<T>, T>
class BackendWrapper : public BackendInterface
{
    T _backend;

public:
    BackendWrapper(T &&backend)
    : _backend(std::move(backend))
    {
    }
    virtual std::unique_ptr<DirSectionInterface> openRootSection() override
    {
        auto &_backend_mixin = static_cast<BackendMixin<T> &>(_backend);
        using sec_t = decltype(_backend_mixin.openRootSection());
        return std::make_unique<DirSectionWrapper<sec_t>>(std::move(_backend_mixin.openRootSection()));
    }
    virtual bool isLeader() override
    {
        auto &_backend_mixin = static_cast<BackendMixin<T> &>(_backend);
        return _backend_mixin.isLeader();
    }
};

class Backend : public std::enable_shared_from_this<Backend>
{
    std::variant<std::shared_ptr<fsl::FSL_Backend>, std::shared_ptr<sql::SQLBackend>> _backend;

public:
    enum BackendMode
    {
        SECTION_FILE,
        SQLITE,
    };
    static std::shared_ptr<Backend> open(std::string_view name, BackendMode backendMode, RWMode rwMode);
    DirSection openRootSection();
    DirSection openDirSection(DirSection &dir, desc_t desc, bool create);
    template <typename record_t>
    DataSection<record_t> openDataSection(DirSection &dir, desc_t desc, bool create, int recordVariableLength = 0)
    {
        DataSection<record_t> res;

        std::visit(
            [&](auto &&val) {
                using backend_t = std::remove_cvref_t<decltype(*val.get())>;
                static_assert(std::is_same_v<backend_t, fsl::FSL_Backend> ||
                              std::is_same_v<backend_t, sql::SQLBackend>);
                using section_unique_ptr_t = std::unique_ptr<typename backend_t::DirSectionImpl_t>;
                res.open(shared_from_this(),
                         val->template openDataSec<record_t>(
                             std::get<section_unique_ptr_t>(dir._sec).get(), desc, create, recordVariableLength));
            },
            _backend);
        return res;
    }

protected:
    Backend(std::string_view name, BackendMode backendMode, RWMode rwMode);
};

} // namespace ral

} // namespace pse
