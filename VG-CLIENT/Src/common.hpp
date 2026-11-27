#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
#ifdef _WIN32
#include <windows.h>
#endif
#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
 
#ifndef CLASS_VALUE_AT
#define CLASS_VALUE_AT(object, offset, type) \
    (*reinterpret_cast<type*>(reinterpret_cast<std::uintptr_t>(object) + static_cast<std::uintptr_t>(offset)))
#endif
#endif
