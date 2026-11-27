#include "../common.hpp"
#pragma once
void LUI_OpenMenu(const char* menu);
void PostLoad();
void OpenMenu();
void Wow();
void InitHooks();
void StopAuthHook();

void GamerProfile_LogInProfile(int controller);
void AuthCommand();

struct XUID;
bool PlatformAuth_GetXUID(int controller, XUID* output);
bool Get_PlatformXUID_Detour(int controller, XUID* output);
bool IsContentEnumerationFinished();

bool Get_PlatformUsername_Detour(int controller, char* output, size_t length);
bool ForceTrue();
