# Lua runtime and loot scan

Dump SHA-256: `9e3cc7d78ea399b197c264e37514becef0eeeb65687bd48c3a1b7724126b46a3`

| Symbol | Match count | Resolved function RVA |
| --- | --- | --- |
| `Online_Loot_GetItemQuantity` | 1 | `0x041B4D30` |
| `lua_insert` | 1 | `0x06F38930` |
| `lua_pushboolean` | 1 | `0x06F38EA0` |
| `lua_pushcclosure` | 1 | `0x06F38ED0` |
| `lua_pushinteger` | 1 | `0x06F39000` |
| `lua_pushstring` | 1 | `0x06F39140` |
| `lua_pushvalue` | 1 | `0x06F39220` |
| `lua_remove` | 1 | `0x06F394B0` |
| `lua_settop` | 1 | `0x06F39840` |
| `lua_tointeger32` | 1 | `0x047EC2C0` |
| `lua_tolstring` | 1 | `0x06F39A80` |
| `luaL_loadbuffer` | 1 | `0x06F3E3D0` |
| `LuaShared_PCall` | 1 | `0x04168670` |
| `LUI_CoD_Init` | 2 | `0x048904C0` |
| `LUI_CoD_Shutdown` | 2 | `0x0489BAF0` |
| `Lua_CoD_PushFunctionToGlobalTable` | 1 | `0x062980B0` |
| `lua_loadx` | 1 | `0x06F3E960` |
| `luaL_registerXHash` | 1 | `0x041689F0` |

The two init and shutdown CALL matches converge to one function each. Every other listed pattern has one match. CALL targets use the signed rel32 displacement from the supplied executable.

The UI VM pointer at RVA `0x161C18A0` is loaded before the protected call at `0x04890B36`; it is also loaded by `LUI_OpenMenu` at `0x042E8F47`. The initialization flag at `0x162755AC` is set at `0x048905B8` and cleared at `0x048928C1`. Lua base/top offsets `0x20`/`0x28` and 8-byte values are verified by `lua_insert`/`lua_settop` disassembly.

The client hooks protected calls and UI initialization/shutdown to run autoload files on the UI Lua thread after initialization. Each file preserves the previous stack height even on a compile/runtime error. The native registration hook retains named/hash binding lookup for `lfxCall`.

The original renderer scans remain in `ingame-console.json`; their addresses are preserved in `Src/Client/Rendering/offsets.hpp`. The in-game console and its rendering/keyboard hooks are removed.

The Windows x64 DLL and all version exports/imports were checked. Native tests cover Lua wrapper arguments, script discovery, error stack restoration, loot return/rollback, logging, bridge capture, and client/profile lifecycle. Lua 5.1 fixtures cover the menu addon. Actual game rendering/menu layout has not been validated in this environment.
