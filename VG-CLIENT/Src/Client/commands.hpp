#include "../common.hpp"
#pragma once
namespace client::commands {
enum class Kind { Empty, Menu, PlayerName, UnlockAll, Fov, FovScale, LuaReload, Auth, Help, Unknown };
struct Command { Kind kind = Kind::Unknown; std::string menu; float value = 0; };
Command parse(std::string_view input);
}
