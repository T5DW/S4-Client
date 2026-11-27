#include "../../common.hpp"

#include "hook.hpp"
#include <windows.h>
#include <cstring>
#include <stdexcept>

namespace utils::hook {
void nop(void* place, const std::size_t length, bool safe) {
    (void)safe;
    if (!place || !length) throw std::invalid_argument("Invalid NOP range");
    DWORD old_protect = 0;
    if (!VirtualProtect(place, length, PAGE_EXECUTE_READWRITE, &old_protect))
        throw std::runtime_error("VirtualProtect failed before NOP patch");
    std::memset(place, 0x90, length);
    DWORD ignored = 0;
    const auto restored = VirtualProtect(place, length, old_protect, &ignored);
    const auto flushed = FlushInstructionCache(GetCurrentProcess(), place, length);
    if (!restored || !flushed) throw std::runtime_error("Cannot finalize NOP patch");
}
void nop(const std::size_t place, const std::size_t length, bool safe) {
    nop(reinterpret_cast<void*>(place), length, safe);
}
}