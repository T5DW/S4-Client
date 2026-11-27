#include "../../common.hpp"
#include "../logger.hpp"
#include "systemlink_lobby.hpp"
#include "../offsets.hpp"
#include "../Utils/hooks.h"

namespace s4::patches::systemlink_lobby {
namespace {
utils::hook::detour live_get_game_party_hook;
std::mutex mutex;
bool installed = false;
PartyData* live_get_game_party_stub() {
    // Route Systemlink bot/party operations to the local lobby party.
    if (is_in_lobby()) return reinterpret_cast<PartyData*>(offsets::Resolve(offsets::g_partyData));
    return live_get_game_party_hook.invoke<s4::PartyData*>();
}
}
bool is_in_lobby() {
    using Function = bool (*)();
    return reinterpret_cast<Function>(offsets::Resolve(offsets::Live_IsInSystemlinkLobby_Call.target_rva))();
}
void install() {
    const std::lock_guard<std::mutex> lock(mutex);
    if (installed) return;
    const auto address = offsets::Resolve(offsets::Live_GetGameParty);
    static constexpr unsigned char expected[] = {0xE9,0x8B,0x18,0x16,0xFF};
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) ||
        address < reinterpret_cast<std::uintptr_t>(info.BaseAddress) ||
        address - reinterpret_cast<std::uintptr_t>(info.BaseAddress) > info.RegionSize ||
        sizeof(expected) > info.RegionSize - (address - reinterpret_cast<std::uintptr_t>(info.BaseAddress)) ||
        std::memcmp(reinterpret_cast<const void*>(address), expected, sizeof(expected)))
        throw std::runtime_error("Systemlink party getter differs from the dump");
    live_get_game_party_hook.create(address, live_get_game_party_stub);
    installed = true;
    S4Log("Auth hook installed: Live_GetGameParty; systemlink uses g_partyData");
}
void shutdown() {
    const std::lock_guard<std::mutex> lock(mutex);
    live_get_game_party_hook.clear();
    installed = false;
    S4Log("Auth: systemlink party getter hook removed");
}
}
