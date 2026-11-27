#include "../../common.hpp"
#include "lua_vm.hpp"
#include "runtime_offsets.hpp"
#include "../offsets.hpp"
#include <limits>

namespace lua::vm {
namespace {
template<typename T> T function(std::uint32_t rva) { return reinterpret_cast<T>(s4::offsets::Resolve(rva)); }
void require_state(lua_State* state) { if (!state) throw std::invalid_argument("Lua operation requires a valid game VM"); }
}
int get_stack_size(lua_State* state) {
    require_state(state);
    std::uintptr_t base = 0, top = 0;
    const auto* bytes = reinterpret_cast<const unsigned char*>(state);
    std::memcpy(&base, bytes+0x20, sizeof(base));
    std::memcpy(&top, bytes+0x28, sizeof(top));
    // Retail Lua uses 8-byte values; checked against lua_insert and lua_settop.
    if (!base || top < base || (top-base)%8 || (top-base)/8 > static_cast<unsigned>(std::numeric_limits<int>::max()))
        throw std::runtime_error("Invalid game Lua stack layout");
    return static_cast<int>((top-base)/8);
}
void set_stack_size(lua_State* s, int n) { require_state(s); function<void(*)(lua_State*,int)>(offsets::lua_settop)(s,n); }
void pop_stack_value(lua_State* s, int n) { if (n < 0) throw std::invalid_argument("Negative Lua pop count"); set_stack_size(s,-1-n); }
void hoist_stack_value(lua_State* s, int i) { require_state(s); function<void(*)(lua_State*,int)>(offsets::lua_insert)(s,i); }
void duplicate_stack_value(lua_State* s, int i) { require_state(s); function<void(*)(lua_State*,int)>(offsets::lua_pushvalue)(s,i); }
void remove_stack_value(lua_State* s, int i) { require_state(s); function<void(*)(lua_State*,int)>(offsets::lua_remove)(s,i); }
const char* get_lstring(lua_State* s, int i, std::size_t* n) { require_state(s); return function<const char*(*)(lua_State*,int,std::size_t*)>(offsets::lua_tolstring)(s,i,n); }
std::string get_std_string(lua_State* s, int i) { std::size_t n = 0; const auto* p = get_lstring(s,i,&n); return p ? std::string(p,n) : std::string{}; }
int get_integer(lua_State* s, int i) { require_state(s); return function<int(*)(lua_State*,int)>(offsets::lua_tointeger32)(s,i); }
void push_boolean(lua_State* s, bool b) { require_state(s); function<void(*)(lua_State*,int)>(offsets::lua_pushboolean)(s,b ? 1 : 0); }
void push_integer(lua_State* s, std::int64_t n) { require_state(s); function<void(*)(lua_State*,std::int64_t)>(offsets::lua_pushinteger)(s,n); }
void push_string(lua_State* s, const char* p) { require_state(s); function<void(*)(lua_State*,const char*)>(offsets::lua_pushstring)(s,p); }
void push_c_closure(lua_State* s, int (*f)(lua_State*), int n) { require_state(s); function<void(*)(lua_State*,int(*)(lua_State*),int)>(offsets::lua_pushcclosure)(s,f,n); }
int load_buffer(lua_State* s, const char* p, std::size_t n, const char* name) { require_state(s); return function<int(*)(lua_State*,const char*,std::size_t,const char*)>(offsets::luaL_loadbuffer)(s,p,n,name); }
int load_string(lua_State* s, const char* p) { if (!p) throw std::invalid_argument("Null Lua source"); return load_buffer(s,p,std::strlen(p),"@S4-Client/inline.lua"); }
int call(lua_State* s, int args, int results) { require_state(s); return function<int(*)(lua_State*,int,int)>(offsets::LuaShared_PCall)(s,args,results); }
RunResult run_script(lua_State* state, std::string_view code, const char* name) {
    const int saved_top = get_stack_size(state);
    struct Restore {
        lua_State* state; int top;
        ~Restore() { set_stack_size(state, top); }
    } restore{state, saved_top};
    int status = load_buffer(state, code.data(), code.size(), name);
    if (!status) status = call(state, 0, 0);
    return {status, status ? get_std_string(state, -1) : std::string{}};
}
void push_global(lua_State* s, const char* name, int(*callback)(lua_State*)) { require_state(s); function<void(*)(lua_State*,const char*,int(*)(lua_State*))>(offsets::Lua_CoD_PushFunctionToGlobalTable)(s,name,callback); }
}
