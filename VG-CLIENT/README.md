# VG-CLIENT

## Auth callback and gamertag update

`RunAuthInitialization` now installs the Demonware signed-in true hook and
`patches2()` after local user data is ready, before native sign-in. The three
user-supplied callbacks return Lua true: `Engine.IsPremiumPlayer` (0x062F1F10),
`Engine.IsLocalPlayAllowed` (0x062FC330), and `Fences.OfflineDataFetched`
(0x06DCC680). Installation validates executable memory and lua_pushboolean,
rolls back partial installs, and removes hooks at auth shutdown. These callback
RVAs have not been independently verified against a game executable here.

`GamertagHash` uses unsigned 32-bit multiplication by 31; auth profile setup
uses `Assign_UsernameAndXUID`, which assigns hashOfGamertag. Online fence and
bool dvar patch modules are absent. This update preserves this ZIP's existing
Lua runtime, registration hooks and autoload configuration. Live gameplay
has not been tested.


A Windows x64 component client for the supplied Vanguard retail BattleNet build. Offline menus and the Multiplayer systemlink lobby have been reached in user testing. The console keeps the requested S4-CLIENT log identity.

## Commands

Type a command into the client console and press Enter.

| Command | Action |
| --- | --- |
| `offline` | Calls `LUI_OpenMenu("MainMenuOffline")`. |
| `menu MainMenuOffline` | Opens the offline main menu through the same function. |
| `menu CPSystemLinkLobby` | Requests the co-op/Zombies systemlink-lobby candidate found in the dump. |
| `menu CPSystemLinkMenu` | Requests its co-op/Zombies systemlink menu counterpart. |
| `menu systemlinklobby` | Requests `systemlinklobby` and logs `Opening SystemLink`. |
| `menu mainlockoutmenu` | Requests `mainlockoutmenu`. |
| `menu mainmenu offline` | Convenience alias for `MainMenuOffline`. |
| `menu <name>` | Passes the supplied menu name to `LUI_OpenMenu`. |
| `auth` | Requests the shared local authentication setup again. |
| `help` | Prints the command list. |
| `lua_reload` | Reruns autoload scripts at the next UI Lua call. |
| `player_name <name>` | Saves the local name in `vg-assets/user/user.json`. |
| `unlockall 1` | Native loot quantity returns 1 (default). |
| `unlockall 0` | Native loot quantity returns 0. |
| `cg_fov <1..110>` | Queues a FOV value for the native float setter. |
| `cg_fovscale <0.2..2>` | Queues the FOV multiplier for the same native setter. |

`menu` is required before arbitrary menu names. Names are passed through as typed, except the documented `mainmenu offline` alias. Use a menu name present in this game build; the command does not add menus or verify that a name exists. `offline` is the direct shortcut. The old `hi` command is removed.

`unlockall` defaults to 1 each process launch and is not saved. It changes the
native quantity callback immediately, without a reload or reinstall. The
`Loot.IsOwned` Lua override has been removed; its original callback is retained
at RVA `0x041B2370` (`s4::offsets::LUI_Loot_IsOwned`). That original wrapper calls
the quantity function and may therefore observe the quantity override. Setting
0 forces quantity zero; it does not restore the native inventory calculation or
force a Lua boolean false. Existing UI caches may need a menu refresh. This does
not grant server inventory. See `Findings/unlockall.md` for the offset evidence.

## FOV scan

Verified global dvar pointer slots: `cg_fov` RVA `0x165B0C88`, `cg_fovScale` RVA
`0x165B0C98`; native float registration function RVA `0x03252180`. These globals
hold dvar pointers and are not addresses of float values. Their pointer contents
are zero in the supplied dump and are assigned during native registration.
`cg_fov 110` and `cg_fovscale 2` submit bounded values through the verified native
float setter at RVA `0x03253B70`. Console requests are applied on the game auth
frame, after native registration is ready. No float is written into a pointer
slot or into protected dvar storage directly. Out-of-range/nonfinite values are
rejected. Pending changes coalesce to the newest value per dvar; successful
requests are not reapplied each frame. Values are not saved across launches.
The engine can still clamp a request to its native domain for the current mode.
See `Findings/fov-offsets.md` and `Tools/scan_fov.py` for repeatable evidence.

## Ownership fence timeout callback

`LUI_CoD_LuaCall_HasOwnershipStatusFenceTimedOut` is registered as
`Fences.HasOwnershipStatusFenceTimedOut` at RVA `0x06DCCAB0`. Auth initialization
installs a native detour that pushes Lua boolean true and returns one result.
This is always enabled after installation and is independent of `unlockall`.
`StopAuthHook()` removes the detour. `Loot.IsOwned` remains without a direct
override. See `Findings/ownership-fence.md` for the verified registration and
call evidence. In-game menu behavior has not been verified here.

## To Skip Ownership Checks

User-tested workaround: you may notice that the Multiplayer button is locked
after entering the offline main menu. Use the client console (command prompt)
and follow this order:

1. Type `menu mainmenuoffline` in the console.
2. If the Multiplayer button is locked, type `menu systemlink`.
3. Click **Back** in the game.
4. Type `menu mainmenuoffline` in the console again.
5. Click **Zombies**. Do **not** click Multiplayer yet.
6. Wait until the Zombies menu appears, then click **Back** to return to the offline main menu.
7. Click **Multiplayer**. You should now be in the Multiplayer systemlink lobby.

## How it reaches the offline menus

1. The `version.dll` proxy forwards the game's Windows version API calls to the system DLL and starts the client console on a background worker.
2. After the console opens, the worker waits **20 seconds**, then starts the components and requests `auth` automatically.
3. The Lua registration hook substitutes boolean callbacks for `Engine.IsLocalPlayAllowed`, `Engine.IsPremiumPlayer`, and `Fences.OfflineDataFetched` when those bindings are registered.
4. The retail BattleNet replacement sets the local `CoDAuthState` to success and the current-build completion flag. The original platform-auth frame continues running.
5. When local controller data is ready, shared setup assigns the saved local name/XUID, marks content enumeration ready, sets the stats flags, runs native stats initialization, and restores the requested flags. The matched registry/region calls are NOP-patched.
6. After auth initialization succeeds, `menu MainMenuOffline` runs automatically once on the game auth frame after the native frame returns. It uses the same command handler as the console, calling `LUI_OpenMenu` with controller 0. Failed or pending initialization does not queue a menu; repeated `auth` commands do not reopen it. You can still type `offline` or `menu MainMenuOffline` manually.

Auth applies the exact two-byte patch in `Src/Client/auth.cpp`:

```cpp
memcpy((void*)live_is_in_systemlink_lobby, "\xB0\x01", 2);
```

The target is validated as the current-build `Live_IsInSystemlinkLobby` entry
(RVA `0x035A9490`) from the supplied signature. Memory protection is changed
around the write, restored, and the instruction cache flushed. The trailing
`RET` is retained. `2` is the number of copied bytes; `B0 01` means `mov al, 1`.
The dvar/client-frame toggle was removed so it cannot overwrite this auth patch.
The party-getter hook remains and returns local `g_partyData` while the patched
lobby predicate returns true. Outside Systemlink it returns
`live_get_game_party_hook.invoke<s4::PartyData*>()`, preserving the native getter.
`RunAuthInitialization(bool manual)` in `Src/Client/postload.cpp` now calls
`s4::patches::systemlink_lobby::install()` and `loot::install()` immediately after
the local username/XUID/sign-in data is assigned. Both installers are idempotent;
an installation failure leaves auth initialization pending for its next retry.
They no longer install from the independent unpack components. `StopAuthHook()`
cleans up both hooks.
The supplied signatures resolve to `Live_GetGameParty` RVA `0x035A3550` and
`g_partyData` RVA `0x0F566B48`. This routing was already implemented; the latest
check covers switching out of and back into Systemlink. Bot spawning still
requires verification in the running game.

The local profile is created at `vg-assets/user/user.json` on first use and reused afterward. The splash image path is `vg-assets/splash/splash.bmp`.

## Logging

Every client-generated console log uses the same prefix:

```text
[S4-CLIENT] // Opening SystemLink
[S4-CLIENT] // Opening MainMenuOffline
[S4-CLIENT] // AUTH BYPASSED
```

The prefix is always gold in the console, including errors. The logger uses RGB `255, 215, 0` when virtual-terminal color is available and a gold/yellow Windows console attribute fallback otherwise. Log messages are serialized so concurrent client threads do not mix their prefixes. Multi-line messages receive a prefix on each line. Command echo and output written directly by the game are outside this logger.

## Source layout

| Path | Purpose |
| --- | --- |
| `Src/Client/auth.cpp` | Image-load hook, unpack-phase entry, and exact two-byte systemlink auth patch. |
| `Src/Client/addresses.cpp` | Resolves RVAs against the current game executable base. |
| `Src/Client/patches.cpp` | Validates and applies the registry/region CALL NOP patches. |
| `Src/Client/postload.cpp` | Shared auth/stats setup, native menu wrapper, and supported game hooks. |
| `Src/Client/console.cpp` | Console lifecycle and command execution. |
| `Src/Client/commands.cpp` | Command parsing and menu aliases. |
| `Src/Client/logger.cpp` | Gold-prefix console logging and persistent file logging. |
| `Src/Client/Rendering/offsets.hpp` | Retained renderer addresses; in-game console removed. |
| `Src/Client/loot.cpp` | Validated loot-quantity hook returning 1. |
| `Src/Client/PartyData.hpp` | Opaque current-build party declaration used by the party-getter hook. |
| `Src/Client/stats.hpp` | Stats/user-data types, including `ScenarioInstallState`. |
| `Src/Client/Lua/` | Named Lua boolean registration hooks. |
| `Src/Client/Patches/` | Systemlink-lobby component. |
| `Src/Loader/` | Component lifecycle. |
| `Src/Proxy/` | Windows version DLL forwarding and startup delay. |
| `ThirdParty/MinHook/` | Required hook dependency. |
| `Tests/` | Command, logger, Lua, stats, profile, auth, and lifecycle tests. |

The package contains the active component implementation and its required loader/proxy dependencies. Unused utility sources, old duplicate wrappers, historical scan files, and previous adaptation patches are removed. `Post.cpp` is now `postload.cpp`. Implementation comments and third-party notices remain in `Licenses/` and the dependency license file.

## Build

Use Visual Studio C++ x64 tools and CMake:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Output: `build/bin/Release/version.dll`. The packaged build is `Bin/version.dll`.

The DLL links MinHook statically and uses standard Windows/UCRT and BCrypt system libraries. Offsets target the supplied dump; other game builds require matching addresses and layouts.

The native mock suite requires Python 3 and `g++` on Linux:

```sh
python3 Tests/run.py
```

The reorganized build and mock checks pass. User testing confirms reaching offline menus and the Multiplayer systemlink lobby; this does not establish that LAN sessions, bot spawning, progress saving, or every menu work. `AUTH BYPASSED` is the local client status message. `GamerProfile_LogInProfile` remains unresolved and is skipped. Lua hooks must be installed before their bindings register; late registration timing can affect them.

## Credits

T5DW & XER0. Adapted from the user-supplied S4-MOD source by xifil & luxploit. MinHook copyright holders and original S4-MOD notices are retained in `Licenses/`. Keep those notices with copies of the source or binaries.

## Zombies systemlink menu search

The supplied dump contains the paired names `CPSystemLinkLobby` /
`CPSystemLinkMenu` and `MPSystemLinkLobby` / `MPSystemLinkMenu`. The CP names are
the co-op/Zombies candidates. Try these exact commands:

```text
menu CPSystemLinkLobby
menu CPSystemLinkMenu
```

| Name | String file offset | String RVA | Menu-table pointer RVA |
| --- | --- | --- | --- |
| CPSystemLinkLobby | 0x094C6594 | 0x094C7F94 | 0x094C84F0 |
| CPSystemLinkMenu | 0x094C6574 | 0x094C7F74 | 0x094C84E8 |

These addresses identify name strings/table entries, not callable functions.
The existing menu wrapper still calls LUI_OpenMenu at RVA 0x042E8F20. Finding
these names does not verify live menu opening or change the game into Zombies
mode. See `Findings/zombies-menus.json` for dump evidence.

Auth logging now reports profile load/create, target validation, hook installs,
the exact systemlink patch, registry/region NOPs, native local-sign-in checks,
user-data setup, content-enumeration readiness, stats initialization/callbacks,
and completion/cleanup. It retains the gold `[S4-CLIENT] //` prefix. Repeated
frame callbacks do not print a line on every frame.

Only the active `VG-CLIENT/Src/Client` component source is built. The files shown
in the supplied screenshot are represented by auth.cpp/auth.hpp,
auth_extensions.hpp, bnet_auth.hpp, console.cpp/console.hpp, local_profile.cpp,
name.hpp, PartyData.hpp, postload.cpp/postload.hpp, profile_format.cpp, stats.hpp,
and stats_memory.hpp; build-required helpers and dependencies remain. No old
Post.cpp or legacy source tree is compiled.

The unused game.cpp/game.hpp menu aliases were removed. Menu commands call
LUI_OpenMenu directly from postload.cpp. addresses.cpp and patches.cpp remain
required by the active component build.

## Demonware predicate hooks

Both current-build predicates are hooked to return true:

| Predicate | Function RVA |
| --- | --- |
| Live_IsUserSignedInToDemonware | 0x035A9F60 |
| Live_IsConnectedToDemonware | 0x035A6050 |

Each supplied signature matches once. The connected predicate was already
hooked; the signed-in predicate is now included in auth setup, code validation,
rollback and cleanup. Both use a controller-index callback and log their hook
installation with the gold prefix. Scan evidence is in
`Findings/demonware-return-true.json`. These are local predicate overrides;
they do not create a remote session.

## Player profile

New profiles use `MatchedPlayer` and a random nonzero 64-bit local XUID.
The XUID is saved as a hexadecimal string in `vg-assets/user/user.json` and reused across restarts and name changes. Existing `vg-assets/profile.cfg` profiles migrate automatically if no JSON profile exists.

Type `player_name Your Name` in the console to save a name (1–35 bytes; spaces allowed). Restart the game to refresh cached player data. Platform name lookups read the saved name immediately.
`auth.cpp` creates `vg-assets`, `vg-assets/user`, and `vg-assets/splash` during initialization.

This archive contains updated source and a rebuilt Windows x64 `Bin/version.dll`.

## Lua autoload and Systemlink test button

The in-game console remains removed; the external command prompt remains available. Rendering addresses remain in `Src/Client/Rendering/offsets.hpp`.

Lua files load from `vg-assets/lua/autoload`, including subfolders and uppercase `.LUA` extensions. File order is lexical by relative path, and UTF-8 BOMs are removed. Files run once after each UI VM initialization, and `lua_reload` requests another pass. Autoload does **not** wait for `LUI`, `MenuBuilder`, a frontend menu, or the lobby to exist. It requires a live initialized Lua VM and executes on its thread.

The previous loader could stay at `waiting for the UI MenuBuilder before autoload`, recompiling a Lua probe repeatedly. That probe is removed. The protected-call hook now takes an atomic fast path after the pass finishes, without memory queries, mutex acquisition, script compilation, or file access. Directory reads and source caching happen at installation, VM initialization, or explicit reload. A failed file is logged and the other files continue; failures never trigger a per-frame retry loop.

The default addon is `vg-assets/lua/autoload/00_s4_systemlink_lobby.lua`. It supports the global `MenuBuilder` and the `LUI.MenuBuilder` variant. It inserts a native CODButton labeled **test** directly after **Start Match** in Systemlink lobby/menu variants. Clicking it writes an S4-Client log message. Existing and newly registered factories are handled. If controls are added later, the addon watches a bounded number of construction operations; it creates no retry timers or frame callbacks.

If the builder is not available when the addon executes, it waits for module loading through `require`; other autoload files continue immediately. `lua_reload` also retries addon installation. The default stock offline-menu addon from the previous release is renamed to `00_s4_main_menu.lua.disabled` when its bytes match that release. Custom edits to that old file are preserved and logged. Existing custom scripts and edits to the new default are preserved.

Use either bridge for console and persistent file logging:

```lua
S4ClientLog("my script loaded")
S4Client.Log("hello from autoload")
```

Logs append to `vg-assets/logs/S4-Client.log`. `lfxCall("Library.Function", ...)` calls a binding captured by the registration hook and logs unavailable bindings. New diagnostic messages identify the selected builder and whether the test button was inserted below Start Match.

## Loot quantity

The supplied `Online_Loot_GetItemQuantity` signature matched one CALL at RVA `0x02547EE1`, resolving to function RVA `0x041B4D30`. Its detour returns integer **1**. The entry bytes are checked before hooking. See `Findings/lua-runtime.json` for scan and VM-layout evidence.

## Validation of this update

The Windows x64 DLL is rebuilt from the included source. Native tests cover Lua script discovery, stack restoration on successful/failed execution, bridge registration, loot-hook return/rollback, command parsing, logging, and the existing client/profile lifecycle. Lua 5.1 fixtures verify global/LUI builder selection, late module loading, autoload without UI globals, Start Match ordering, controller arguments, navigation, click logging, construction watching, and duplicate prevention. A native regression test checks that 100,000 callbacks produce one autoload pass, including concurrent scheduling and initialization handling.

```sh
python3 Tests/run.py
# Optional Lua fixture, with Python package lupa installed:
python3 Tests/run_lua.py
```

The runtime hook and UI placement have not been tested in a running Vanguard instance here. If the live menu layout differs, the addon logs the missing control/API in `vg-assets/logs/S4-Client.log`.

## Party offsets

All 11 supplied signatures matched exactly once in the same dump. Function/data RVAs, match evidence, and CALL targets are declared in `Src/Client/offsets.hpp` and defined in `Src/Client/offsets.cpp`. Existing `Live_GetGameParty` and `g_partyData` RVAs were confirmed. No party function is overridden by this scan. See `Findings/party-offsets.md` and `Findings/party-offsets.json`.

```sh
python3 Tools/scan_party.py Vanguard_dump.exe --output party-scan.json
```

## Strict-global namespace correction

LUI rejects ordinary creation of undeclared globals. The loader now explicitly registers the `S4Client` namespace with `rawset`, then assigns its logging helper through a local table reference. The Systemlink addon also uses a local reference and does not issue an ordinary global `S4Client = ...` assignment. `S4Client.Log` compatibility is retained.

Replace `Bin/version.dll` and the included `vg-assets/lua/autoload/00_s4_systemlink_lobby.lua`, then restart the game. Existing on-disk scripts are intentionally preserved by the autoloader; the new loader also registers the namespace before those scripts execute.

Lua 5.1 tests reproduce `LUI Error: Tried to create global variable S4Client`, verify the former assignment fails, and verify the prelude/addon work with guarded globals. Both global/LUI MenuBuilder variants, late builder initialization, and Start Match lookup are tested under that guard. Live game UI placement remains unverified here.

## Systemlink control lookup update

Autoload success followed by `Start Match control not found` indicates that the addon executed but could not identify the live control. The lookup now handles linked children, child arrays, named control fields/collections, nested text controls, and `properties.text` localization keys. It selects the enclosing button rather than a text label, and can recover direct sibling order from named controls when native child iteration is unavailable.

Replace the included `vg-assets/lua/autoload/00_s4_systemlink_lobby.lua` and restart the game. The new script logs `Systemlink addon revision: systemlink-controls-2`. Restarting clears wrappers from older script versions. The rebuilt DLL embeds this same script for new installations.

If lookup still fails, the addon logs `Systemlink lookup:` followed by up to 32 `Systemlink control:` paths, IDs, and text previews, once per menu instance. Those lines identify the live build's actual layout; the two generic deferred messages alone do not. Discovery and diagnostics are bounded construction-time work and add no frame callback or retry timer. Live placement remains unverified here.

## Deferred Systemlink tab content

The supplied log identifies `SystemLinkMenu` / `SystemLinkLobby` with `MPTabsMenu` and `Tabs`, while the diagnostic printed before the deeper action panel was identified. The addon now tracks pending lobby roots and observes registered UI factories, including factories invoked directly rather than through `BuildRegisteredType`. A completed nested panel containing Start Match can receive the test button before native code attaches that panel to Tabs.

Discovery is limited to 96 new widget constructions per tracked lobby and stops after insertion, lobby close, or budget exhaustion. It schedules no frame callback or timer. Diagnostics prioritize Tabs/button/list/action branches and omit player details/helper decorations, with up to 96 focused nodes per diagnostic phase. They no longer cut off the useful branch behind a 32-line dump of player widgets.

Replace the DLL and `vg-assets/lua/autoload/00_s4_systemlink_lobby.lua`, then restart. The script logs `Systemlink addon revision: systemlink-panels-3`. The Lua fixtures reproduce the supplied outer-menu layout, native attachment of a later action panel, direct factory invocation, closure cleanup, and budget exhaustion. This verifies the mechanism in fixtures; live menu timing/placement remains unverified here.
