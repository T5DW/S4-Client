#pragma once
#include <cmath>
namespace fov {
enum class Parameter { View, Scale };
inline bool valid(Parameter parameter, float value) {
    return std::isfinite(value) && (parameter == Parameter::View
        ? value >= 1.0f && value <= 110.0f
        : value >= 0.2f && value <= 2.0f);
}
void request(Parameter parameter, float value);
void apply_pending();
void clear_pending();
}
