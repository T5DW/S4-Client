#include "../common.hpp"
#pragma once
#include <stdexcept>
#include <string>
#include <MinHook.h>

namespace s4::hooks {
class Detour {
    void* target_ = nullptr;
    void* original_ = nullptr;
public:
    Detour() = default;
    Detour(const Detour&) = delete;
    Detour& operator=(const Detour&) = delete;
    void create(void* target, void* replacement) {
        if (target_) return;
        auto status = MH_Initialize();
        if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
            throw std::runtime_error(MH_StatusToString(status));
        status = MH_CreateHook(target, replacement, &original_);
        if (status != MH_OK) throw std::runtime_error(MH_StatusToString(status));
        target_ = target;
        status = MH_EnableHook(target_);
        if (status != MH_OK) {
            MH_RemoveHook(target_); target_ = nullptr; original_ = nullptr;
            throw std::runtime_error(MH_StatusToString(status));
        }
    }
    void* original() const { return original_; }
    void clear() {
        if (!target_) return;
        const auto status = MH_DisableHook(target_);
        if (status != MH_OK && status != MH_ERROR_DISABLED)
            throw std::runtime_error(MH_StatusToString(status));
        const auto removed = MH_RemoveHook(target_);
        if (removed != MH_OK) throw std::runtime_error(MH_StatusToString(removed));
        target_ = nullptr; original_ = nullptr;
    }
};
}
