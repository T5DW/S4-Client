#include "autoload.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace lua::autoload {
void ensure_directory(const std::filesystem::path& directory, std::string_view default_script,
                      std::string_view legacy_script, const Report& report) {
    std::filesystem::create_directories(directory);
    const auto legacy = directory/"00_s4_main_menu.lua";
    if (!legacy_script.empty() && std::filesystem::exists(legacy)) {
        std::ifstream input(legacy, std::ios::binary);
        std::ostringstream bytes; bytes << input.rdbuf();
        if (input && bytes.str() == legacy_script) {
            input.close();
            auto backup = legacy; backup += ".disabled";
            unsigned suffix = 0;
            while (std::filesystem::exists(backup)) { backup = legacy; backup += ".disabled."+std::to_string(++suffix); }
            std::filesystem::rename(legacy, backup);
            if (report) report("Disabled the previous stock main-menu addon; backup: "+backup.filename().u8string());
        } else if (report) report("Preserved edited legacy addon 00_s4_main_menu.lua; stock migration skipped");
    }
    const auto path = directory/"00_s4_systemlink_lobby.lua";
    if (!std::filesystem::exists(path)) {
        std::ofstream output(path, std::ios::binary);
        output.write(default_script.data(), static_cast<std::streamsize>(default_script.size()));
        output.close();
        if (!output) throw std::runtime_error("Cannot create default S4-Client menu script");
        if (report) report("Created 00_s4_systemlink_lobby.lua");
    }
}
std::vector<Script> discover(const std::filesystem::path& directory, const Report& report) {
    std::vector<std::filesystem::path> paths;
    std::error_code error;
    std::filesystem::recursive_directory_iterator it(directory, error), end;
    if (error) { report("Cannot read Lua autoload directory: " + error.message()); return {}; }
    for (; it != end; it.increment(error)) {
        if (error) { report("Cannot enumerate Lua autoload files: " + error.message()); break; }
        if (!it->is_regular_file(error)) { error.clear(); continue; }
        auto extension = it->path().extension().u8string();
        for (auto& c : extension) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (extension == ".lua") paths.push_back(it->path());
    }
    if (error) report("Cannot enumerate Lua autoload files: " + error.message());
    std::sort(paths.begin(), paths.end(), [&](const auto& a, const auto& b) {
        return a.lexically_relative(directory).generic_u8string() < b.lexically_relative(directory).generic_u8string();
    });
    std::vector<Script> scripts;
    for (const auto& path : paths) {
        try {
            std::ifstream input(path, std::ios::binary);
            if (!input) throw std::runtime_error("Cannot open script");
            std::ostringstream bytes; bytes << input.rdbuf();
            if (input.bad()) throw std::runtime_error("Cannot read script");
            auto code = bytes.str();
            if (code.compare(0, 3, "\xEF\xBB\xBF") == 0) code.erase(0, 3);
            scripts.push_back({path, "@vg-assets/lua/autoload/"+path.lexically_relative(directory).generic_u8string(), std::move(code)});
        } catch (const std::exception& e) { report(path.filename().u8string()+": "+e.what()); }
    }
    return scripts;
}
}
