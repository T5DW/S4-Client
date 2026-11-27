#include "../common.hpp"
#pragma once
namespace client::commands {
enum class Kind { Empty, Menu, Auth, Help, Unknown };
struct Command { Kind kind = Kind::Unknown; std::string menu; };
Command parse(std::string_view input);
}
