#include "rocm_helper.h"
#include "record/record_writer.h"


extern std::unique_ptr<pse::ral::DirSectionInterface> & get_thread_trace_dir();

size_t writeStringSection(const char *str) {
    auto& dir = get_thread_trace_dir();
    auto string_sec = dir->openStringSection(StaticSectionDesc::COMMON_USE_STRING_SEC, true);
    auto res = string_sec->write(str);
    return res;
}

void helperExtStore(size_t id, const void* record, size_t size) {
    RecordWriter::extStore(id, record, size);
}

void helperTraceStore(void* record) {
    RecordWriter::traceStore((const record_t*)(record));
}

void rocmMetaSet() {
    RecordWriter::metaSectionStart("ACCL TRACE META");
    RecordWriter::metaStore<std::string>("ACCL_DEVICE_TYPE", "HYGON");
    RecordWriter::metaSectionEnd("ACCL TRACE META");
}