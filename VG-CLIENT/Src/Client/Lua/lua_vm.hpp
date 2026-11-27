#pragma once
#include "../lua.hpp"
#include <cstdint>
#include <string>
#include <string_view>

namespace lua::vm {
inline constexpr int RegistryIndex = -10000, GlobalsIndex = -10002, MultipleResults = -1;
inline constexpr int up_value_index(int value) { return GlobalsIndex-value; }
int get_stack_size(lua_State* state);
void set_stack_size(lua_State* state, int size);
void pop_stack_value(lua_State* state, int count);
void hoist_stack_value(lua_State* state, int index);
void duplicate_stack_value(lua_State* state, int index);
void remove_stack_value(lua_State* state, int index);
const char* get_lstring(lua_State* state, int index, std::size_t* size);
std::string get_std_string(lua_State* state, int index);
int get_integer(lua_State* state, int index);
void push_boolean(lua_State* state, bool value);
void push_integer(lua_State* state, std::int64_t value);
void push_string(lua_State* state, const char* value);
void push_c_closure(lua_State* state, int (*callback)(lua_State*), int upvalues);
int load_buffer(lua_State* state, const char* data, std::size_t size, const char* name);
int load_string(lua_State* state, const char* text);
int call(lua_State* state, int args, int results);
struct RunResult { int status; std::string error; };
RunResult run_script(lua_State* state, std::string_view code, const char* name);
void push_global(lua_State* state, const char* name, int (*callback)(lua_State*));
}
