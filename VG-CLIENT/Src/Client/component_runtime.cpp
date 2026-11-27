#include "logger.hpp"
#include "../common.hpp"
#include "client_runtime.hpp"
#include "auth.hpp"
#include "Lua/lua_hook.hpp"
#include "Lua/runtime.hpp"
#include "postload.hpp"
#include "console.hpp"
#include "patches.hpp"
#include "../Loader/component_loader.hpp"

namespace {
class uploaded_component final : public component_interface {
public:
    void post_start() override {
        lua::hook::set("Engine.IsLocalPlayAllowed", true);
        lua::hook::set("Engine.IsPremiumPlayer", true);
        lua::hook::set("Fences.OfflineDataFetched", true);
        lua::hook::try_install();
        lua::runtime::try_install();
        AutoAuthLoad();
    }
    void post_unpack() override {

        InitHooks();
        PatchSystemlinkLobby();
        patches();
        lua::runtime::try_install();
    }
    void pre_destroy() override {
        lua::runtime::shutdown();
        lua::hook::shutdown();
        StopAuthHook();
        StopAuthImageHook();
    }
};
}
REGISTER_COMPONENT(uploaded_component)

extern "C" int S4ClientOpenConsole(void) {
    return console(nullptr) == 0;
}
extern "C" void S4ClientCloseConsole(void) {
    StopComponentConsole();
}
extern "C" S4_CLIENT_API int S4ClientStartComponents() {
    try {
        if (!component_loader::activate(false) || !component_loader::post_start()) return 0;
         
         
        static std::atomic<bool> auth_requested{false};
        if (!auth_requested.exchange(true)) AuthCommand();
        return 1;
    }
    catch (const std::exception& e) { S4Log("Component startup failed: %s\n", e.what()); return 0; }
}
extern "C" S4_CLIENT_API void S4ClientStopComponents() {
    S4ClientCloseConsole();
    component_loader::pre_destroy();
}
