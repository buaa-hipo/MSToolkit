#include <fsl/fsl_lib.h>
#include <ral/backend.h>
#include <fsl/raw_backend.h>
#include <filesystem>

struct Dummy
{
    int64_t time;
};
using namespace pse::fsl;
using std::string;
using std::vector;
vector<string> split(const string &s, const string &seperator)
{
    vector<string> result;
    typedef string::size_type string_size;
    string_size i = 0;

    while (i != s.size())
    {
        //找到字符串中首个不等于分隔符的字母；
        int flag = 0;
        while (i != s.size() && flag == 0)
        {
            flag = 1;
            for (string_size x = 0; x < seperator.size(); ++x)
            {
                if (s[i] == seperator[x])
                {
                    ++i;
                    flag = 0;
                    break;
                }
            }
        }

        //找到又一个分隔符，将两个分隔符之间的字符串取出；
        flag = 0;
        string_size j = i;
        while (j != s.size() && flag == 0)
        {
            for (string_size x = 0; x < seperator.size(); ++x)
            {
                if (s[j] == seperator[x])
                {
                    flag = 1;
                    break;
                }
            }
            if (flag == 0)
            {
                ++j;
            }
        }
        if (i != j)
        {
            result.push_back(s.substr(i, j - i));
            i = j;
        }
    }
    return result;
}

void printDir(auto&& dir, int level=0)
{
        std::string level_tab = "";
        for (int i = 0; i < level; ++i)
        {
                level_tab += "\t";
        }
        spdlog::info("{}dir sec desc: {}", level_tab.c_str(), dir->self_desc());
        std::unique_ptr<pse::ral::DataSectionInterface> dataSec = nullptr;
        for (auto iter = dir->begin(); iter != dir->end(); ++iter)
        {
                if (iter.isa(pse::ral::SectionBase::DIR))
                {
                        auto child = iter.getDirSection();
                        printDir(child, level + 1);
                }
                else if(iter.isa(pse::ral::SectionBase::DATA))
                {
                        //auto child = iter.getDataSection();
                        //spdlog::info("print child {}", iter.getDesc());
                        auto child = dir->template openDataSection<Dummy>(iter.getDesc(), false, 0, 0);
                        spdlog::info("{}\tsec desc: {}, size: {}, record size: {}", level_tab, child->self_desc(), child->size(), child->record_size());
                }
        }


}
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        spdlog::error("Usage: {} <filename>", argv[0]);
        return 1;
    }
    spdlog::set_level(spdlog::level::info);

    auto backend = pse::fsl::RawSectionBackend(argv[1], pse::ral::RWMode::READ);
    auto wrapper = pse::ral::BackendWrapper(std::move(backend));

    auto parent = wrapper.openRootSection();
    printDir(parent);
}
