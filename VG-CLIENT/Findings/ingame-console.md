# Retained rendering addresses

The in-game console has been removed from the source, build, and component lifecycle. These addresses are retained for reference; no renderer or keyboard hooks are installed. All entries are RVAs relative to the game module base.

| Symbol | Signature file offset | Signature RVA | Resolved target RVA |
| --- | --- | --- | --- |
| `EndRenderCommandList` | `0x03FD0EC0` | `0x03FD18C0` | `0x03FD18C0` |
| `AddBaseDrawTextCmd` | `0x03FCD762` | `0x03FCE162` | `0x03FC70A0` |
| `R_TextWidth` | `0x03F0E900` | `0x03F0F300` | `0x03F0F300` |
| `R_RegisterFont` | `0x03F0CE85` | `0x03F0D885` | `0x03F0E9A0` |
| `UI_FillRectPhysical` | `0x042E7AD0` | `0x042E84D0` | `0x042E84D0` |
| `scrPlaceFull_realViewportSize` | `0x049B635C` | `0x049B6D5C` | `0x11934C28` |
| `sharedUiInfo_assets` | `0x042EBC90` | `0x042EC690` | `0x15E6D4D0` |

See `ingame-console.json` for the original signatures, file offsets, target prefixes, and disassembly evidence.
