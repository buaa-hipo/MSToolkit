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
    auto send = generic_trace_dir->openDataSection<record_comm_t>(StaticSectionDesc::RECORD_SEC_OFFSET + event_MPI_Send, false, 0, 0);
    if (!send)
    {
        spdlog::error("failed to open send data section");
    }
    auto recv = generic_trace_dir->openDataSection<record_comm_t>(StaticSectionDesc::RECORD_SEC_OFFSET + event_MPI_Recv, false, 0, 0);
    record_comm_t rec;
    bool error = false;
    for (int i = 0; i < 5; ++i)
    {
        send->read(&rec, i);
        if (rec.record.MsgType != event_MPI_Send)
        {
            spdlog::error("failed at test1: rec.MsgType {} != {}", rec.record.MsgType, event_MPI_Send);
            error = true;
        }
        if (rec.dest != i * 10)
        {
            spdlog::error("failed at test1: rec.dest {} != {}", rec.dest, i * 10);
            error = true;
        }
        if (rec.count != i * 100)
        {
            spdlog::error("failed at test1: rec.count {} != {}", rec.count, i * 100);
            error = true;
        }
        if (rec.record.timestamps.enter != i * 1000)
        {
            spdlog::error("failed at test1: rec.record.timestamps.enter {} != {}", rec.record.timestamps.enter, i * 1000);
            error = true;
        }

        recv->read(&rec, i);
        if (rec.record.MsgType != event_MPI_Recv)
        {
            spdlog::error("failed at test1: rec.MsgType {} != {}", rec.record.MsgType, event_MPI_Recv);
            error = true;
        }
        if (rec.dest != (i+5) * 10)
        {
            spdlog::error("failed at test1: rec.dest {} != {}", rec.dest, i * 10);
            error = true;
        }

        if (rec.count != (i+5) * 100)
        {
            spdlog::error("failed at test1: rec.count {} != {}", rec.count, i * 100);
            error = true;
        }
        if (rec.record.timestamps.enter != (i+5) * 1000)
        {
            spdlog::error("failed at test1: rec.record.timestamps.enter {} != {}", rec.record.timestamps.enter, i * 1000);
            error = true;
        }
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