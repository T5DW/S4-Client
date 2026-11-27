#include "../common.hpp"
#pragma once
#include "component_interface.hpp"
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>

namespace component_loader {
using registration_functor = std::function<std::unique_ptr<generic_component>()>;
inline std::function<bool()> finished_auth = [] { return true; };
void register_component(registration_functor functor, component_type type);
bool activate(bool server = false);
bool post_start();
bool post_load();
void post_unpack();
void pre_destroy();
void find_signatures(memory::signature_store& batch);
bool is_post_load_phase_complete();
bool is_post_unpack_phase_complete();
[[noreturn]] void trigger_premature_shutdown();
class premature_shutdown_trigger final : public std::exception {
public:
    const char* what() const noexcept override { return "Premature shutdown requested"; }
};
template <typename T> class installer final {
    static_assert(std::is_base_of_v<generic_component, T>, "invalid component base");
public:
    installer() {
        register_component([] { return std::make_unique<T>(); }, T::type);
    }
};
}
#define S4_COMPONENT_JOIN_(a, b) a##b
#define S4_COMPONENT_JOIN(a, b) S4_COMPONENT_JOIN_(a, b)
#define REGISTER_COMPONENT(name) namespace { \
    component_loader::installer<name> S4_COMPONENT_JOIN(s4_component_, __COUNTER__); \
}
