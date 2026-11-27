#pragma once
#include <atomic>
namespace lua::autoload {
// Failed passes also finish: a user script must never trigger a per-frame retry.
class Gate {
    std::atomic<bool> pending_{true};
public:
    bool pending() const { return pending_.load(std::memory_order_acquire); }
    void queue() { pending_.store(true, std::memory_order_release); }
    bool begin(bool initializing) {
        return !initializing && pending_.exchange(false, std::memory_order_acq_rel);
    }
};
}
