#include "../common.hpp"
#include "patches.hpp"
#include "logger.hpp"
#include "offsets.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>

#include "Utils/hook.hpp"

void patches()
{
    static std::mutex mutex;
    const std::lock_guard<std::mutex> lock(mutex);

    struct Patch {
        std::uintptr_t address;
        std::array<std::uint8_t, 5> expected;
    };
    const std::array<Patch, 2> sites{{
        {s4::offsets::Resolve(s4::offsets::LoadRegistry_Call.site_rva),
         {0xE8, 0xB6, 0x0C, 0x00, 0x00}},
        {s4::offsets::Resolve(s4::offsets::ValidateRegion_Call.site_rva),
         {0xE8, 0x5D, 0x02, 0x00, 0x00}},
    }};
    constexpr std::array<std::uint8_t, 5> nops{0x90, 0x90, 0x90, 0x90, 0x90};

    for (const auto& site : sites) {
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQuery(reinterpret_cast<void*>(site.address), &info, sizeof(info)) ||
            info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) ||
            site.address < reinterpret_cast<std::uintptr_t>(info.BaseAddress) ||
            site.address - reinterpret_cast<std::uintptr_t>(info.BaseAddress) > info.RegionSize ||
            site.expected.size() > info.RegionSize - (site.address - reinterpret_cast<std::uintptr_t>(info.BaseAddress)))
            throw std::runtime_error("Auth patch: CALL site is not readable");
        const auto* bytes = reinterpret_cast<const void*>(site.address);
        if (std::memcmp(bytes, site.expected.data(), site.expected.size()) != 0 &&
            std::memcmp(bytes, nops.data(), nops.size()) != 0) {
            throw std::runtime_error("patches: expected call bytes do not match");
        }
    }

    utils::hook::nop(sites[0].address, 5, true);
    S4Log("Auth patch: LoadRegistry CALL RVA 0x%08X -> 5 NOP bytes", s4::offsets::LoadRegistry_Call.site_rva);  
    utils::hook::nop(sites[1].address, 5, true);
    S4Log("Auth patch: ValidateRegion CALL RVA 0x%08X -> 5 NOP bytes", s4::offsets::ValidateRegion_Call.site_rva);  
     
}
