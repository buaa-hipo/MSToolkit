#include <fsl/fsl_lib.h>
#include <ral/backend.h>
#include <fsl/raw_backend.h>
#include <filesystem>

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
struct Dummy
{
    int64_t time;
};
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        spdlog::error("Usage: {} <filename> <section>", argv[0]);
        return 1;
    }
    spdlog::set_level(spdlog::level::info);

    auto backend = pse::fsl::RawSectionBackend(argv[1], pse::ral::RWMode::READ);
    auto wrapper = pse::ral::BackendWrapper(std::move(backend));

    auto parent = wrapper.openRootSection();
    string path = argv[2];
    auto sections = split(path, "/");
    std::unique_ptr<pse::ral::DataSectionInterface> dataSec = nullptr;
    pse::ral::SectionBase::SectionType type;
    for (auto section_id : sections)
    {
        type = parent->getSectionType(std::stoi(section_id));
        if (type == pse::ral::SectionBase::SectionType::DIR)
        {
            parent = parent->openDirSection(std::stoi(section_id), false);
        }
        else if (type == pse::ral::SectionBase::SectionType::DATA)
        {
            dataSec = std::move(parent->openDataSection<Dummy>(std::stoi(section_id), false, 0, 0));
            parent = nullptr;
            break;
        }
        else
        {
            spdlog::error("Invalid section type");
            return 1;
        }
    }
    if (parent)
    {
        for (auto iter = parent->begin(); iter != parent->end(); ++iter)
        {
            spdlog::info("[desc]: {}, [isa dir]: {}", iter.getDesc(), iter.isa(pse::ral::SectionBase::DIR));
        }
    }
    else if (dataSec)
    {
        spdlog::info("sec size: {}", dataSec->size() * sizeof(Dummy));
    }
}
// int main(int argc, char *argv[])
// {
//     if (argc != 3)
//     {
//         spdlog::error("Usage: {} <filename> <section>", argv[0]);
//         return 1;
//     }
//     spdlog::set_level(spdlog::level::debug);
//     BlockManager bm(argv[1], pse::ral::READ);
//     auto parent = bm.openRootSection();
//     string path = argv[2];
//     auto sections = split(path, "/");
//     pse::fsl::DataSectionDescBlock *dataSec = nullptr;
//     for (auto section_id : sections)
//     {
//         auto section = bm.openSection(parent, std::stoi(section_id), false, true, 0);
//         parent = Block::dyn_cast<DirSectionDescBlock>(section);
//         if (!parent)
//         {
//             dataSec = Block::dyn_cast<DataSectionDescBlock>(section);
//             break;
//         }
//     }
//     if (parent)
//     {
//         for (auto id : parent->direct)
//         {
//             if (id.desc == 0)
//             {
//                 break;
//             }
//             spdlog::info("[desc, id]: {} {}", id.desc, id.blockId);
//         }
//     }
//     else
//     {
//         spdlog::info("sec size: {}", dataSec->sectionSize);
//         for (auto id : dataSec->direct)
//         {
//             spdlog::info("[id]: {}", id);
//         }
//     }
// }