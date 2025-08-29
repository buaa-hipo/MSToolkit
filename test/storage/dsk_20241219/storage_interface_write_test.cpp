#include <record/record_writer.h>
#include <record/wrap_defines.h>
#include <record/record_type.h>
int main()
{
    record_comm_t records[10];
    for (int i = 0; i < 10; ++i)
    {
        if (i < 5)
        {
            records[i].record.MsgType = event_MPI_Send;
        }
        else
        {
            records[i].record.MsgType = event_MPI_Recv;
        }
        records[i].tag = i;
        records[i].dest = i * 10;
        records[i].count = i * 100;
        records[i].record.timestamps.enter = i * 1000;
    }
    // RecordWriter::init();
    for (int i = 0; i < 10; ++i)
    {
        RecordWriter::traceStore((record_t*)&records[i]);
    }

}