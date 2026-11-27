# Lua autoload correction

The supplied runtime log repeatedly stopped at `waiting for the UI MenuBuilder before autoload`, without a file-loading pass. The old loader required `LUI.MenuBuilder` and recompiled that readiness probe on protected Lua calls. That blocks global-MenuBuilder builds and adds repeated work while waiting.

Autoload now runs once per initialized UI VM or explicit reload, regardless of UI table/menu availability. A completed or failed pass consumes its pending flag. The steady path forwards directly after atomic/TLS checks. Source files are cached outside the protected-call callback. File and script errors are logged once per pass, and other scripts continue.

The new default is `00_s4_systemlink_lobby.lua`. It recognizes global `MenuBuilder` and `LUI.MenuBuilder`, and adds `test` after Start Match. Deferred module/control creation is handled through module loading and bounded construction watching, with no timer or frame callback. A stock legacy `00_s4_main_menu.lua` is preserved as a disabled backup; custom edits are kept.

Native regression: 100,000 callbacks yield one pass; initialization and concurrent scheduling are checked. Lua 5.1 fixtures cover both builder names, late builder loading, user scripts executing with no UI globals, Start Match lookup by ID/text, ordering, navigation, action logging, deferred construction, and duplicate prevention. The rebuilt x64 DLL satisfies all game version.dll imports. Live FPS and button placement still require testing in the running game.
