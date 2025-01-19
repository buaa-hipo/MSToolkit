#include <fsl/fsl_lib.h>
#include <stdlib.h>
#include <assert.h>
#include <fsl/fsl_block.h>
#include <string.h>
#include <spdlog/spdlog.h>
#include <filesystem>
constexpr int DATA_LEN = 80;
char buf1[DATA_LEN];
char buf2[DATA_LEN];
using namespace pse;
int main()
{
    std::filesystem::remove("a.txt");
    std::filesystem::remove("a.txt.lock");
    spdlog::set_level(spdlog::level::debug);
    {
        for (int i = 0; i < DATA_LEN; ++i)
        {
            buf1[i] = 'u';
        }
        // fsl::FileSectionLayerDriver driver("a.txt");
        fsl::BlockManager manager("a.txt", ral::WRITE);
        auto root = manager.openRootSection();
        auto dataSection = manager.openDirSection(root, 1, true);
        auto dataSection2 = manager.openDataSection(dataSection, 100, true);
        manager.writeDataSection(dataSection2, 123, buf1, DATA_LEN);
        manager.writeDataSection(dataSection2, 143, buf1, DATA_LEN);
    }
    spdlog::info("write finish");
    {
        // fsl::FileSectionLayerDriver driver("a.txt");
        fsl::BlockManager manager("a.txt", ral::READ);
        auto root = manager.openRootSection();
        auto dataSection = manager.openDirSection(root, 1, false);
        auto dataSection2 = manager.openDataSection(dataSection, 100, false);
        manager.readDataSection(dataSection2, 123, buf2, DATA_LEN);
        manager.readDataSection(dataSection2, 143, buf2, DATA_LEN);
    }

    std::filesystem::remove("a.txt");
    std::filesystem::remove("a.txt.lock");
    assert(strncmp(buf1, buf2, 11) == 0);
    spdlog::info("test passed");
}