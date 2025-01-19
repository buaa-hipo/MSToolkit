#include "record/record_defines.h"
#include "record/record_reader.h"
#include "record/record_type.h"
#include "record/record_type_info.h"
#include "record/record_writer.h"
#include <alloca.h>
#include <spdlog/spdlog.h>
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("%s <trace-dir>\n", argv[0]);
        exit(0);
    }
    {
        RecordReader reader(argv[1], SECTION_MODEL, nullptr, true, false, false);
        auto traces = reader.get_all_traces();
        for (auto& trace: traces)
        {
            int pass = 0;
            int i = 0;
            for (auto it = trace.second->begin(); it != trace.second->end(); it = it.next())
            {
                if (it.val()->MsgType == JSI_PROCESS_EXIT || it.val()->MsgType == JSI_PROCESS_START)
                {
                    continue;
                }
                if (it.val()->MsgType != 9 - i)
                {
                    spdlog::error("msgtype not match: {} != {}", it.val()->MsgType, 9-i);
                    exit(-1);
                }
                i++;
                pass += 1;
            }
            if (pass != 10)
            {
                spdlog::error("record number not match: {} != {}", pass, 10);
                exit(-1);
            }
            spdlog::info("pass read test");
            bool zooms = trace.second->zoom(4, 7, 0);
            pass = 0;
            i = 4;
            for (auto it = trace.second->begin(); it != trace.second->end(); it = it.next())
            {
                if (it.val()->MsgType == JSI_PROCESS_EXIT || it.val()->MsgType == JSI_PROCESS_START)
                {
                    continue;
                }
                if (it.val()->MsgType != 10 - i)
                {
                    spdlog::error("zoom msgtype not match: {} != {}", it.val()->MsgType, 10-i);
                    exit(-1);
                }
                pass += 1;
                i++;
            }
            if (pass != 3)
            {
                spdlog::error("zoom record number not match: {} != {}", pass, 4);
                exit(-1);
            }
            pass = 0;
            i = 0;

            for (auto it = trace.second->global_begin(); it != trace.second->global_end(); it = it.next())
            {
                if (it.val()->MsgType == JSI_PROCESS_EXIT || it.val()->MsgType == JSI_PROCESS_START)
                {
                    continue;
                }
                if (it.val()->MsgType != 9 - i)
                {
                    spdlog::error("global msgtype not match: {} != {}", it.val()->MsgType, 9-i);
                    exit(-1);
                }
                pass += 1;
                i++;
            }
            if (pass != 10)
            {
                spdlog::error("global record number not match: {} != {}", pass, 10);
                exit(-1);
            }

            break;
        }
    }
    spdlog::info("pass");

}