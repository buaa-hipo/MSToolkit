#include <fsl/fsl_lib.h>
#include <stdlib.h>
#include <assert.h>
#include <fsl/fsl_block.h>
#include <string.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <omp.h>
constexpr int DATA_LEN = 80;
constexpr int section_num = 100'000;
char buf1[DATA_LEN];
char buf2[DATA_LEN];
using namespace pse;
int main()
{
    std::filesystem::remove("a.txt");
    std::filesystem::remove("a.txt.lock");
    spdlog::set_pattern("[%Y-%m-%d %T][thread %t][%l]%v");
    omp_set_num_threads(32);
    // spdlog::set_level(spdlog::level::debug);
    {
        for (int i = 0; i < DATA_LEN; ++i)
        {
            buf1[i] = 'u';
        }
        //         // fsl::FileSectionLayerDriver driver("a.txt");
        fsl::BlockManager manager("a.txt", ral::WRITE);
        auto root = manager.openRootSection();
#pragma omp parallel for
        for (int j = 1; j < section_num; ++j)
        {
            // #pragma omp critical
            {
                auto dataSection = manager.openDirSection(root, j, true);
                auto dataSection2 = manager.openDataSection(dataSection, 100, true);
                manager.writeDataSection(dataSection2, 123, buf1, DATA_LEN);
                manager.writeDataSection(dataSection2, 143, buf1, DATA_LEN);
            }
        }
    }
    spdlog::info("write finish");
    {
        // fsl::FileSectionLayerDriver driver("a.txt");
        fsl::BlockManager manager("a.txt", ral::READ);
        auto root = manager.openRootSection();
#pragma omp parallel for
        for (int j = 1; j < section_num; ++j)
        {
            auto dataSection = manager.openDirSection(root, j, false);
            auto dataSection2 = manager.openDataSection(dataSection, 100, false);
            if (j % 100 == 0)
            {
                spdlog::info("{}", j);
            }
            manager.readDataSection(dataSection2, 123, buf2, DATA_LEN);
            manager.readDataSection(dataSection2, 143, buf2, DATA_LEN);
            assert(strncmp(buf1, buf2, 11) == 0);
        }
    }

    spdlog::info("test passed");
}