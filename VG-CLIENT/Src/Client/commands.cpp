#include "../common.hpp"
#include "commands.hpp"
#include <cctype>
#include <charconv>
#include "fov.hpp"

namespace client::commands {
namespace {
std::string lower(std::string_view text) {
    std::string out(text);
    for (auto& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}
std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}
}
Command parse(std::string_view input) {
    input = trim(input);
    if (input.empty()) return {Kind::Empty, {}};
    const auto split = input.find_first_of(" \t");
    const auto verb = lower(input.substr(0, split));
    const auto argument = split == std::string_view::npos ? std::string_view{} : trim(input.substr(split + 1));
    if (verb == "cg_fov" || verb == "cg_fovscale") {
        if (argument.empty()) return {Kind::Help, {}};
        float value = 0;
        const auto parsed = std::from_chars(argument.data(), argument.data()+argument.size(), value);
        const auto parameter = verb == "cg_fov" ? fov::Parameter::View : fov::Parameter::Scale;
        if (parsed.ec != std::errc{} || parsed.ptr != argument.data()+argument.size() || !fov::valid(parameter, value))
            return {Kind::Unknown, {}};
        return {parameter == fov::Parameter::View ? Kind::Fov : Kind::FovScale, {}, value};
    }
    if (verb == "unlockall") {
        if (argument.empty()) return {Kind::Help, {}};
        if (argument != "0" && argument != "1") return {Kind::Unknown, {}};
        return {Kind::UnlockAll, std::string(argument)};
    }
    if (verb == "player_name") {
        if (argument.empty()) return {Kind::Help, {}};
        if (argument.size() > 35) return {Kind::Unknown, {}};
        for (const unsigned char c : argument) if (c < 32 || c == 127) return {Kind::Unknown, {}};
        return {Kind::PlayerName, std::string(argument)};
    }
    if (verb == "menu") {
        if (argument.empty()) return {Kind::Help, {}};
        for (const unsigned char c : argument) if (c < 32 || c == 127) return {Kind::Unknown, {}};
        const auto target = lower(argument);
        return {Kind::Menu, target == "mainmenu offline" ? "MainMenuOffline" : std::string(argument)};
    }
    if (!argument.empty()) return {Kind::Unknown, {}};
    if (verb == "offline") return {Kind::Menu, "MainMenuOffline"};
    if (verb == "lua_reload") return {Kind::LuaReload, {}};
    if (verb == "auth") return {Kind::Auth, {}};
    if (verb == "help") return {Kind::Help, {}};
    return {Kind::Unknown, {}};
}
}
