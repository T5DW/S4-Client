#include "../../common.hpp"
#pragma once
#include "../lua.hpp"

namespace lua::hook {
void set(const std::string& name, bool value);
 
bool try_install() noexcept;
void shutdown();
namespace detail {
using Callback = int (*)(lua_State*);
struct XHashReg { std::uint64_t key; Callback function; };
static_assert(sizeof(XHashReg) == 16, "Retail x64 Lua registration layout");
std::uint64_t hash_name(std::string_view name);
 
std::vector<XHashReg> prepare(const char* library, const XHashReg* entries);
}
}
