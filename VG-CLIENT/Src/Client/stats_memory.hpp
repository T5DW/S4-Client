#include "../common.hpp"
#pragma once
#include "offsets.hpp"
#include <cstddef>
#include <cstdint>

namespace s4::stats {
 
 
struct RuntimeFlags {
    bool stats_fetched_;
    bool online_stat_write_pending_;
    bool allow_saving_progress_;
};
static_assert(sizeof(RuntimeFlags) == 3);
static_assert(offsetof(RuntimeFlags, stats_fetched_) == 0);
static_assert(offsetof(RuntimeFlags, online_stat_write_pending_) == 1);
static_assert(offsetof(RuntimeFlags, allow_saving_progress_) == 2);

inline RuntimeFlags* Flags(std::uint32_t controller, std::uint32_t source) {
    if (source >= 2) throw std::invalid_argument("Invalid stats source");
    const auto address = offsets::Resolve(offsets::offset_controllerStatData_playerStats)
        + static_cast<std::uintptr_t>(controller) * offsets::controllerStatData_ControllerStride
        + static_cast<std::uintptr_t>(source) * offsets::offset_controllerStatData_playerStats_unk_fetched;
    return reinterpret_cast<RuntimeFlags*>(address);
}
}
