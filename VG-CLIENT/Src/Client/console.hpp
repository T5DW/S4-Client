#include "../common.hpp"
#pragma once
void LUI_OpenMenu(const char* menu);
DWORD WINAPI console(LPVOID);
void StopComponentConsole();

void ExecuteClientCommand(std::string_view input);
