#include "logger.hpp"
#include "../common.hpp"
#include "postload.hpp"
#include "auth.hpp"
#include "patches.hpp"
#include "bnet_auth.hpp"
#include "stats.hpp"
#include "lua.hpp"
#include "stats_memory.hpp"
#include "auth_extensions.hpp"
#include "name.hpp"
#include "offsets.hpp"
#include "Utils/hooks.h"

void LUI_OpenMenu(const char* menu)
{
    if (!menu || !*menu) return;
    std::string display(menu);
    std::string key(display);
    for (auto& c : key) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if (key == "systemlinklobby") display = "SystemLink";
    S4Log("Opening %s", display.c_str());
	auto func = reinterpret_cast<void* (*)(int a1, const char* menu, int a3, int a4, int a5)>(s4::offsets::Resolve(s4::offsets::LUI_OpenMenu));  
	func(0, menu, 0, 0, 0);
}

bool Get_PlatformUsername_Detour(int controller, char* a2, size_t a3)
{
    (void)controller;
    if (!a2 || !a3) return false;
    strncpy_s(a2, a3, name::get_player_name(), _TRUNCATE);
    return true;
}

bool ForceTrue()
{
    return true;
}

bool PlatformAuth_GetXUID(int controller, XUID* output)
{
    if (!output) return false;
    using Function = bool (*)(int, XUID*);
    return reinterpret_cast<Function>(s4::offsets::Resolve(s4::offsets::PlatformAuth_GetXUID))(controller, output);
}

bool Get_PlatformXUID_Detour(int controller, XUID* a2)
{
    (void)controller;
    if (!a2) return false;
    XUID xuid;
    xuid.m_id = auth::local_profile().xuid;
    *a2 = xuid;
    return true;
}

bool IsContentEnumerationFinished()
{
    const auto address = s4::offsets::Resolve(s4::offsets::s_isContentEnumerationFinished);
    return *reinterpret_cast<const volatile std::uint8_t*>(address) != 0;
}

#if S4_LOCAL_XUID_OVERRIDE
utils::hook::detour PlatformAuth_GetXUID_Hook;
#endif

void GamerProfile_LogInProfile(int controller)
{
    const auto rva = s4::offsets::GamerProfile_LogInProfile;
    if (!rva) {
        S4Log("GamerProfile_LogInProfile unresolved for this build; skipped.\n");
        return;
    }
    reinterpret_cast<void(*)(int)>(s4::offsets::Resolve(*rva))(controller);
}

void PostLoad()
{
    S4Log("Auth: applying post-load patches");
    patches();
}

utils::hook::detour BattleNetAuth_BeginAuth_Hook;
namespace {
utils::hook::detour platform_auth_frame_hook;
utils::hook::detour platform_auth_username_hook;
utils::hook::detour live_can_user_play_online_hook;
utils::hook::detour live_connected_hook;
bool hooks_installed = false;

bool LiveCanPlayOnlineStub(int controller, std::int32_t* failure_reason)
{
    (void)controller;
    if (failure_reason) *failure_reason = 0;
    return true;
}
}

void Wow()
{ 
    SignIn();
}

void OpenMenu()
{
	LUI_OpenMenu("MainMenuOffline");  
 
}

static std::atomic<bool> is_initialized{false};
static std::recursive_mutex auth_mutex;
static bool auth_initializing = false;

void StatsInit()
{
    auto* offset_controllerStatData_playerStats =
        s4::stats::Flags(0, static_cast<std::uint32_t>(StatsSource::STATS_ONLINE));
     
    const auto* offset_controllerStatData_playerStats_unk_fetched =
        &s4::offsets::offset_controllerStatData_playerStats_unk_fetched;

    const auto apply_flags = [=]
    {
        offset_controllerStatData_playerStats->stats_fetched_ = true;
        offset_controllerStatData_playerStats->online_stat_write_pending_ = false;
        offset_controllerStatData_playerStats->allow_saving_progress_ = true;

        CLASS_VALUE_AT(offset_controllerStatData_playerStats, *offset_controllerStatData_playerStats_unk_fetched, bool) = true;
    };

    S4Log("Auth stats: applying fetched=true, write-pending=false, allow-saving=true");
    apply_flags();
    S4Log("Auth stats: calling LiveStorage_StatsInit(0, true, false, STATS_ONLINE)");
    LiveStorage_StatsInit(0, true, false, StatsSource::STATS_ONLINE);
     
    apply_flags();
     
    S4Log("Auth stats: native initialization returned; requested flags restored");
    auth::notify_stats_initialized();
    S4Log("Auth stats: callbacks complete");
}

namespace {
void RunAuthInitialization(bool manual)
{
    const std::lock_guard<std::recursive_mutex> lock(auth_mutex);
    if (auth_initializing) return;
    const bool already_initialized = is_initialized.load();
    if (!already_initialized) {
         
         
        if (!auth::get_local_user_data()) {
            if (manual) S4Log("auth: local user data is not ready; setup will retry from the game auth frame.\n");
            return;
        }
        auth_initializing = true;
        struct Reset { ~Reset() { auth_initializing = false; } } reset;
        try {
            S4Log("Auth: local controller data ready; beginning local initialization");
            using NativeSignIn = bool (*)(int, char**);
            char* disconnect_message = nullptr;
            const auto native_sign_in = reinterpret_cast<NativeSignIn>(
                s4::offsets::Resolve(s4::offsets::Live_UserSignedInToLive));
            S4Log("Auth: calling native local sign-in check");
            if (native_sign_in(0, &disconnect_message))
                S4Log("Native sign-in reported a condition; continuing local profile setup.\n");
            auth::setup_local_user_data();
            S4Log("Auth: username, XUID and local sign-in state assigned");
             
            auto* s_isContentEnumerationFinished = reinterpret_cast<bool*>(
                s4::offsets::Resolve(s4::offsets::s_isContentEnumerationFinished));
            if (s_isContentEnumerationFinished) {
                *s_isContentEnumerationFinished = true;
                S4Log("Auth: content enumeration marked finished at RVA 0x%08X", s4::offsets::s_isContentEnumerationFinished);
            } else S4Log("Auth: content-enumeration pointer unavailable");
            StatsInit();
            PostLoad();
            GamerProfile_LogInProfile(0);
            Wow();
            S4Log("Auth: local SignIn completed");
            is_initialized = true;
        } catch (const std::exception& e) {
            S4Log("auth initialization failed: %s\n", e.what());
            return;
        }
    }
    if (manual || !already_initialized) {
        S4Log("AUTH BYPASSED\n");
        fflush(stdout);
    }
}
}

namespace {
void PlatformAuthFrameStub()
{
    static thread_local bool inside = false;
    if (inside) { platform_auth_frame_hook.invoke<void>(); return; }
    inside = true;
    struct Reset { bool& value; ~Reset() { value = false; } } reset{inside};
    try { RunAuthInitialization(false); }
    catch (const std::exception& e) { S4Log("Platform frame setup failed: %s\n", e.what()); }
    platform_auth_frame_hook.invoke<void>();
}

void ClearAuthHooks() noexcept
{
    const auto clear = [](auto& hook) {
        try { hook.clear(); }
        catch (const std::exception& e) { S4Log("Hook cleanup failed: %s\n", e.what()); }
    };
    clear(platform_auth_frame_hook);
    clear(BattleNetAuth_BeginAuth_Hook);
    clear(live_connected_hook);
#if S4_LOCAL_XUID_OVERRIDE
    clear(PlatformAuth_GetXUID_Hook);
#endif
    clear(platform_auth_username_hook);
    clear(live_can_user_play_online_hook);
    hooks_installed = false;
}
}

void BattleNetAuth_BeginAuth_Detour(void* _this) {  
    if (!_this) {
        S4Log("Auth: BattleNet begin received a null object; skipped");
        return;
    }

    auto* auth = static_cast<s4::BattleNetAuth*>(_this);
    auth->state_ = s4::CoDAuthState::AUTHSTATE_SUCCESS;
    auth->finished_auth_ = true;
    static std::atomic<bool> reported{false};
    if (!reported.exchange(true)) S4Log("Auth: BattleNet state=AUTHSTATE_SUCCESS (2), completion=true");
     
     
}

void AuthCommand()
{
    try {
        S4Log("Auth: command/startup initialization requested");
        InitHooks();
        PatchSystemlinkLobby();
        RunAuthInitialization(true);
    } catch (const std::exception& e) {
        S4Log("auth command failed: %s\n", e.what());
    }
}

void InitHooks()
{
    const std::lock_guard<std::recursive_mutex> lock(auth_mutex);
    if (hooks_installed) return;
    S4Log("Auth: loading local profile and validating game targets");
    (void)auth::local_profile();
    const auto check = [](std::uint32_t rva, std::initializer_list<unsigned char> expected) {
        const auto address = s4::offsets::Resolve(rva);
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) ||
            info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) ||
            address + expected.size() > reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize ||
            memcmp(reinterpret_cast<const void*>(address), expected.begin(), expected.size()))
            throw std::runtime_error("Auth hook bytes do not match; game may not be unpacked or build differs");
        S4Log("Auth: validated target RVA 0x%08X", rva);
    };
    check(s4::offsets::Live_CanUserPlayOnline, {0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x20});
    check(s4::offsets::PlatformAuth_GetUsername, {0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x20});
#if S4_LOCAL_XUID_OVERRIDE
    check(s4::offsets::PlatformAuth_GetXUID, {0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xDA});
#endif
    check(s4::offsets::Live_IsConnectedToDemonware, {0x48,0x83,0xEC,0x28,0xE8,0xA7,0xCF,0x79,0x00});
    check(s4::offsets::BattleNetAuth_BeginAuth, {0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x20});
     
    check(s4::offsets::BattleNetAuth_StateSuccess_Match.rva, {0xC7,0x06,0x02,0x00,0x00,0x00});
    check(s4::offsets::BattleNetAuth_FinishedFlag_Match.rva, {0x80,0xBB,0xF4,0x02,0x00,0x00,0x00});
    check(s4::offsets::PlatformAuth_Frame, {0x48,0x83,0xEC,0x28,0xE8,0xB7,0xCA,0xFF,0xFF});
    check(s4::offsets::Live_UserSignedInToLive, {0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x74,0x24,0x20});
    try {
        live_can_user_play_online_hook.create(s4::offsets::Resolve(s4::offsets::Live_CanUserPlayOnline), LiveCanPlayOnlineStub);
        S4Log("Auth hook installed: Live_CanUserPlayOnline");
        platform_auth_username_hook.create(s4::offsets::Resolve(s4::offsets::PlatformAuth_GetUsername), Get_PlatformUsername_Detour);
        S4Log("Auth hook installed: PlatformAuth_GetUsername");
#if S4_LOCAL_XUID_OVERRIDE
        PlatformAuth_GetXUID_Hook.create(s4::offsets::Resolve(s4::offsets::PlatformAuth_GetXUID), Get_PlatformXUID_Detour);
        S4Log("Auth hook installed: PlatformAuth_GetXUID");
#endif
        live_connected_hook.create(s4::offsets::Resolve(s4::offsets::Live_IsConnectedToDemonware), ForceTrue);
        S4Log("Auth hook installed: Live_IsConnectedToDemonware");
        BattleNetAuth_BeginAuth_Hook.create(s4::offsets::Resolve(s4::offsets::BattleNetAuth_BeginAuth), BattleNetAuth_BeginAuth_Detour);
        S4Log("Auth hook installed: BattleNetAuth_BeginAuth");
        platform_auth_frame_hook.create(s4::offsets::Resolve(s4::offsets::PlatformAuth_Frame), PlatformAuthFrameStub);
        S4Log("Auth hook installed: PlatformAuth_Frame");
        hooks_installed = true;
    } catch (...) {
        S4Log("Auth: hook installation failed; rolling back");
        ClearAuthHooks();
        throw;
    }
}

void StopAuthHook()
{
    const std::lock_guard<std::recursive_mutex> lock(auth_mutex);
    ClearAuthHooks();
    S4Log("Auth: hooks removed");
}
