#include "../common.hpp"
#include "auth_extensions.hpp"
#include <charconv>
#include <iomanip>
#include <sstream>

namespace auth {
namespace {
void Validate(const LocalProfile& profile) {
    if (profile.name.empty() || profile.name.size() > 35 || profile.xuid == 0)
        throw std::invalid_argument("Profile needs a 1..35-byte name and nonzero 64-bit XUID");
    for (const unsigned char c : profile.name)
        if (c < 32 || c == 127) throw std::invalid_argument("Player name contains a control character");
}
}
LocalProfile ParseProfile(std::string_view text) {
    std::istringstream input{std::string(text)};
    std::string header, name_key, xuid_key, xuid_text;
    LocalProfile result{};
    std::getline(input, header);
    if (!header.empty() && header.back() == '\r') header.pop_back();
    if (header != "S4_PROFILE_V1" ||
        !(input >> name_key >> std::quoted(result.name) >> xuid_key >> std::quoted(xuid_text)) ||
        name_key != "name" || xuid_key != "xuid")
        throw std::invalid_argument("Invalid vg-assets/profile.cfg format");
    if (xuid_text.size() >= 2 && xuid_text[0] == '0' && (xuid_text[1] == 'x' || xuid_text[1] == 'X'))
        xuid_text.erase(0, 2);
    if (xuid_text.empty() || xuid_text.size() > 16) throw std::invalid_argument("Invalid profile XUID length");
    const auto parsed = std::from_chars(xuid_text.data(), xuid_text.data()+xuid_text.size(), result.xuid, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != xuid_text.data()+xuid_text.size())
        throw std::invalid_argument("Invalid profile XUID");
    input >> std::ws;
    if (!input.eof()) throw std::invalid_argument("Unexpected profile fields");
    Validate(result);
    return result;
}
std::string SerializeProfile(const LocalProfile& profile) {
    Validate(profile);
    std::ostringstream output;
    output << "S4_PROFILE_V1\nname " << std::quoted(profile.name) << "\nxuid \""
           << std::hex << std::setfill('0') << std::setw(16) << profile.xuid << "\"\n";
    return output.str();
}
}
