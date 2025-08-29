#pragma once
#include <stddef.h>
#include <memory>
#include <ral/section_declare.h>
namespace pse
{
namespace ral
{

template <typename record_t, typename DataSectionImpl_t>
class DataSectionImplMixin
{
public:
    void writeRecord(size_t i, const record_t *record)
    {
        auto child_this = static_cast<DataSectionImpl_t *>(this);
        // return child_this->backend()->writeDataSection()
        return child_this->_writeRecord(i, record);
    }
    void readRecord(size_t i, record_t *record)
    {
        auto child_this = static_cast<DataSectionImpl_t *>(this);
        return child_this->_readRecord(i, record);
    }
};

template <typename DirSectionImpl_t, template <typename> typename DataSectionImpl_t>
class DirSectionImplMixin
{
public:
    std::unique_ptr<DirSectionImpl_t> openDir(desc_t desc, bool create)
    {
        auto child_this = static_cast<DirSectionImpl_t *>(this);
        return child_this->backend()->openDirSec(child_this, desc, create);
    }

    template <typename record_t>
    inline std::unique_ptr<DataSectionImpl_t<record_t>> openData(desc_t desc, bool create)
    {
        auto child_this = static_cast<DirSectionImpl_t *>(this);
        return child_this->backend()->template openDataSec<record_t>(child_this, desc, create);
    }
};

} // namespace ral

} // namespace pse
