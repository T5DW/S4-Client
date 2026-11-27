#include "../common.hpp"
#include "ownership_fence.hpp"
#include "logger.hpp"
#include "offsets.hpp"
#include "lua.hpp"
#include "Lua/runtime_offsets.hpp"
#include "platform_detour.hpp"

namespace ownership_fence {
namespace {
s4::hooks::Detour timeout_hook;
std::mutex mutex;
bool installed = false;
void check(std::uintptr_t address, const unsigned char* expected, std::size_t size) {
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
        throw std::runtime_error("Ownership timeout hook target is not readable");
    const auto base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    if (address < base || address-base > info.RegionSize || size > info.RegionSize-(address-base) ||
        std::memcmp(reinterpret_cast<void*>(address), expected, size))
        throw std::runtime_error("Ownership timeout hook target differs from the verified dump");
}
}
void install() {
    const std::lock_guard<std::mutex> lock(mutex);
    if (installed) return;
    const auto target = s4::offsets::Resolve(s4::offsets::LUI_CoD_LuaCall_HasOwnershipStatusFenceTimedOut);
    static constexpr unsigned char prefix[] = {
        0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0xE8,0xA2,0xBD,0x7F,0xFB,0x0F,0xB6};
    check(target, prefix, sizeof(prefix));
    const auto& push = lua::offsets::lua_pushboolean_prefix;
    check(s4::offsets::Resolve(s4::offsets::lua_pushboolean), push.data(), push.size());
    // A Lua C callback must push the boolean and then return its result count.
    timeout_hook.create(reinterpret_cast<void*>(target), reinterpret_cast<void*>(&Lua_ReturnTrue));
    installed = true;
    S4Log("Auth hook installed: Fences.HasOwnershipStatusFenceTimedOut RVA 0x%08X -> Lua true",
        s4::offsets::LUI_CoD_LuaCall_HasOwnershipStatusFenceTimedOut);
}
void shutdown() {
    const std::lock_guard<std::mutex> lock(mutex);
    timeout_hook.clear();
    installed = false;
}
}
