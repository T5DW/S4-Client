# Verified party offsets

Dump SHA-256: `9e3cc7d78ea399b197c264e37514becef0eeeb65687bd48c3a1b7724126b46a3`

All 11 patterns have one match. Addresses below are RVAs relative to the running game module base. The two globals identify data, not callable functions.

| Symbol | File offset | Match RVA | Target RVA |
| --- | --- | --- | --- |
| `Live_GetGameParty` | `0x05F6075A` | `0x05F6115A` | `0x035A3550` |
| `Party_AreWeServer` | `0x02783B64` | `0x02784564` | `0x02707E90` |
| `Party_CountAllMembersEvenIfInactive` | `0x033D5390` | `0x033D5D90` | `0x033D5D90` |
| `Party_GetGametype` | `0x02721380` | `0x02721D80` | `0x02721D80` |
| `Party_IsRunning` | `0x02739FC2` | `0x0273A9C2` | `0x02726700` |
| `PartyAtomic_AcceptInviteMP` | `0x0252FD20` | `0x02530720` | `0x03BCFA40` |
| `PartyAtomic_SetupPotentialHostForJoining` | `0x03BD1A50` | `0x03BD2450` | `0x03BD2450` |
| `PartyAtomic_StartJoin` | `0x03BD1B00` | `0x03BD2500` | `0x03BD2500` |
| `PartyHost_PartyRestrictsF2PUsers` | `0x027648B9` | `0x027652B9` | `0x027554E0` |
| `g_partyData` | `0x02723B20` | `0x02724520` | `0x0F566B48` |
| `g_partyJoinInfo` | `0x01FC24F6` | `0x01FC2EF6` | `0x0F5C0AC0` |

Constants are in `Src/Client/offsets.hpp` / `offsets.cpp`. `_PartyMatch` preserves each match location; `_PartyCall` preserves call-site and target RVAs for CALL signatures. Existing `Live_GetGameParty` and `g_partyData` values are reused and confirmed. No new party hooks are installed.
