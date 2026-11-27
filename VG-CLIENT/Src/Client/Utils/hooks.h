#include "../../common.hpp"
#pragma once
#include "hook.hpp"
#include "../platform_detour.hpp"

namespace utils::hook {
class detour final : public s4::hooks::Detour {
public:
    template <typename Target, typename Callback>
    void create(Target target, Callback callback) {
        s4::hooks::Detour::create(reinterpret_cast<void*>(target),
                                reinterpret_cast<void*>(callback));
    }
    template <typename Result = void, typename... Args>
    Result invoke(Args... args) { return stub<Result>(args...); }
    template <typename Result = void, typename... Args>
    Result stub(Args... args) {
        using Function = Result (WINAPI*)(Args...);
        return reinterpret_cast<Function>(original())(args...);
    }
};
}
