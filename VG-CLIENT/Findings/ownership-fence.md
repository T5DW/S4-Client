# HasOwnershipStatusFenceTimedOut

Dump SHA-256: `9e3cc7d78ea399b197c264e37514becef0eeeb65687bd48c3a1b7724126b46a3`.
Addresses are RVAs relative to the loaded game module.

| Item | RVA / value |
|---|---|
| `LUI_CoD_LuaCall_HasOwnershipStatusFenceTimedOut` | `0x06DCCAB0` |
| Lua API | `Fences.HasOwnershipStatusFenceTimedOut` |
| Normalized FNV-1a function-name hash | `0x3887F2F7D3EA0512` |
| Lua registration table | `0x077E4C80` |
| Function's table entry (index 24) | `0x077E4E00` |
| Native underlying timeout predicate | `0x025C8860` |
| Native `lua_pushboolean` | `0x06F38EA0` |

Registration proof: table LEA at `0x06DCD839` targets `0x077E4C80`; library-name
LEA at `0x06DCD843` targets `0x0AD5C35C`, containing `Fences`; CALL at
`0x06DCD84A` targets the verified `luaL_registerXHash` at `0x041689F0`.
The hash table entry contains the callback's preferred-image VA.

Original callback: `0x06DCCAB9` calls the native timeout predicate, converts AL
to EDX, and `0x06DCCAC4` calls `lua_pushboolean`. The wrapper returns one result.
This identifies the Lua-facing wrapper separately from its underlying predicate.

## Requested behavior

`ownership_fence::install()` validates the callback and pushboolean prefixes,
then detours the native callback to the existing `Lua_ReturnTrue` helper. The
helper pushes boolean true onto the supplied Lua state and returns integer 1
as the Lua result count. Returning integer 1 without the push would be wrong.

Installation is idempotent and runs from `RunAuthInitialization`, after local
profile setup, alongside the party and loot installers. If installation fails,
auth remains pending for retry. Auth shutdown clears the detour. It does not
depend on the `unlockall` toggle and does not reintroduce the IsOwned override.
The underlying timeout predicate itself is not patched for other native callers.

Native tests cover callback/pushboolean code guards, hook enable rollback and
retry, one pushed true result, repeated installation, repeated cleanup, and auth
integration with all ten hooks removed on shutdown. Actual game menus remain
untested in this environment.
