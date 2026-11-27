# VG-CLIENT

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

`menu` is required before arbitrary menu names. Names are passed through as typed, except the documented `mainmenu offline` alias. Use a menu name present in this game build; the command does not add menus or verify that a name exists. `offline` is the direct shortcut. The old `hi` command is removed.

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
6. You type `offline` or `menu MainMenuOffline`. The command calls the matched native `LUI_OpenMenu` entry with controller 0 and your menu name. Startup never opens a menu automatically.

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
lobby predicate returns true.

The local profile is created at `vg-assets/profile.cfg` on first use and reused afterward. The splash image path is `vg-assets/splash/splash.bmp`.

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
| `Src/Client/logger.cpp` | Gold-prefix logging. |
| `Src/Client/PartyData.hpp` | Opaque current-build party declaration used by the party-getter hook. |
| `Src/Client/stats.hpp` | Stats/user-data types, including `ScenarioInstallState`. |
| `Src/Client/Lua/` | Named Lua boolean registration hooks. |
| `Src/Client/Patches/` | Systemlink-lobby component. |
| `Src/Loader/` | Component lifecycle. |
| `Src/Proxy/` | Windows version DLL forwarding and startup delay. |
| `ThirdParty/MinHook/` | Required hook dependency. |
| `Tests/` | Command, logger, Lua, stats, profile, auth, and lifecycle tests. |

The package contains the active component implementation and its required loader/proxy dependencies. Unused utility sources, old duplicate wrappers, historical scan files, and previous adaptation patches are removed. `Post.cpp` is now `postload.cpp`. Code comments are removed; third-party notices remain in `Licenses/` and the dependency license file.

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
