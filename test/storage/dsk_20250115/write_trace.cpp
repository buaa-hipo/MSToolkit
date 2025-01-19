#include "record/record_defines.h"
#include "record/record_type.h"
#include "record/record_type_info.h"
#include "record/record_writer.h"
#include <alloca.h>
int main()
{
        for (int i = 0; i < 10; ++i)
        {
            record_t* buf = (record_t*)alloca(record_info[i].size);
            buf->MsgType = i;
            buf->timestamps.enter = 10 - i;
            buf->timestamps.exit = 10 - i;
            RecordWriter::traceStore(buf);
        }
}