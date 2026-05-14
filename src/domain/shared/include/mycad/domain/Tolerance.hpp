#pragma once

#include <cstring>

/// @file Floating-point tolerance constants and comparison helpers (ADR-0013 §4).

namespace mycad::domain {

/// @brief Default CAD tolerance: 1e-9 mm (1 femtometre).
/// @si-units{millimeter}
inline constexpr double kDefaultEpsilon = 1e-9;

/// @brief Returns true if |a - b| <= kDefaultEpsilon.
/// @noexcept-ok
[[nodiscard]] constexpr bool nearEqual(double a, double b) noexcept {
    const double diff = a - b;
    return (diff >= -kDefaultEpsilon) && (diff <= kDefaultEpsilon);
}

/// @brief Returns true if |a - b| <= tolerance.
/// @param tolerance  Must be >= 0.
/// @noexcept-ok
[[nodiscard]] constexpr bool nearEqual(double a, double b, double tolerance) noexcept {
    const double diff = a - b;
    return (diff >= -tolerance) && (diff <= tolerance);
}

/// @brief Returns true if a and b share identical bit patterns (+0.0 != -0.0).
/// @noexcept-ok
[[nodiscard]] inline bool bitwiseEqual(double a, double b) noexcept {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
}

}  // namespace mycad::domain
