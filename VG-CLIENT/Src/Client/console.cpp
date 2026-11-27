#include "commands.hpp"
#include "logger.hpp"
#include "../common.hpp"
#include "postload.hpp"
#include <MinHook.h>

void mh_init()
{
    S4Log("Initializing MinHook\n");
    const auto status = MH_Initialize();
    if (status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED)
        S4Log("MinHook initialized successfully\n");
    else {
        S4Log("MinHook initialization failed: %s", MH_StatusToString(status));
        MessageBoxA(0, "Failed to init minhook!", "Fatal error", MB_ICONERROR);
    }
}

HANDLE createConsole()
{
    AllocConsole();
    FILE* stream;
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    freopen_s(&stream, "CONIN$", "r", stdin);

    SetConsoleTitleA("S4-Client -V (NOT EVEN DONE!) -T5DW & XER0");
    DWORD consoleMode;
    if (GetConsoleMode(consoleHandle, &consoleMode))
        SetConsoleMode(consoleHandle, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    return consoleHandle;
}

static std::atomic<bool> console_stopping{false};
static HANDLE command_thread = nullptr;
static bool oneconsole = false;

void consoleCommandLoop()
{
    std::string input;
    while (!console_stopping && std::getline(std::cin, input)) {
        if (console_stopping) break;
        try {
            const auto command = client::commands::parse(input);
            switch (command.kind) {
            case client::commands::Kind::Empty: break;
            case client::commands::Kind::Menu: LUI_OpenMenu(command.menu.c_str()); break;
            case client::commands::Kind::Auth: AuthCommand(); break;
            case client::commands::Kind::Help:
                S4Log("Commands: offline | menu <name> | auth | help"); break;
            default: S4Log("Unknown command. Commands: offline | menu <name> | auth | help"); break;
            }
        } catch (const std::exception& error) { S4Log("Command failed: %s", error.what()); }
    }
}

static DWORD WINAPI commandLoop(LPVOID)
{
    consoleCommandLoop();
    return 0;
}

DWORD WINAPI console(LPVOID)
{
    if (oneconsole) return 0;
    oneconsole = true;
    createConsole();
    S4Log("S4-Client -V (NOT EVEN DONE!) -T5DW & XER0\n");
    mh_init();
     
     
    command_thread = CreateThread(nullptr, 0, commandLoop, nullptr, 0, nullptr);
    return command_thread ? 0 : 1;
}

void StopComponentConsole()
{
    console_stopping = true;
    if (!command_thread) return;
    for (int attempt = 0; attempt < 40; ++attempt) {
        CancelSynchronousIo(command_thread);
        if (WaitForSingleObject(command_thread, 50) == WAIT_OBJECT_0) {
            CloseHandle(command_thread);
            command_thread = nullptr;
            return;
        }
    }
     
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                      reinterpret_cast<LPCWSTR>(&StopComponentConsole), &self);
    S4Log("Console input did not stop; DLL retained until process exit.\n");
}
