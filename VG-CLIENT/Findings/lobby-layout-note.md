# Observed lobby hierarchy and deferred panel handling

The supplied log shows SystemLinkMenu snapshots with 62/81 nodes and SystemLinkLobby snapshots with 70 nodes. Their first child is MPTabsMenu, with Tabs below that child. The former diagnostics stopped at 32 lines amid player-details decoration. The logged nodes do not establish the Start Match control's actual ID or whether it was visible at capture time.

The addon now observes all registered factories while a tracked lobby awaits its button, so a nested tab panel built after the outer menu can receive the test button before native attachment. Direct factory calls are covered as well as BuildRegisteredType. Tracking ends on insertion, close, or a 96-construction budget. Diagnostics focus on action/tab controls and avoid unrelated player text.

The Lua 5.1 fixture models SystemLinkLobby -> MPTabsMenu -> Tabs, builds MPLobbyActions later via the registry directly, and attaches it by calling the native/class method rather than the overridden instance method. It verifies button insertion/order and owner tracking, then verifies that closed lobbies do not modify unrelated panels and that unsuccessful discovery stops at its budget. These fixtures do not constitute a live-game placement test.
