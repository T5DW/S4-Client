#pragma once
#include "fov.hpp"
#include <array>
#include <mutex>
#include <optional>
#include <stdexcept>

namespace fov::detail {
// Coalesce repeated console changes; retain a request while its dvar is not ready.
class Requests {
    std::mutex mutex_;
    std::array<std::optional<float>, 2> pending_{};
public:
    void submit(Parameter parameter, float value) {
        if (!valid(parameter, value)) throw std::invalid_argument("FOV value outside command range");
        const std::lock_guard<std::mutex> lock(mutex_);
        pending_[parameter == Parameter::View ? 0 : 1] = value;
    }
    template<class Setter> void process(Setter&& set) {
        const std::lock_guard<std::mutex> lock(mutex_);
        for (std::size_t i = 0; i < pending_.size(); ++i)
            if (pending_[i] && set(i == 0 ? Parameter::View : Parameter::Scale, *pending_[i]))
                pending_[i].reset();
    }
    void clear() {
        const std::lock_guard<std::mutex> lock(mutex_);
        for (auto& value : pending_) value.reset();
    }
};
}
