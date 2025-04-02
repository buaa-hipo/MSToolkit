#include "record/record_reader.h"
#include <alloca.h>
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("%s <trace-dir>\n", argv[0]);
        exit(0);
    }
    RecordReader reader(argv[1], SECTION_MODEL, nullptr, true, false, false);

    auto sampling_traces = reader.get_all_sampling_traces();
    for (auto& trace : sampling_traces)
    {
         int pass = 0;
            int i = 0;
            for (auto it = trace.second->begin(); it != trace.second->end(); it = it.next())
            {
                if (it.val()->timestamps.exit != 100 + i)
                {
                    // spdlog::error("timestamps not match: {} != {}", it.val()->timestamps.exit, 100+i);
                    exit(-1);
                }
                i++;
                pass += 1;
            }
            if (pass != 10)
            {
                // spdlog::error("record number not match: {} != {}", pass, 10);
                exit(-1);
            }
            // spdlog::info("pass sampling test");
            break;
    }
    auto etraces = reader.get_all_etraces();

    for (auto& [rank, etrace]: etraces)
    {
        for (int i = 0; i < 3; ++i)
        {
            auto it = etrace->begin(i);
            int j = 0;
            while (it != etrace->end(i))
            {
                char buffer[256] = {0};
                for (int u = 0; u < j + 1; ++u)
                {
                    buffer[u] = 'a' + j;
                }
                it.get();
                if (it.record_size() != j + 1)
                {
                    // spdlog::error("size not match: {} != {}", it.record_size(), j + 1);
                    exit(-1);
                }
                if (memcmp(it.get(), buffer, j + 1) != 0)
                {
                    // spdlog::error("data not match: {} != {}", std::string_view((char*)it.get(), (char*)it.get()+j+1), buffer);
                    exit(-1);
                }
                j++;
                ++it;
            }
            if (j != 10)
            {
                // spdlog::error("record number not match: {} != {}", j, 10);
                exit(-1);
            }
        }
        // spdlog::info("pass ext test");
        break;
    }


    

}