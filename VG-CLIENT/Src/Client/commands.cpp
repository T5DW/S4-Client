#include "../common.hpp"
#include "commands.hpp"
#include <cctype>

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
    if (verb == "menu") {
        if (argument.empty()) return {Kind::Help, {}};
        for (const unsigned char c : argument) if (c < 32 || c == 127) return {Kind::Unknown, {}};
        const auto target = lower(argument);
        return {Kind::Menu, target == "mainmenu offline" ? "MainMenuOffline" : std::string(argument)};
    }
    if (!argument.empty()) return {Kind::Unknown, {}};
    if (verb == "offline") return {Kind::Menu, "MainMenuOffline"};
    if (verb == "auth") return {Kind::Auth, {}};
    if (verb == "help") return {Kind::Help, {}};
    return {Kind::Unknown, {}};
}
}
