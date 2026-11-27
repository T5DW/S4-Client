#include "../logger.hpp"
#include "../../common.hpp"
#include "lua_hook.hpp"
#include "../offsets.hpp"
#include "../platform_detour.hpp"

namespace lua::hook {
namespace {
s4::hooks::Detour registration_hook;
std::mutex install_mutex;
bool installed = false;
bool reported = false;
using Register = void (*)(lua_State*, const char*, const detail::XHashReg*);
bool matches_code(std::uintptr_t address, const unsigned char* expected, std::size_t size) {
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION memory{};
    if (!VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) ||
        memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    const DWORD protection = memory.Protect & 0xff;
    if (protection != PAGE_EXECUTE_READ && protection != PAGE_EXECUTE_READWRITE &&
        protection != PAGE_EXECUTE_WRITECOPY) return false;
    const auto region = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    if (address < region || address - region > memory.RegionSize ||
        size > memory.RegionSize - (address - region)) return false;
#endif
    return std::memcmp(reinterpret_cast<const void*>(address), expected, size) == 0;
}
void register_stub(lua_State* state, const char* library, const detail::XHashReg* entries) {
    std::vector<detail::XHashReg> copy;
    try { copy = detail::prepare(library, entries); }
    catch (const std::exception& e) { S4Log("Lua hook registration skipped: %s\n", e.what()); }

    reinterpret_cast<Register>(registration_hook.original())(state, library, copy.empty() ? entries : copy.data());
}
}
bool try_install() noexcept {
    std::lock_guard<std::mutex> lock(install_mutex);
    if (installed) return true;
    try {
        const auto address = s4::offsets::Resolve(s4::offsets::luaL_registerXHash);
        static constexpr unsigned char expected[] = {
            0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,0x24,0x10,
            0x57,0x48,0x83,0xEC,0x20,0x49,0x8B,0xD8,0x48,0x8B,0xF2};
        static constexpr unsigned char push_expected[] = {0x48,0x8B,0x41,0x28,0x45,0x33,0xC0,0x85,0xD2};
        const auto push = s4::offsets::Resolve(s4::offsets::lua_pushboolean);
        if (!matches_code(address, expected, sizeof(expected)) ||
            !matches_code(push, push_expected, sizeof(push_expected)))
            throw std::runtime_error("Lua registrar/pushboolean code differs from the verified dump");
        registration_hook.create(reinterpret_cast<void*>(address), reinterpret_cast<void*>(&register_stub));
        installed = true;
        S4Log("%s", "Lua registration hook installed; waiting for Engine/Fences bindings.");
        return true;
    } catch (const std::exception& e) {
        if (!reported) { S4Log("Lua hooks deferred: %s\n", e.what()); reported = true; }
        return false;
    }
}
void shutdown() {
    std::lock_guard<std::mutex> lock(install_mutex);
    registration_hook.clear();
    installed = false;
    reported = false;
     
     
}
}
