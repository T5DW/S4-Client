#include "../logger.hpp"
#include "../../common.hpp"
#include "lua_hook.hpp"
#include <map>
#include <charconv>

namespace lua::hook {
namespace {
struct Binding { std::string name; bool value; BridgeCallback callback = nullptr; };
std::mutex registry_mutex;
std::map<std::string, std::map<std::uint64_t, Binding>> bindings;
std::map<std::string, std::map<std::uint64_t, BridgeCallback>> bridge_functions;
int return_true(lua_State* state) { lua_pushboolean(state, 1); return 1; }
int return_false(lua_State* state) { lua_pushboolean(state, 0); return 1; }
}
std::uint64_t detail::hash_name(std::string_view name) {
    std::uint64_t hash = 0xCBF29CE484222325ULL;
    for (unsigned char c : name) {
        if (!c) break;
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (c == '\\') c = '/';
        hash = (hash ^ c) * 0x100000001B3ULL;
    }
    return hash;
}
void set(const std::string& name, bool value) {
    const auto dot = name.find('.');
    if (dot == std::string::npos || dot == 0 || dot + 1 == name.size() ||
        name.find('.', dot + 1) != std::string::npos || name.find('\0') != std::string::npos)
        throw std::invalid_argument("Lua hook name must be Library.Function");
    std::lock_guard<std::mutex> lock(registry_mutex);
    bindings[name.substr(0, dot)][detail::hash_name(std::string_view(name).substr(dot + 1))] = {name, value};
}
void set(const std::string& name, BridgeCallback callback) {
    const auto dot = name.find('.');
    if (!callback || dot == std::string::npos || dot == 0 || dot + 1 == name.size() ||
        name.find('.', dot + 1) != std::string::npos || name.find('\0') != std::string::npos)
        throw std::invalid_argument("Lua callback hook requires Library.Function and a nonnull callback");
    std::lock_guard<std::mutex> lock(registry_mutex);
    bindings[name.substr(0, dot)][detail::hash_name(std::string_view(name).substr(dot + 1))] = {name, false, callback};
}
std::vector<detail::XHashReg> detail::prepare(const char* library, const XHashReg* entries) {
    if (!library || !entries) return {};
    std::map<std::uint64_t, Binding> selected;
    {
        std::lock_guard<std::mutex> lock(registry_mutex);
        const auto found = bindings.find(library);
        if (found == bindings.end()) return {};
        selected = found->second;
    }
    std::vector<XHashReg> copy;
     
    constexpr std::size_t max_entries = 16384;
    for (std::size_t i = 0; i < max_entries; ++i) {
        copy.push_back(entries[i]);
        auto& entry = copy.back();
        if (!entry.key || !entry.function) return copy;
        const auto found = selected.find(entry.key);
        if (found != selected.end()) {
            entry.function = found->second.callback ? found->second.callback :
                (found->second.value ? return_true : return_false);
            S4Log("Lua hook bound: %s = %s\n", found->second.name.c_str(),
                found->second.callback ? "dynamic callback" : (found->second.value ? "true" : "false"));
        }
    }
    throw std::runtime_error("Lua registration table has no terminator within limit");
}
}

namespace lua::hook {
void detail::capture(const char* library, const XHashReg* entries) {
    if (!library || !entries) return;
    std::map<std::uint64_t, BridgeCallback> captured;
    for (std::size_t i = 0; i < 16384; ++i) {
        if (!entries[i].key || !entries[i].function) {
            const std::lock_guard<std::mutex> lock(registry_mutex);
            auto& functions = bridge_functions[library];
            for (const auto& entry : captured) functions[entry.first] = entry.second;
            return;
        }
        captured[entries[i].key] = entries[i].function;
    }
    throw std::runtime_error("Lua bridge registration table has no terminator");
}
BridgeCallback get_lua_bridge_function(const std::string& name) {
    const auto dot = name.find('.');
    if (dot == std::string::npos || !dot || dot+1 == name.size() || name.find('.',dot+1) != std::string::npos) return nullptr;
    const auto function = std::string_view(name).substr(dot+1);
    std::uint64_t hash = 0;
    if (function[0] == '#') {
        if (function.size() == 1 || function.size() > 17) return nullptr;
        const auto parsed = std::from_chars(function.data()+1, function.data()+function.size(), hash, 16);
        if (parsed.ec != std::errc{} || parsed.ptr != function.data()+function.size()) return nullptr;
    } else hash = detail::hash_name(function);
    const std::lock_guard<std::mutex> lock(registry_mutex);
    const auto library = bridge_functions.find(name.substr(0,dot));
    if (library == bridge_functions.end()) return nullptr;
    const auto found = library->second.find(hash);
    return found == library->second.end() ? nullptr : found->second;
}
void clear_lua_bridge_functions() {
    const std::lock_guard<std::mutex> lock(registry_mutex); bridge_functions.clear();
}
}
