#include "ral/section.h"
#include "record/record_defines.h"
#include "record/record_type.h"
#include "record/wrap_defines.h"
#include <ral/backend.h>
#include <fsl/raw_backend.h>
#include <filesystem>
#include <spdlog/spdlog.h>

using namespace pse;
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        spdlog::error("usage: {} <file>", argv[0]);
        return 1;
    }
    spdlog::info("open file {}", argv[1]);
    auto raw_backend = pse::fsl::RawSectionBackend(argv[1], pse::ral::RWMode::READ);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    // ral::BackendInterface &backend = static_cast<pse::ral::BackendInterface &>(_backend);
    auto root = backend.openRootSection();
    auto trace_dir = root->openDirSection(StaticSectionDesc::TRACE_SEC_ID, false);
    auto iter = trace_dir->begin();
    auto pid = iter.getDesc();
    spdlog::info("iter for process isa dir: {}", iter.isa(ral::SectionBase::DIR));
    auto process_trace_dir = iter.getDirSection();
    auto thread_trace_dir = process_trace_dir->openDirSection(pid, false);
    if (!thread_trace_dir)
    {
        spdlog::error("failed to open thread trace dir");
    }
    auto generic_trace_dir = thread_trace_dir->openDirSection(StaticSectionDesc::GENERIC_TRACE_SEC_ID, false);
    if (!generic_trace_dir)
    {
        spdlog::error("failed to open generic trace dir");
    }
    spdlog::info("event_PROCESS_START = {}", event_PROCESS_START);
    auto start = generic_trace_dir->openDataSection<record_t>(StaticSectionDesc::RECORD_SEC_OFFSET + event_PROCESS_START, false, 0, 0);
    if (!start)
    {
        spdlog::error("failed to open start data section");
    }
    spdlog::info("event_PROCESS_EXIT = {}", event_PROCESS_EXIT);
    auto exit = generic_trace_dir->openDataSection<record_t>(StaticSectionDesc::RECORD_SEC_OFFSET + event_PROCESS_EXIT, false, 0, 0);
    if (!exit)
    {
        spdlog::error("failed to open exit data section");
    }
    record_t rec;
    bool error = false;
    start->read(&rec, 0);
    if (rec.MsgType != event_PROCESS_START)
    {
        spdlog::error("failed at test1: rec.MsgType {} != {}", rec.MsgType, event_PROCESS_START);
        error = true;
    }
    exit->read(&rec, 0);
    if (rec.MsgType != event_PROCESS_EXIT)
    {
        spdlog::error("failed at test2: rec.MsgType {} != {}", rec.MsgType, event_PROCESS_EXIT);
        error = true;
    }
    if (!error)
    {
        spdlog::info("pass");
    }
    else 
    {
        spdlog::error("fail");
    }
}