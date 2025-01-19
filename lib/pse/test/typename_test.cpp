#include <source_location>
#include <spdlog/spdlog.h>
#include <ral/desc_manager.h>
#include <type_traits>
#include <algorithm>
// #include <spdlog/spdlog.h>
// using namespace pse;
namespace N1
{
struct R1
{
    int a;
    int b;
};
template <int N>
struct R2
{
    int a;
    int b;
};
void fn(N1::R1 r)
{
    [](R1 r2) {
        auto source = std::source_location::current();
        // spdlog::warn("source: {:d}", source.file_name());
        spdlog::warn("source: {} ", source.function_name());
    }(R1());
    [](R2<1> r2) {
        auto source = std::source_location::current();
        // spdlog::warn("source: {:d}", source.file_name());
        spdlog::warn("source: {} ", source.function_name());
    }(R2<1>());
}

} // namespace N1
int main()
{
    spdlog::info("raw type: {}", pse::utils::typesv<N1::R2<2>>);
    spdlog::info("raw type: {}", pse::utils::typesv<int *>);
    spdlog::info("raw type: {}", pse::utils::typesv<N1::R1>);
    spdlog::info("enum str: {}", pse::utils::namesv<1>);
    spdlog::info("enum str: {}", pse::utils::namesv<pse::ral::READ>);
}
