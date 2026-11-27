# Loot quantity toggle and native IsOwned offset

`unlockall 1` (default each launch): native item quantity = integer 1.
`unlockall 0`: quantity = integer 0. No setting is persisted and no server-side
inventory is changed. The original quantity calculation is not restored at 0.

The `Loot.IsOwned` Lua override has been removed. Its original native callback
is retained and recorded as `s4::offsets::LUI_Loot_IsOwned = 0x041B2370`.
No replacement detour or byte patch is installed on that callback. It calls
the quantity function, so the existing quantity hook can still affect its
result. Its native return behavior is preserved rather than forced to boolean.
The quantity callback reads the atomic toggle without needing a reload or
reinstall. Hook installation remains in `RunAuthInitialization`, after local
user data is assigned.

## Dump evidence for library/function identity

- Dump SHA: `9e3cc7d78ea399b197c264e37514becef0eeeb65687bd48c3a1b7724126b46a3`.
- Retail normalized FNV-1a `IsOwned` key: `0x0B7D304F1F31158E`.
- Registration table RVA `0x076BFB30`, IsOwned entry RVA `0x076BFB50`.
- Entry function pointer resolves to RVA `0x041B2370`.
- Registration sequence: table LEA at `0x041B21FB`, library-name LEA at
  `0x041B2205` -> RVA `0x0A8DADB4` containing `Loot`, registrar CALL at
  `0x041B220C` -> `0x041689F0` (`luaL_registerXHash`).
- Native IsOwned implementation calls `Online_Loot_GetItemQuantity` at
  `0x041B2400` -> `0x041B4D30`.

The hook registry leaves this Loot table key unchanged. Other Lua boolean
overrides keep their existing behavior. See `isowned-offset.json` for the
rechecked callback, library/table reference, and quantity call evidence.

Native tests cover the unchanged Loot registration table, default-on native
quantity and live 1/0/1 transitions, command argument validation, installation
rollback/retry, auth integration, and shutdown. In-game UI behavior has not been
tested here.
