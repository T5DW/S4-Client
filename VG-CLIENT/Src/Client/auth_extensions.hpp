#include "../common.hpp"
#pragma once

struct XenonUserData;
namespace auth {
struct LocalProfile {
    std::string name;
    std::uint64_t xuid;
};
LocalProfile ParseProfile(std::string_view text);
std::string SerializeProfile(const LocalProfile& profile);
LocalProfile local_profile();
void set_player_name(std::string name);
XenonUserData* get_local_user_data();
void setup_local_user_data();
void on_stats_init(std::function<void()> callback);
void notify_stats_initialized();
}
