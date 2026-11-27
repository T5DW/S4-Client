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
namespace {
class JsonReader {
    std::string_view text;
    std::size_t position = 0;
public:
    explicit JsonReader(std::string_view value) : text(value) {}
    void whitespace() {
        while (position < text.size() && std::string_view(" \t\r\n").find(text[position]) != std::string_view::npos) ++position;
    }
    void expect(char c) {
        whitespace();
        if (position == text.size() || text[position++] != c)
            throw std::invalid_argument("Invalid user.json syntax");
    }
    unsigned hex4() {
        unsigned value = 0;
        for (int i = 0; i < 4; ++i) {
            if (position == text.size()) throw std::invalid_argument("Incomplete JSON escape");
            const char c = text[position++];
            const auto digit = std::string_view("0123456789abcdef").find(
                c >= 'A' && c <= 'F' ? static_cast<char>(c + ('a'-'A')) : c);
            if (digit == std::string_view::npos) throw std::invalid_argument("Invalid JSON escape");
            value = value * 16 + static_cast<unsigned>(digit);
        }
        return value;
    }
    std::string string() {
        expect('"');
        std::string result;
        while (position < text.size()) {
            const unsigned char c = text[position++];
            if (c == '"') return result;
            if (c < 32) throw std::invalid_argument("Unescaped JSON control character");
            if (c != '\\') { result += static_cast<char>(c); continue; }
            if (position == text.size()) break;
            const char escape = text[position++];
            switch (escape) {
            case '"': case '\\': case '/': result += escape; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case 'u': {
                unsigned cp = hex4();
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    if (text.substr(position, 2) != "\\u") throw std::invalid_argument("Missing JSON low surrogate");
                    position += 2;
                    const auto low = hex4();
                    if (low < 0xDC00 || low > 0xDFFF) throw std::invalid_argument("Invalid JSON low surrogate");
                    cp = 0x10000 + ((cp-0xD800) << 10) + low-0xDC00;
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) throw std::invalid_argument("Invalid JSON surrogate");
                if (cp < 0x80) result += static_cast<char>(cp);
                else {
                    if (cp >= 0x10000) result += static_cast<char>(0xF0 | (cp >> 18));
                    if (cp >= 0x800) result += static_cast<char>((cp >= 0x10000 ? 0x80 : 0xE0) | ((cp >> 12) & 0x3F));
                    result += static_cast<char>((cp >= 0x800 ? 0x80 : 0xC0) | ((cp >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (cp & 0x3F));
                }
                break;
            }
            default: throw std::invalid_argument("Invalid JSON escape");
            }
        }
        throw std::invalid_argument("Unterminated JSON string");
    }
    void end() { whitespace(); if (position != text.size()) throw std::invalid_argument("Unexpected JSON data"); }
};
LocalProfile ParseJson(std::string_view text) {
    JsonReader reader(text);
    reader.expect('{');
    LocalProfile result{};
    std::string xuid;
    bool has_name = false, has_xuid = false;
    for (int i = 0; i < 2; ++i) {
        if (i) reader.expect(',');
        const auto key = reader.string();
        reader.expect(':');
        const auto value = reader.string();
        if (key == "name" && !has_name) { result.name = value; has_name = true; }
        else if (key == "xuid" && !has_xuid) { xuid = value; has_xuid = true; }
        else throw std::invalid_argument("Unknown or duplicate user.json field");
    }
    reader.expect('}'); reader.end();
    if (xuid.substr(0, 2) == "0x" || xuid.substr(0, 2) == "0X") xuid.erase(0, 2);
    if (xuid.empty() || xuid.size() > 16) throw std::invalid_argument("Invalid profile XUID length");
    const auto parsed = std::from_chars(xuid.data(), xuid.data()+xuid.size(), result.xuid, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != xuid.data()+xuid.size()) throw std::invalid_argument("Invalid profile XUID");
    Validate(result);
    return result;
}
}
LocalProfile ParseProfile(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first != std::string_view::npos && text[first] == '{') return ParseJson(text);
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
    output << "{\n  \"name\": " << std::quoted(profile.name) << ",\n  \"xuid\": \""
           << std::hex << std::setfill('0') << std::setw(16) << profile.xuid << "\"\n}\n";
    return output.str();
}
}
