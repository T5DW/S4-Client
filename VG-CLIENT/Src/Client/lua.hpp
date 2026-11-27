#include "../common.hpp"
#pragma once

 
struct lua_State;
void lua_pushboolean(lua_State* luaVM, int value);
int Lua_ReturnTrue(lua_State* luaVM);
