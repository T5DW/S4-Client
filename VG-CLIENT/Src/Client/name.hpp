#include "../common.hpp"
#pragma once
#include "auth_extensions.hpp"
namespace name {
inline const char* get_player_name() { return auth::local_profile().name.c_str(); }
}
