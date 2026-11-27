#include "../common.hpp"
#pragma once

#include <cstdint>

namespace s4::offsets {

struct SignatureMatch {
    std::uint32_t file_offset;
    std::uint32_t rva;
};

struct DirectCall {
    std::uint32_t site_rva;
    std::uint32_t target_rva;
};

extern const std::uint32_t Live_GetGameParty;
extern const std::uint32_t g_partyData;

extern const SignatureMatch Dvar_RegisterBool_Match;
extern const DirectCall Dvar_RegisterBool_Call;

extern const SignatureMatch Live_IsInSystemlinkLobby_Match;
extern const DirectCall Live_IsInSystemlinkLobby_Call;

extern const SignatureMatch LiveStorage_StatsInit;

extern const SignatureMatch something_1_match;
extern const DirectCall something_1_call;

extern const SignatureMatch s_isContentEnumerationFinished_Match;
extern const std::uint32_t s_isContentEnumerationFinished;
extern const DirectCall ContentEnumerationCheck_Call;

 
extern const SignatureMatch LUI_OpenMenu_Match;
extern const std::uint32_t LUI_OpenMenu;  
extern const DirectCall LUI_OpenMenu_Call;
extern const SignatureMatch unkw_Get_UserData_Match;
extern const std::uint32_t unkw_Get_UserData;

 
std::uintptr_t Resolve(std::uint32_t rva);
std::uintptr_t LUI_OpenMenu_Address();

extern const SignatureMatch LoadRegistry_Match;
extern const DirectCall LoadRegistry_Call;
extern const SignatureMatch ValidateRegion_Match;
extern const DirectCall ValidateRegion_Call;
extern const SignatureMatch BNet_Begin_Match;
extern const DirectCall BNet_Begin_Call;

extern const std::uint32_t BattleNetAuth_BeginAuth;
extern const SignatureMatch BattleNetAuth_StateSuccess_Match;
extern const SignatureMatch BattleNetAuth_FinishedFlag_Match;

extern const SignatureMatch LocalUserData_GetByController_Match;
extern const std::uint32_t LocalUserData_GetByController;
extern const SignatureMatch controllerStatData_SourceStride_Match;
extern const SignatureMatch controllerStatData_FetchedFlag_Match;
 
extern const std::uint32_t offset_controllerStatData_playerStats_unk_fetched;
 
extern const std::uint32_t offset_controllerStatData_playerStats;
extern const std::uint32_t controllerStatData_playerStatsBase;
extern const std::uint32_t controllerStatData_ControllerStride;
extern const SignatureMatch Live_IsConnectedToDemonware_Match;
extern const DirectCall Live_IsConnectedToDemonware_Call;
extern const std::uint32_t Live_IsConnectedToDemonware;
extern const SignatureMatch Live_UserSignedInToLive_Match;
extern const std::uint32_t Live_UserSignedInToLive;

extern const std::optional<std::uint32_t> GamerProfile_LogInProfile;

extern const SignatureMatch PlatformAuth_GetXUID_Match;
extern const DirectCall PlatformAuth_GetXUID_Call;
extern const std::uint32_t PlatformAuth_GetXUID;

extern const SignatureMatch luaL_registerXHash_Match;
extern const std::uint32_t luaL_registerXHash;

extern const SignatureMatch lua_pushboolean_Match;
extern const std::uint32_t lua_pushboolean;

extern const SignatureMatch Live_CanUserPlayOnline_Match;
extern const DirectCall Live_CanUserPlayOnline_Call;
extern const std::uint32_t Live_CanUserPlayOnline;
extern const SignatureMatch PlatformAuth_GetUsername_Match;
extern const DirectCall PlatformAuth_GetUsername_Call;
extern const std::uint32_t PlatformAuth_GetUsername;
extern const SignatureMatch PlatformAuth_Frame_Match;
extern const std::uint32_t PlatformAuth_Frame;

}  
