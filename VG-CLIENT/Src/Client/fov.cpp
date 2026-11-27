#include "../common.hpp"
#include "fov.hpp"
#include "fov_requests.hpp"
#include "logger.hpp"
#include "offsets.hpp"

namespace fov {
namespace {
detail::Requests requests;
bool readable(std::uintptr_t address, std::size_t size) {
    MEMORY_BASIC_INFORMATION info{};
    if (!address || !VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
    const auto base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    return address >= base && address-base <= info.RegionSize && size <= info.RegionSize-(address-base);
}
bool set_native(Parameter parameter, float value) {
    const auto rva = parameter == Parameter::View ? s4::offsets::cg_fov : s4::offsets::cg_fovScale;
    const auto slot = s4::offsets::Resolve(rva);
    if (!readable(slot, sizeof(std::uintptr_t))) return false;
    std::uintptr_t dvar = 0;
    std::memcpy(&dvar, reinterpret_cast<void*>(slot), sizeof(dvar));
    if (!dvar) return false; // Wait for the game's native registration.
    if (!readable(dvar, 0x16)) throw std::runtime_error("FOV dvar is not readable");
    if (*reinterpret_cast<const unsigned char*>(dvar+0x15) != 1)
        throw std::runtime_error("FOV dvar does not have the verified float type");
    const auto setter = s4::offsets::Resolve(s4::offsets::Dvar_SetFloat_Internal);
    static constexpr unsigned char expected[] = {
        0x40,0x53,0x55,0x56,0x57,0x41,0x56,0x48,0x81,0xEC,0x80,0,0,0,0x48,0x8B};
    if (!readable(setter, sizeof(expected)) ||
        std::memcmp(reinterpret_cast<void*>(setter), expected, sizeof(expected)))
        throw std::runtime_error("FOV setter differs from the verified dump");
    // Same ABI/source as the float setter call at native Lua RVA 0x0416CD3F:
    // RCX=dvar, XMM1=value, R8D=0. Native code handles protected value storage.
    using Setter = void (*)(void*, float, int);
    reinterpret_cast<Setter>(setter)(reinterpret_cast<void*>(dvar), value, 0);
    S4Log("%s %.6g: submitted to native dvar setter",
        parameter == Parameter::View ? "cg_fov" : "cg_fovscale", static_cast<double>(value));
    return true;
}
}
void request(Parameter parameter, float value) { requests.submit(parameter, value); }
void apply_pending() {
    try { requests.process(set_native); }
    catch (const std::exception& e) {
        requests.clear();
        S4Log("FOV update stopped: %s", e.what());
    }
}
void clear_pending() { requests.clear(); }
}
