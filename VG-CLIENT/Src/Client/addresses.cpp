#include "../common.hpp"
#include "offsets.hpp"
#include <stdexcept>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace s4::offsets {
std::uintptr_t Resolve(std::uint32_t rva) {
#ifdef _WIN32
    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    if (!base) throw std::runtime_error("Game executable base is unavailable");
    return base + rva;
#else
    (void)rva;
    throw std::runtime_error("Game address resolution requires Windows");
#endif
}

std::uintptr_t LUI_OpenMenu_Address() {
    return Resolve(LUI_OpenMenu);
}
}  
