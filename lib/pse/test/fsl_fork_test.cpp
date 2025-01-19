#include <fsl/fsl_lib.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <assert.h>
#include <fsl/fsl_block.h>
#include <string.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <thread>
constexpr int DATA_LEN = 8000;
constexpr int thread_num = 20;
char buf1[DATA_LEN];
char buf2[DATA_LEN];
using namespace pse;
int main()
{
    // spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %T][thread %t][%l]%v");

    std::filesystem::remove("a.txt");
    std::filesystem::remove("a.txt.lock");
    std::thread *ths = new std::thread[thread_num];
    for (int i = 0; i < thread_num; ++i)
    {
        ths[i] = std::thread([=]() {
            auto pid = fork();
            if (pid == 0)
            {
                {
                    for (int i = 0; i < DATA_LEN; ++i)
                    {
                        buf1[i] = 'u';
                    }
                    // fsl::FileSectionLayerDriver driver("a.txt");
                    fsl::BlockManager manager("a.txt", ral::WRITE);
                    auto root = manager.openRootSection();
                    auto dataSection = manager.openDirSection(root, 1, true);
                    auto dataSection2 = manager.openDataSection(dataSection, 100 + i * 2, true);
                    manager.writeDataSection(dataSection2, 123, buf1, DATA_LEN);
                    manager.writeDataSection(dataSection2, 143, buf1, DATA_LEN);
                }
                spdlog::info("write finish");
                {
                    // fsl::FileSectionLayerDriver driver("a.txt");
                    fsl::BlockManager manager("a.txt", ral::READ);
                    auto root = manager.openRootSection();
                    auto dataSection = manager.openDirSection(root, 1, false);
                    auto dataSection2 = manager.openDataSection(dataSection, 100 + i * 2, false);
                    manager.readDataSection(dataSection2, 123, buf2, DATA_LEN);
                    manager.readDataSection(dataSection2, 143, buf2, DATA_LEN);
                }

                assert(strncmp(buf1, buf2, DATA_LEN) == 0);
                spdlog::info("child test passed");
            }
            else
            {
                {
                    for (int i = 0; i < DATA_LEN; ++i)
                    {
                        buf1[i] = 'u';
                    }
                    // fsl::FileSectionLayerDriver driver("a.txt");
                    fsl::BlockManager manager("a.txt", ral::WRITE);
                    auto root = manager.openRootSection();
                    auto dataSection = manager.openDirSection(root, 1, true);
                    auto dataSection2 = manager.openDataSection(dataSection, 100 + i * 2 + 1, true);
                    manager.writeDataSection(dataSection2, 123, buf1, DATA_LEN);
                    manager.writeDataSection(dataSection2, 143, buf1, DATA_LEN);
                }
                spdlog::info("write finish");
                {
                    // fsl::FileSectionLayerDriver driver("a.txt");
                    fsl::BlockManager manager("a.txt", ral::READ);
                    auto root = manager.openRootSection();
                    auto dataSection = manager.openDirSection(root, 1, false);
                    auto dataSection2 = manager.openDataSection(dataSection, 100 + i * 2 + 1, false);
                    manager.readDataSection(dataSection2, 123, buf2, DATA_LEN);
                    manager.readDataSection(dataSection2, 143, buf2, DATA_LEN);
                }

                assert(strncmp(buf1, buf2, DATA_LEN) == 0);
                spdlog::info("test passed");
                waitpid(pid, nullptr, 0);
            }
        });
    }
    for (int i = 0; i < thread_num; ++i)
    {
        ths[i].join();
    }
    delete[] ths;
}