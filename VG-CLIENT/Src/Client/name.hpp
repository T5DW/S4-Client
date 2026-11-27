#include "../common.hpp"
#pragma once
#include "auth_extensions.hpp"
namespace name {
inline const char* get_player_name() { thread_local std::string value; value = auth::local_profile().name; return value.c_str(); }
}
