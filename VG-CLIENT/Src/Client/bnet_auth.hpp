#include "../common.hpp"
#pragma once

namespace s4 {
enum class CoDAuthState : std::int32_t {
    AUTHSTATE_NONE = 0,
    AUTHSTATE_UNK1 = 1,
    AUTHSTATE_SUCCESS = 2,
    AUTHSTATE_FAILURE = 3,
};
 
 
struct BattleNetAuth {
    CoDAuthState state_;  
    std::uint8_t reserved_[0x2F4 - sizeof(CoDAuthState)];
    bool finished_auth_;  
};
static_assert(offsetof(BattleNetAuth, state_) == 0);
static_assert(offsetof(BattleNetAuth, finished_auth_) == 0x2F4);
static_assert(sizeof(CoDAuthState) == 4);
}
