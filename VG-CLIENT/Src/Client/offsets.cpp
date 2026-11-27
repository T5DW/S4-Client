#include "../common.hpp"
#include "offsets.hpp"

namespace s4::offsets {

 
const std::uint32_t Live_GetGameParty = 0x035A3550;
const std::uint32_t g_partyData = 0x0F566B48;

 
const SignatureMatch BattleNetAuth_StateSuccess_Match{0x03D2458B, 0x03D24F8B};
const SignatureMatch BattleNetAuth_FinishedFlag_Match{0x03D22DE5, 0x03D237E5};

 
const SignatureMatch luaL_registerXHash_Match{0x04167FF0, 0x041689F0};
const std::uint32_t luaL_registerXHash = 0x041689F0;

const SignatureMatch Dvar_RegisterBool_Match{0x01FF74B1, 0x01FF7EB1};
const DirectCall Dvar_RegisterBool_Call{0x01FF7EB1, 0x03251C60};

const SignatureMatch Live_IsInSystemlinkLobby_Match{0x01F626B3, 0x01F630B3};
const DirectCall Live_IsInSystemlinkLobby_Call{0x01F630B3, 0x035A9490};

const SignatureMatch LiveStorage_StatsInit{0x027A31D0, 0x027A3BD0};

const SignatureMatch something_1_match{0x032A9EC0, 0x032AA8C0};
const DirectCall something_1_call{0x032AA8D4, 0x02538040};

const SignatureMatch s_isContentEnumerationFinished_Match{0x03246C58, 0x03247658};
const std::uint32_t s_isContentEnumerationFinished = 0x095712D0;
const DirectCall ContentEnumerationCheck_Call{0x03247666, 0x03FC67B0};

const SignatureMatch LUI_OpenMenu_Match{0x042E8520, 0x042E8F20};
const std::uint32_t LUI_OpenMenu = 0x042E8F20;
const DirectCall LUI_OpenMenu_Call{0x0356E567, 0x042E8F20};

const SignatureMatch unkw_Get_UserData_Match{0x02537640, 0x02538040};
const std::uint32_t unkw_Get_UserData = 0x02538040;

const SignatureMatch LoadRegistry_Match{0x03D204C5, 0x03D20EC5};
const DirectCall LoadRegistry_Call{0x03D20EC5, 0x03D21B80};

 
const SignatureMatch ValidateRegion_Match{0x03D2066E, 0x03D2106E};
const DirectCall ValidateRegion_Call{0x03D2106E, 0x03D212D0};

const SignatureMatch BNet_Begin_Match{0x03D298D9, 0x03D2A2D9};
const DirectCall BNet_Begin_Call{0x03D2A2D9, 0x03D23180};

const std::uint32_t BattleNetAuth_BeginAuth = 0x03D23180;

 
const SignatureMatch LocalUserData_GetByController_Match{0x02537640, 0x02538040};
const std::uint32_t LocalUserData_GetByController = 0x02538040;

const SignatureMatch controllerStatData_SourceStride_Match{0x027A4E3D, 0x027A583D};
const std::uint32_t offset_controllerStatData_playerStats_unk_fetched = 0x00022D54;

 
const SignatureMatch controllerStatData_FetchedFlag_Match{0x027A4ED3, 0x027A58D3};
const std::uint32_t offset_controllerStatData_playerStats = 0x100F40A0;
 
const std::uint32_t controllerStatData_playerStatsBase = 0x100D1350;
const std::uint32_t controllerStatData_ControllerStride = 0x000431C4;

 
const SignatureMatch Live_IsConnectedToDemonware_Match{0x03546C5F, 0x0354765F};
const DirectCall Live_IsConnectedToDemonware_Call{0x0354765F, 0x035A6050};
const std::uint32_t Live_IsConnectedToDemonware = 0x035A6050;
const SignatureMatch Live_UserSignedInToLive_Match{0x032A9EC0, 0x032AA8C0};
const std::uint32_t Live_UserSignedInToLive = 0x032AA8C0;

const std::optional<std::uint32_t> GamerProfile_LogInProfile = std::nullopt;

const SignatureMatch PlatformAuth_GetXUID_Match{0x032A9D36, 0x032AA736};
const DirectCall PlatformAuth_GetXUID_Call{0x032AA736, 0x03D2D040};
const std::uint32_t PlatformAuth_GetXUID = 0x03D2D040;

const SignatureMatch lua_pushboolean_Match{0x06F384A0, 0x06F38EA0};
const std::uint32_t lua_pushboolean = 0x06F38EA0;

 
const SignatureMatch Live_CanUserPlayOnline_Match{0x062E8106, 0x062E8B06};
const DirectCall Live_CanUserPlayOnline_Call{0x062E8B06, 0x032A9500};
const std::uint32_t Live_CanUserPlayOnline = 0x032A9500;
const SignatureMatch PlatformAuth_GetUsername_Match{0x032A9163, 0x032A9B63};
const DirectCall PlatformAuth_GetUsername_Call{0x032A9B63, 0x03D2C3B0};
const std::uint32_t PlatformAuth_GetUsername = 0x03D2C3B0;
const SignatureMatch PlatformAuth_Frame_Match{0x03D298C0, 0x03D2A2C0};
const std::uint32_t PlatformAuth_Frame = 0x03D2A2C0;

}  
