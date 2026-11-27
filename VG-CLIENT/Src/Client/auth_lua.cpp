#include "../common.hpp"
#include "auth_lua.hpp"
#include "offsets.hpp"
#include "platform_detour.hpp"
#include "lua.hpp"
#include "Lua/runtime_offsets.hpp"
#include "logger.hpp"

namespace {
std::mutex patches2_mutex;
std::array<s4::hooks::Detour, 3> patches2_hooks;
bool patches2_installed = false;
void CheckPatches2Target(std::uintptr_t address, std::size_t size) {
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) ||
        !(info.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
        throw std::runtime_error("patches2: target is not executable; game may not be unpacked");
    const auto base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    if (address < base || address - base > info.RegionSize || size > info.RegionSize - (address - base))
        throw std::runtime_error("patches2: target crosses a memory region");
}
}

void patches2() {
    const std::lock_guard<std::mutex> lock(patches2_mutex);
    if (patches2_installed) return;
    constexpr std::array<std::uint32_t, 3> rvas{0x062F1F10, 0x062FC330, 0x06DCC680};
    constexpr std::array<const char*, 3> names{
        "Engine.IsPremiumPlayer", "Engine.IsLocalPlayAllowed", "Fences.OfflineDataFetched"};
    const auto push = s4::offsets::Resolve(s4::offsets::lua_pushboolean);
    const auto& prefix = lua::offsets::lua_pushboolean_prefix;
    CheckPatches2Target(push, prefix.size());
    if (std::memcmp(reinterpret_cast<const void*>(push), prefix.data(), prefix.size()))
        throw std::runtime_error("patches2: lua_pushboolean bytes differ from the supported build");
    for (const auto rva : rvas) CheckPatches2Target(s4::offsets::Resolve(rva), 16);
    try {
        for (std::size_t i = 0; i < rvas.size(); ++i)
            patches2_hooks[i].create(reinterpret_cast<void*>(s4::offsets::Resolve(rvas[i])),
                                     reinterpret_cast<void*>(&Lua_ReturnTrue));
    } catch (...) {
        for (auto& hook : patches2_hooks) {
            try { hook.clear(); }
            catch (const std::exception& e) { S4Log("patches2 rollback failed: %s", e.what()); }
        }
        throw;
    }
    patches2_installed = true;
    for (std::size_t i = 0; i < rvas.size(); ++i)
        S4Log("patches2: %s RVA 0x%08X -> Lua true", names[i], rvas[i]);
}

void StopPatches2() {
    const std::lock_guard<std::mutex> lock(patches2_mutex);
    for (auto& hook : patches2_hooks) hook.clear();
    patches2_installed = false;
}
