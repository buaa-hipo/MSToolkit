#include <fsl/fsl_lib.h>
#include <stdio.h>
#include <spdlog/spdlog.h>
#include <time.h>
#include <filesystem>
constexpr int thread_num = 4;
using namespace pse::fsl;
double time_diff(timeval st, timeval ed) { return (ed.tv_sec - st.tv_sec) + (ed.tv_usec - st.tv_usec) / 1e6; }
constexpr int BLK_LEN = 10 * 1024;

timeval st, ed;
void write_test(char *data, size_t len, int i)
{
    BlockManager bm("a.out", pse::ral::WRITE);
    auto root = bm.openRootSection();
    auto dataSec = bm.openDataSection(root, i + 1, true);
    gettimeofday(&st, nullptr);
    for (int i = 0; (i + 1) * BLK_LEN < len; ++i)
    {
        bm.writeDataSection(dataSec, i * BLK_LEN, data + i * BLK_LEN, BLK_LEN);
    }
    gettimeofday(&ed, nullptr);
    spdlog::info("section write speed: {:.4f}MB/s", (len / 1024.0 / 1024) / time_diff(st, ed));
}

void read_test(char *data, size_t len, int i)
{
    BlockManager bm("a.out", pse::ral::READ);
    auto root = bm.openRootSection();
    auto dataSec = bm.openDataSection(root, i + 1, false);
    gettimeofday(&st, nullptr);
    for (int i = 0; (i + 1) * BLK_LEN < len; ++i)
    {
        bm.readDataSection(dataSec, i * BLK_LEN, data + i * BLK_LEN, BLK_LEN);
    }
    gettimeofday(&ed, nullptr);
    spdlog::info("section read speed: {:.4f}MB/s", (len / 1024.0 / 1024) / time_diff(st, ed));
}

int main()
{
    std::filesystem::remove("a.out");
    std::filesystem::remove("a.out.lock");
    spdlog::set_level(spdlog::level::info);
    FILE *f = fopen("data.bin", "rb");
    fseek(f, 0, SEEK_END);
    // auto len = ftell(f);
    auto len = 1024L * 1024 * 1024 * 2;
    fseek(f, 0, SEEK_SET);
    char *data = new char[len];
    spdlog::info("data ptr {:x}", (size_t)data);

    // gettimeofday(&st, nullptr);
    // for (int i = 0; (i + 1) * BLK_LEN < len; ++i)
    // {
    //     fread(data + i * BLK_LEN, 1, BLK_LEN, f);
    // }
    // gettimeofday(&ed, nullptr);
    // spdlog::info("raw read speed: {:.4f}MB/s", (len / 1024.0 / 1024) / time_diff(st, ed));

    fclose(f);

#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < thread_num; ++i)
    {
        write_test(data, len, i);
    }
    // exit(0);
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < thread_num; ++i)
    {
        char *data2 = new char[len];
        read_test(data2, len, i);
        for (int i = 0; i < len; ++i)
        {
            if (data[i] != data2[i])
            {
                spdlog::error("read value does not equal to write value at {}", i);
                assert(0);
            }
        }
        delete[] data2;
    }
    exit(0);

    FILE *f2 = fopen("test.out", "wb");
    gettimeofday(&st, nullptr);
    for (int i = 0; (i + 1) * BLK_LEN < len; ++i)
    {
        fwrite(data + i * BLK_LEN, 1, BLK_LEN, f2);
    }
    gettimeofday(&ed, nullptr);
    fclose(f2);
    spdlog::info("raw write speed: {:.4f}MB/s", (len / 1024.0 / 1024) / time_diff(st, ed));

    spdlog::info("pass");
    delete[] data;
}
