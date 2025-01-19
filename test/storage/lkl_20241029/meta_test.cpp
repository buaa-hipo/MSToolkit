#include "record/record_meta.h"
#include <cstdint>
#include <fsl/raw_backend.h>
#include <filesystem>
using namespace pse;
bool has_error = 0;

void write_meta_test()
{
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::WRITE);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    auto root = backend.openRootSection();
    auto dir = root->openDirSection(1, true);

    RecordMeta *meta = new RecordMeta(dir, false);
    meta->sectionStart("MPI_COMM_WORLD");
    meta->metaRaw<int64_t>("MPI_COMM_WORLD", 114514);
    meta->metaRaw<int>("rank", 0);
    meta->metaRaw<int>("size", 1);
    meta->sectionEnd("MPI_COMM_WORLD");

    meta->sectionStart("Host Info");
    meta->metaRaw<std::string>("HOSTNAME", "BUAA");
    meta->sectionEnd("Host Info");

    puts("====================================================");
}
void read_meta_test()
{
    auto raw_backend = pse::fsl::RawSectionBackend("test.db", pse::ral::RWMode::READ);
    auto backend = ral::BackendWrapper(std::move(raw_backend));
    auto root = backend.openRootSection();
    auto dir = root->openDirSection(1, true);

    RecordMeta *meta = new RecordMeta(dir);
	MetaDataMap::MetaValue_t *t;
	const std::unordered_map<std::string, MetaDataMap *> & metaMap = meta->getMetaMap();
	metaMap.at("MPI_COMM_WORLD")->get("MPI_COMM_WORLD", &t);
    assert((uint64_t)t->i64 == 114514);
	metaMap.at("MPI_COMM_WORLD")->get("rank", &t);
    assert((int)t->i32 == 0);
	metaMap.at("MPI_COMM_WORLD")->get("size", &t);
    assert((int)t->i32 == 1);
	metaMap.at("Host Info")->get("HOSTNAME", &t);
    assert(t->ptr == "BUAA");
}

int main()
{
    write_meta_test();
    read_meta_test();
    if (!has_error)
    spdlog::info("pass");
    else
    spdlog::error("failed");
    std::filesystem::remove("test.db");
    std::filesystem::remove("test.db.lock");
}
