# CG FOV dvars — pinned Vanguard dump

Dump SHA-256: `9e3cc7d78ea399b197c264e37514becef0eeeb65687bd48c3a1b7724126b46a3`.
All addresses below are RVAs, added to the loaded game module base.

| Dvar | Retail name hash | Global dvar pointer slot |
|---|---|---|
| `cg_fov` | `0x56E0C9E15809767A` | `0x165B0C88` |
| `cg_fovScale` | `0x08BD4585C77E2050` | `0x165B0C98` |

Names are hashed rather than stored as plaintext `cg_fov`/`cg_fovScale` strings.
The retail c-prefix hash begins at `0x6F089DD565F89E95`, then XOR/multiplies each
normalized character after the leading c by `0x10000000233`, modulo 2^64.
The hash algorithm constants were cross-checked with the S4 reference below.
Each resulting hash has two MOV RCX immediate references in this executable.

| Dvar | Hash-load RVA | Float-register CALL RVA | Return-value store RVA |
|---|---|---|---|
| cg_fov | `0x0519570D` | `0x05195758` | `0x0519575D` |
| cg_fov | `0x055D965C` | `0x055D969F` | `0x055D96A4` |
| cg_fovScale | `0x0519580E` | `0x05195825` | `0x0519582A` |
| cg_fovScale | `0x055D974F` | `0x055D9767` | `0x055D976C` |

All four calls target `0x03252180` (Dvar_RegisterFloat); both paths store each
returned RAX into the same RIP-relative slot shown above. FOV scale registration
loads default 1.0, lower bound approximately 0.2, and upper bound 2.0. These are
observed registration values, not a newly imposed client setting.

**Do not write a float to these global addresses.** They contain dvar pointers.
Both slots are zero in this snapshot; the runtime's native registration populates
them. No current-value field offset or runtime write is assumed by this change.
The console now submits bounded changes using native `Dvar_SetFloat_Internal`
at RVA `0x03253B70`. Its ABI is `(dvar pointer, float value, int source)` with
source 0, matching the native Lua float-setting call at `0x0416CD3F`. The Lua
wrapper is at `0x0416A0B0`, its implementation at `0x0416CBC0`, and its registered
SetDvarFloat name hash is `0xD14B6102841AB394`.

The setter reads the dvar type at offset `0x15`; float is 1. Its float branch at
`0x03253E69` stores the XMM1 input and enters the game's protected-value path.
The client validates the setter's first 16 bytes and the dvar float type before
calling it on the game auth frame. It does not infer an editable current-value
offset. The source tree adds no raw dvar or pointer-slot write.

Command bounds are 1..110 for `cg_fov` and 0.2..2 for `cg_fovscale`; invalid or
nonfinite values are rejected. The native domain can further clamp a value.
Requests made before dvar registration remain queued; only the latest request
per variable is kept. Successful changes are submitted once and are not saved.
Native parser/request tests pass; live in-game changes remain unverified here.

Reproduce:

```sh
python3 Tools/scan_fov.py Vanguard_dump.exe --output fov-scan.json
```

The scanner checks the exact dump SHA and the two independent registrations per
name before reporting `validated=true`. JSON evidence is in `fov-offsets.json`.

Hash reference (algorithm facts only, no implementation source copied):
https://github.com/purrpleeW/s4-mod/blob/fbae90d4d5276c4483fdcaa4a1ee4b3ed396ec2b/src/common/utils/hashing.hpp
