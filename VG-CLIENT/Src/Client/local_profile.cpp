#include "../common.hpp"
#include "logger.hpp"
#include "auth_extensions.hpp"
#include "stats.hpp"
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace auth {
namespace {
std::mutex callbacks_mutex;
std::vector<std::function<void()>> stats_callbacks;
LocalProfile ReadProfile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot read vg-assets/user/user.json");
    std::ostringstream bytes; bytes << input.rdbuf();
    if (input.bad()) throw std::runtime_error("Cannot read local profile bytes");
    auto profile = ParseProfile(bytes.str());
    S4Log("Auth profile: loaded saved local profile from vg-assets/user/user.json");
    return profile;
}
std::filesystem::path ProfilePath() {
    const auto directory = std::filesystem::current_path()/"vg-assets"/"user";
    std::filesystem::create_directories(directory);
    return directory/"user.json";
}
void SaveProfile(const std::filesystem::path& path, const LocalProfile& profile, bool replace) {
    auto temporary = path;
    temporary += std::wstring(L".tmp.") + std::to_wstring(GetCurrentProcessId());
    {
        std::ofstream output(temporary, std::ios::binary|std::ios::trunc);
        const auto text = SerializeProfile(profile);
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        output.close();
        if (!output) {
            std::error_code ignored; std::filesystem::remove(temporary, ignored);
            throw std::runtime_error("Cannot save local profile");
        }
    }
    if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH | (replace ? MOVEFILE_REPLACE_EXISTING : 0))) {
        const auto error = GetLastError();
        std::error_code ignored; std::filesystem::remove(temporary, ignored);
        if (!replace && (error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS)) return;
        throw std::system_error(static_cast<int>(error), std::system_category(), "Cannot publish local profile");
    }
}
LocalProfile LoadOrCreate() {
    const auto path = ProfilePath();
    if (std::filesystem::exists(path)) return ReadProfile(path);
    // Preserve the identity of existing installations when moving to JSON.
    const auto legacy = path.parent_path().parent_path()/"profile.cfg";
    if (std::filesystem::exists(legacy)) {
        SaveProfile(path, ReadProfile(legacy), false);
        return ReadProfile(path);
    }
    const char* username = "MatchedPlayer";
    LocalProfile created{username, 0};
    do {
        const auto status = BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&created.xuid),
                                            sizeof(created.xuid), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        if (status < 0) throw std::runtime_error("Cannot generate a local XUID");
    } while (!created.xuid);
    SaveProfile(path, created, false);
    return ReadProfile(path);
}
std::mutex profile_mutex;
LocalProfile& ProfileValue() {
    static LocalProfile value = LoadOrCreate();
    return value;
}
}
LocalProfile local_profile() {
    const std::lock_guard<std::mutex> lock(profile_mutex);
    return ProfileValue();
}
void set_player_name(std::string name) {
    const std::lock_guard<std::mutex> lock(profile_mutex);
    auto updated = ProfileValue();
    updated.name = std::move(name);
    // Validate and save before publishing the new in-memory snapshot.
    (void)SerializeProfile(updated);
    SaveProfile(ProfilePath(), updated, true);
    ProfileValue() = std::move(updated);
}
XenonUserData* get_local_user_data() { return unkw_Get_UserData(0); }
void setup_local_user_data() {
    auto* user = get_local_user_data();
    if (!user) throw std::runtime_error("Local user data is not ready");
    const auto& profile = local_profile();
    Assign_UsernameAndXUID(profile.name.c_str(), profile.xuid);
}
void on_stats_init(std::function<void()> callback) {
    if (!callback) throw std::invalid_argument("Stats callback is empty");
    const std::lock_guard<std::mutex> lock(callbacks_mutex);
    stats_callbacks.push_back(std::move(callback));
}
void notify_stats_initialized() {
    std::vector<std::function<void()>> callbacks;
    { const std::lock_guard<std::mutex> lock(callbacks_mutex); callbacks = stats_callbacks; }
    S4Log("Auth stats: notifying %zu registered callback(s)", callbacks.size());
    for (auto& callback : callbacks) callback();
}
}
