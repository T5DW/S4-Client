#include "../../common.hpp"
#pragma once
#include "../lua.hpp"

namespace lua::hook {
using BridgeCallback = int (*)(lua_State*);
void set(const std::string& name, bool value);
void set(const std::string& name, BridgeCallback callback);
 
bool try_install() noexcept;
void shutdown();
BridgeCallback get_lua_bridge_function(const std::string& name);
void clear_lua_bridge_functions();
namespace detail {
using Callback = int (*)(lua_State*);
struct XHashReg { std::uint64_t key; Callback function; };
static_assert(sizeof(XHashReg) == 16, "Retail x64 Lua registration layout");
void capture(const char* library, const XHashReg* entries);
std::uint64_t hash_name(std::string_view name);
 
std::vector<XHashReg> prepare(const char* library, const XHashReg* entries);
}
}
