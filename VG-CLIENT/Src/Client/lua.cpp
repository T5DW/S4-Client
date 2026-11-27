#include "../common.hpp"
#include "lua.hpp"
#include "offsets.hpp"

void lua_pushboolean(lua_State* luaVM, int value)
{
    if (!luaVM) throw std::invalid_argument("lua_pushboolean requires a valid game Lua state");
    using Function = void (*)(lua_State*, int);
    const auto function = reinterpret_cast<Function>(
        s4::offsets::Resolve(s4::offsets::lua_pushboolean));
    function(luaVM, value);
}

int Lua_ReturnTrue(lua_State* luaVM)
{
    lua_pushboolean(luaVM, 1);
    return 1;
}
