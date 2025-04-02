#include "record/record_defines.h"
#include "record/record_type.h"
#include "record/record_type_info.h"
#include "record/record_writer.h"
#include <alloca.h>
int main()
{
        for (int i = 0; i < 10; ++i)
        {
            record_t* buf = (record_t*)alloca(record_info[event_MPI_Init].size);
            buf->MsgType = event_MPI_Init;
            buf->timestamps.enter = i;
            buf->timestamps.exit = i + 100;
            RecordWriter::samplingStore(event_MPI_Init, buf);
        }

        for (int i = 0 ; i < 3; ++i)
        {
            for (int j = 0; j < 10; ++j)
            {
                char buffer[256];
                for (int u = 0; u < j + 1; ++u)
                {
                    buffer[u] = 'a' + j;
                }
                RecordWriter::extStore(i, buffer, j+1);
            }
        }
}