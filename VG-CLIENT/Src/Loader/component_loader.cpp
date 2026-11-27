#include "../Client/logger.hpp"
#include "../common.hpp"
#include "component_loader.hpp"
#include <algorithm>
#include <cstdio>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace component_loader {
namespace {
struct State {
    std::recursive_mutex mutex;
    std::vector<std::pair<registration_functor, component_type>> factories;
    std::vector<std::unique_ptr<generic_component>> components;
    bool active = false;
    bool started = false;
    bool loaded = false;
    bool unpacked = false;
    bool destroyed = false;
    bool phase_running = false;
};
State& state() { static State value; return value; }
template <typename Action> bool run_phase(bool& complete, Action action) {
    auto& s = state();
    if (complete) return true;
    if (s.destroyed || s.phase_running) return false;
    s.phase_running = true;
    try {
        for (auto& component : s.components)
            if (component->should_enable()) action(*component);
        complete = true;
    } catch (const std::exception& e) {
        S4Log("Component phase failed: %s\n", e.what());
    } catch (...) {
        S4Log("%s", "Component phase failed\n");
    }
    s.phase_running = false;
    return complete;
}
}
void register_component(registration_functor factory, component_type type) {
    auto& s = state(); const std::lock_guard<std::recursive_mutex> lock(s.mutex);
    if (s.active || s.phase_running || !factory) throw std::runtime_error("Invalid or late component registration");
    s.factories.emplace_back(std::move(factory), type);
}
bool activate(bool server) {
    auto& s = state(); const std::lock_guard<std::recursive_mutex> lock(s.mutex);
    if (s.active) return !s.destroyed;
    if (s.phase_running) return false;
    s.phase_running = true;
    try {
        std::vector<std::unique_ptr<generic_component>> candidates;
        for (auto& entry : s.factories) {
            if (entry.second != component_type::any &&
                server != (entry.second == component_type::server)) continue;
            auto component = entry.first();
            if (!component) throw std::runtime_error("Null component");
            candidates.push_back(std::move(component));
        }
        std::stable_sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            return a->priority() > b->priority();
        });
        s.components = std::move(candidates);
        s.active = true;
    } catch (const std::exception& e) {
        S4Log("Component activation failed: %s\n", e.what());
    } catch (...) { S4Log("%s", "Component activation failed\n"); }
    s.phase_running = false;
    return s.active;
}
bool post_start() {
    auto& s = state(); const std::lock_guard<std::recursive_mutex> lock(s.mutex);
    return s.active && run_phase(s.started, [](auto& c) { c.post_start(); });
}
bool post_load() {
    auto& s = state(); const std::lock_guard<std::recursive_mutex> lock(s.mutex);
    return s.started && run_phase(s.loaded, [](auto& c) { c.post_load(); });
}
void post_unpack() {
    auto& s = state(); const std::lock_guard<std::recursive_mutex> lock(s.mutex);
    if (!s.loaded || !finished_auth()) return;
    run_phase(s.unpacked, [](auto& c) { if (c.should_activate()) c.post_unpack(); });
}
void pre_destroy() {
    auto& s = state(); const std::lock_guard<std::recursive_mutex> lock(s.mutex);
    if (s.destroyed) return;
    s.destroyed = true;
    for (auto it = s.components.rbegin(); it != s.components.rend(); ++it) {
        try { if ((*it)->should_enable()) (*it)->pre_destroy(); }
        catch (...) { S4Log("%s", "Component shutdown failed\n"); }
    }
}
void find_signatures(memory::signature_store& batch) {
    auto& s = state(); const std::lock_guard<std::recursive_mutex> lock(s.mutex);
    for (auto& component : s.components)
        if (component->should_enable()) component->find_signatures(batch);
}
bool is_post_load_phase_complete() {
    auto& s = state(); const std::lock_guard<std::recursive_mutex> lock(s.mutex); return s.loaded;
}
bool is_post_unpack_phase_complete() {
    auto& s = state(); const std::lock_guard<std::recursive_mutex> lock(s.mutex); return s.unpacked;
}
[[noreturn]] void trigger_premature_shutdown() { throw premature_shutdown_trigger(); }
}
