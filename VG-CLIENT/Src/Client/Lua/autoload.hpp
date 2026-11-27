#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace lua::autoload {
struct Script { std::filesystem::path path; std::string name, code; };
using Report = std::function<void(const std::string&)>;
void ensure_directory(const std::filesystem::path& directory, std::string_view default_script,
                      std::string_view legacy_script = {}, const Report& report = {});
std::vector<Script> discover(const std::filesystem::path& directory, const Report& report);
}
