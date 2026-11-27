#include "../../common.hpp"
#pragma once

#include <cstddef>
namespace utils::hook {
    void nop(void* place, std::size_t length, bool safe = false);
    void nop(std::size_t place, std::size_t length, bool safe = false);
}