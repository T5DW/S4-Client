#include "../common.hpp"
#include "loot.hpp"
#include "logger.hpp"
#include "offsets.hpp"
#include "Lua/runtime_offsets.hpp"
#include "platform_detour.hpp"

namespace loot {
namespace {
s4::hooks::Detour quantity_hook;
std::mutex mutex;
bool installed = false;
std::atomic<bool> unlock_all{true};
// The retail entry uses a receiver in RCX and two integer arguments in EDX/R8D.
int quantity(void*, int, int) { return unlock_all.load(std::memory_order_relaxed) ? 1 : 0; }
}
void set_enabled(bool enabled) { unlock_all.store(enabled, std::memory_order_relaxed); }
bool is_enabled() { return unlock_all.load(std::memory_order_relaxed); }
void install() {
    const std::lock_guard<std::mutex> lock(mutex);
    if (installed) return;
    const auto target = s4::offsets::Resolve(lua::offsets::Online_Loot_GetItemQuantity);
    const auto& expected = lua::offsets::Online_Loot_GetItemQuantity_prefix;
    MEMORY_BASIC_INFORMATION info{};
    const auto readable = VirtualQuery(reinterpret_cast<void*>(target), &info, sizeof(info));
    const auto base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    if (!readable || info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) ||
        target < base || target-base > info.RegionSize || expected.size() > info.RegionSize-(target-base) ||
        std::memcmp(reinterpret_cast<void*>(target), expected.data(), expected.size()))
        throw std::runtime_error("Loot hook target differs from the verified dump");
    quantity_hook.create(reinterpret_cast<void*>(target), reinterpret_cast<void*>(&quantity));
    installed = true;
    S4Log("Loot: Online_Loot_GetItemQuantity RVA 0x%08X -> unlockall %d (default 1)",
        lua::offsets::Online_Loot_GetItemQuantity, is_enabled() ? 1 : 0);
}
void shutdown() {
    const std::lock_guard<std::mutex> lock(mutex);
    quantity_hook.clear(); installed = false;
}
}
