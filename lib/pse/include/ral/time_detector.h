#pragma once
#include "utils/compile_time.h"
namespace pse
{
namespace ral
{
using namespace std::literals;
// template <typename T>
// struct TimeAccessor : public utils::GetField<T,
//                                              [] { return "time"sv; },
//                                              [] { return "record.timestamps.entry"sv; },
//                                              [] { return "timestamps.entry"sv; }>
// {
// };
template <typename T>
using TimeAccessor = utils::GetField<T,
                                     [] { return "time"sv; },
                                     [] { return "record.timestamps.enter"sv; },
                                     [] { return "timestamps.enter"sv; }>;

} // namespace ral

} // namespace pse
