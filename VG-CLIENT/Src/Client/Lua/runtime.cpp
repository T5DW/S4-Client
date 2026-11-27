#include "../../common.hpp"
#include "runtime.hpp"
#include "runtime_offsets.hpp"
#include "lua_vm.hpp"
#include "lua_hook.hpp"
#include "autoload.hpp"
#include "autoload_gate.hpp"
#include "../logger.hpp"
#include "../offsets.hpp"
#include "../platform_detour.hpp"
#include "main_menu_addon.hpp"
#include <filesystem>

namespace lua::runtime {
namespace {
s4::hooks::Detour protected_call_hook, init_hook, shutdown_hook;
std::mutex install_mutex;
bool installed = false, install_reported = false;
std::atomic<bool> stopping{true};
autoload::Gate autoload_gate;
std::atomic<unsigned> init_depth{0};
const volatile unsigned char* initializing_vm = nullptr;
std::mutex cache_mutex;
std::shared_ptr<const std::vector<autoload::Script>> script_cache;
std::recursive_mutex state_mutex;
lua_State* loaded_state = nullptr;
bool bridges_ready = false;
thread_local bool inside = false;
using ProtectedCall = int (*)(lua_State*, int, int);

bool readable(const void* pointer, std::size_t size) {
    MEMORY_BASIC_INFORMATION info{};
    if (!pointer || !VirtualQuery(pointer, &info, sizeof(info)) || info.State != MEM_COMMIT ||
        (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
    const auto start = reinterpret_cast<std::uintptr_t>(pointer), base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    return start >= base && start-base <= info.RegionSize && size <= info.RegionSize-(start-base);
}
template<typename T> T* address(std::uint32_t rva) { return reinterpret_cast<T*>(s4::offsets::Resolve(rva)); }
template<std::size_t N> void validate(std::uint32_t rva, const std::array<unsigned char, N>& expected) {
    const auto* target = address<unsigned char>(rva);
    if (!readable(target, N) || std::memcmp(target, expected.data(), N))
        throw std::runtime_error("Lua runtime entry differs from the supplied retail dump");
}
struct StackRestore {
    lua_State* state;
    int top;
    explicit StackRestore(lua_State* value) : state(value), top(vm::get_stack_size(value)) {}
    ~StackRestore() { vm::set_stack_size(state, top); }
};
void reset_state() {
    loaded_state = nullptr; bridges_ready = false;
}
int log_bridge(lua_State* state) {
    try {
        std::string message;
        for (int i = 1, count = vm::get_stack_size(state); i <= count; ++i) {
            if (i > 1) message += " ";
            message += vm::get_std_string(state, i);
        }
        S4Log("Lua: %s", message.c_str());
    } catch (const std::exception& error) { S4Log("Lua logging failed: %s", error.what()); }
    return 0;
}
int call_bridge(lua_State* state) {
    try {
        const int count = vm::get_stack_size(state);
        if (!count) return 0;
        hook::BridgeCallback callback = nullptr;
        {
            const auto name = vm::get_std_string(state, 1);
            callback = hook::get_lua_bridge_function(name);
            if (!callback) { S4Log("Lua bridge: function unavailable: %s", name.c_str()); return 0; }
        }
        // Removing the name leaves exactly the original argument list visible.
        vm::remove_stack_value(state, 1);
        return callback(state);
    } catch (const std::exception& error) { S4Log("Lua bridge call failed: %s", error.what()); return 0; }
}
bool execute(lua_State* state, std::string_view code, const char* name, bool report_failure = true) {
    const auto result = vm::run_script(state, code, name);
    if (result.status && report_failure)
        S4Log("Lua load failed: %s (status %d): %s", name, result.status, result.error.c_str());
    return result.status == 0;
}
void prepare_scripts() {
    const std::lock_guard<std::mutex> lock(cache_mutex);
    const auto directory = std::filesystem::current_path()/"vg-assets"/"lua"/"autoload";
    autoload::ensure_directory(directory, assets::SystemlinkAddon, assets::LegacyMainMenuAddon,
        [](const auto& message) { S4Log("Lua autoload: %s", message.c_str()); });
    auto scripts = autoload::discover(directory, [](const auto& error) { S4Log("Lua autoload: %s", error.c_str()); });
    std::shared_ptr<const std::vector<autoload::Script>> snapshot =
        std::make_shared<const std::vector<autoload::Script>>(std::move(scripts));
    std::atomic_store(&script_cache, std::move(snapshot));
}
void bootstrap(lua_State* state) {
    if (!readable(state, 0x30)) throw std::runtime_error("The game Lua state is not readable");
    const std::lock_guard<std::recursive_mutex> lock(state_mutex);
    if (loaded_state != state) { reset_state(); loaded_state = state; }
    const StackRestore restore(state);
    if (!bridges_ready) {
        vm::push_global(state, "S4ClientLog", log_bridge);
        vm::push_global(state, "lfxCall", call_bridge);
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                          reinterpret_cast<LPCWSTR>(&log_bridge), &self);
        bridges_ready = true;
        S4Log("Lua: S4ClientLog and lfxCall bridges installed");
    }
    // Autoload is independent of LUI/MenuBuilder and the active menu name.
    // Explicit registration also lets existing on-disk addons assign this declared key.
    (void)execute(state, assets::RuntimePrelude, "@S4-Client/runtime.lua");
    const auto scripts = std::atomic_load(&script_cache);
    if (!scripts) { S4Log("Lua: no prepared autoload snapshot; use lua_reload to retry"); return; }
    S4Log("Lua: loading %zu autoload file(s) from vg-assets/lua/autoload", scripts->size());
    std::size_t passed = 0;
    for (const auto& script : *scripts) {
        if (execute(state, script.code, script.name.c_str())) { ++passed; S4Log("Lua loaded: %s", script.name.c_str()); }
    }
    S4Log("Lua autoload complete: %zu loaded, %zu failed", passed, scripts->size()-passed);
}

int protected_call(lua_State* state, int args, int results) {
    // Steady state: no memory queries, locks, script compilation or file access.
    if (!stopping && !inside && autoload_gate.pending() && state &&
        autoload_gate.begin(init_depth.load(std::memory_order_relaxed) != 0 ||
                            (initializing_vm && *initializing_vm))) {
        inside = true;
        try { bootstrap(state); }
        catch (const std::exception& error) {
            S4Log("Lua autoload failed: %s. Use lua_reload to retry.", error.what());
        }
        inside = false;
    }
    return reinterpret_cast<ProtectedCall>(protected_call_hook.original())(state, args, results);
}
void init(bool frontend, bool recovery) {
    ++init_depth;
    struct EndInit { ~EndInit() { --init_depth; } } end_init;
    { const std::lock_guard<std::recursive_mutex> lock(state_mutex); reset_state(); }
    hook::clear_lua_bridge_functions();
    S4Log("Lua: UI VM initializing (frontend=%d, recovery=%d)", frontend ? 1 : 0, recovery ? 1 : 0);
    reinterpret_cast<void(*)(bool,bool)>(init_hook.original())(frontend, recovery);
    try { prepare_scripts(); }
    catch (const std::exception& error) { S4Log("Lua snapshot preparation failed: %s", error.what()); }
    autoload_gate.queue();
    S4Log("Lua: UI VM initialized; autoload queued without a MenuBuilder/menu dependency");
}
void quit() {
    ++init_depth;
    struct EndShutdown { ~EndShutdown() { --init_depth; } } end_shutdown;
    { const std::lock_guard<std::recursive_mutex> lock(state_mutex); reset_state(); }
    autoload_gate.queue();
    S4Log("Lua: UI VM shutdown; autoload state cleared");
    reinterpret_cast<void(*)()>(shutdown_hook.original())();
    hook::clear_lua_bridge_functions();
}
}
bool try_install() noexcept {
    const std::lock_guard<std::mutex> lock(install_mutex);
    if (installed) return true;
    try {
        validate(offsets::luaL_loadbuffer, offsets::luaL_loadbuffer_prefix);
        validate(offsets::lua_settop, offsets::lua_settop_prefix);
        validate(offsets::lua_tolstring, offsets::lua_tolstring_prefix);
        validate(offsets::lua_insert, offsets::lua_insert_prefix);
        validate(offsets::lua_remove, offsets::lua_remove_prefix);
        validate(offsets::lua_pushvalue, offsets::lua_pushvalue_prefix);
        validate(offsets::lua_pushcclosure, offsets::lua_pushcclosure_prefix);
        validate(offsets::Lua_CoD_PushFunctionToGlobalTable, offsets::Lua_CoD_PushFunctionToGlobalTable_prefix);
        validate(offsets::LuaShared_PCall, offsets::LuaShared_PCall_prefix);
        validate(offsets::LUI_CoD_Init, offsets::LUI_CoD_Init_prefix);
        validate(offsets::LUI_CoD_Shutdown, offsets::LUI_CoD_Shutdown_prefix);
        initializing_vm = address<unsigned char>(offsets::InitializingVM);
        if (!readable(const_cast<const unsigned char*>(initializing_vm), 1)) throw std::runtime_error("Lua VM initialization flag is unreadable");
        prepare_scripts();
        autoload_gate.queue();
        protected_call_hook.create(address<void>(offsets::LuaShared_PCall), reinterpret_cast<void*>(&protected_call));
        init_hook.create(address<void>(offsets::LUI_CoD_Init), reinterpret_cast<void*>(&init));
        shutdown_hook.create(address<void>(offsets::LUI_CoD_Shutdown), reinterpret_cast<void*>(&quit));
        stopping = false; installed = true;
        S4Log("Lua runtime: VM calls/init/shutdown hooks installed; autoload is enabled");
        return true;
    } catch (const std::exception& error) {
        stopping = true;
        try { shutdown_hook.clear(); init_hook.clear(); protected_call_hook.clear(); } catch (...) {}
        if (!install_reported) { S4Log("Lua runtime deferred: %s", error.what()); install_reported = true; }
        return false;
    }
}
void request_reload() {
    prepare_scripts();
    autoload_gate.queue();
    S4Log("Lua: autoload snapshot refreshed; execution queued for the next UI Lua call");
}
void shutdown() {
    const std::lock_guard<std::mutex> lock(install_mutex);
    stopping = true;
    shutdown_hook.clear(); init_hook.clear(); protected_call_hook.clear();
    const std::lock_guard<std::recursive_mutex> state_lock(state_mutex);
    reset_state(); hook::clear_lua_bridge_functions(); installed = false; install_reported = false;
}
}
