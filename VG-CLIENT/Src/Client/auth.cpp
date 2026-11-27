#include "../common.hpp"
#include "logger.hpp"
#include "auth.hpp"
#include "offsets.hpp"

void PatchSystemlinkLobby()
{
    static std::mutex mutex;
    const std::lock_guard<std::mutex> lock(mutex);
    const auto live_is_in_systemlink_lobby = s4::offsets::Resolve(
        s4::offsets::Live_IsInSystemlinkLobby_Call.target_rva);
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(reinterpret_cast<void*>(live_is_in_systemlink_lobby), &info, sizeof(info)) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) ||
        live_is_in_systemlink_lobby < reinterpret_cast<std::uintptr_t>(info.BaseAddress) ||
        live_is_in_systemlink_lobby - reinterpret_cast<std::uintptr_t>(info.BaseAddress) > info.RegionSize ||
        3 > info.RegionSize - (live_is_in_systemlink_lobby - reinterpret_cast<std::uintptr_t>(info.BaseAddress)))
        throw std::runtime_error("Auth systemlink patch: target is not readable");
    const auto* bytes = reinterpret_cast<const unsigned char*>(live_is_in_systemlink_lobby);
    if (bytes[2] != 0xC3 || !((bytes[0] == 0x32 && bytes[1] == 0xC0) ||
                             (bytes[0] == 0xB0 && bytes[1] == 0x01)))
        throw std::runtime_error("Auth systemlink patch: target bytes differ from the dump");
    if (bytes[0] == 0xB0 && bytes[1] == 0x01) {
        S4Log("Auth patch: Live_IsInSystemlinkLobby already returns true");
        return;
    }
    DWORD old_protect = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(live_is_in_systemlink_lobby), 2,
                        PAGE_EXECUTE_READWRITE, &old_protect))
        throw std::runtime_error("Auth systemlink patch: VirtualProtect failed");
    memcpy((void*)live_is_in_systemlink_lobby, "\xB0\x01", 2);
    DWORD ignored = 0;
    const auto restored = VirtualProtect(reinterpret_cast<void*>(live_is_in_systemlink_lobby),
                                         2, old_protect, &ignored);
    const auto flushed = FlushInstructionCache(GetCurrentProcess(),
        reinterpret_cast<void*>(live_is_in_systemlink_lobby), 2);
    if (!restored || !flushed) throw std::runtime_error("Auth systemlink patch: finalization failed");
    S4Log("Auth patch: Live_IsInSystemlinkLobby RVA 0x%08X -> B0 01 (2 bytes; RET retained)",
        s4::offsets::Live_IsInSystemlinkLobby_Call.target_rva);
}

#ifndef S4_AUTH_PATCH_TEST
#include "postload.hpp"
#include "../Loader/component_loader.hpp"
#include "Lua/lua_hook.hpp"
#include "Utils/hooks.h"
#include <direct.h>
#include <sys/stat.h>
#include <filesystem>
 

utils::hook::detour LoadImageA_hk;

inline bool file_exists(const char* name)
{
    struct stat buffer;
    return (stat(name, &buffer) == 0);
}

std::string GetPath()
{
    char buffer[MAX_PATH + 1]{};

    if (_getcwd(buffer, MAX_PATH) != nullptr)
    {
        return std::string(buffer);
    }

    S4Log("Error getting current working directory!");
    return "";
}

HBITMAP WINAPI LoadImageA_Detour(
    HINSTANCE hInst,
    LPCSTR name,
    UINT type,
    int cx,
    int cy,
    UINT fuLoad)
{
    lua::hook::try_install();  
    static std::atomic<bool> once{false};
    if (!once)
    {
        try {
            if (!component_loader::post_load()) throw std::runtime_error("Client startup is not ready");
            component_loader::post_unpack();
            if (!component_loader::is_post_unpack_phase_complete())
                throw std::runtime_error("post_unpack will retry when game code is ready");
            std::filesystem::create_directories(GetPath() + "\\vg-assets\\splash");
            once = true;
            S4Log("Auth: image-load post_load/post_unpack phases complete");
        } catch (const std::exception& e) {
            static std::atomic<bool> reported{false};
            if (!reported.exchange(true)) S4Log("Auth hooks deferred: %s\n", e.what());
        }
    }

    std::string customPath =
        GetPath() + "\\vg-assets\\splash\\splash.bmp";

    if (file_exists(customPath.c_str()))
    {
        return LoadImageA_hk.stub<HBITMAP>(
            nullptr,
            customPath.c_str(),
            IMAGE_BITMAP,
            cx,
            cy,
            fuLoad | LR_LOADFROMFILE
        );
    }

    return LoadImageA_hk.stub<HBITMAP>(
        hInst,
        name,
        type,
        cx,
        cy,
        fuLoad
    );
}

void AutoAuthLoad()
{
    S4Log("Auth: installing LoadImageA hook");
    LoadImageA_hk.create(LoadImageA, LoadImageA_Detour);
    S4Log("Auth: LoadImageA hook installed");
}
void StopAuthImageHook()
{
    LoadImageA_hk.clear();
    S4Log("Auth: LoadImageA hook removed");
}

#endif
