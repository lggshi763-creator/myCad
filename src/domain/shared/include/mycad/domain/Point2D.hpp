#pragma once

#include <mycad/domain/Tolerance.hpp>

#include <string>

/// @file Immutable 2D point value object (ADR-0013).

namespace mycad::domain {

/// @brief Immutable 2D point in the sketch plane (mm).
/// @thread-safe
struct Point2D {
    double x{0.0};
    double y{0.0};

    constexpr Point2D() noexcept = default;
    constexpr Point2D(double x, double y) noexcept : x{x}, y{y} {}

    /// @brief Returns a new point offset by delta.
    /// @noexcept-ok
    [[nodiscard]] constexpr Point2D translated(Point2D delta) const noexcept {
        return Point2D{x + delta.x, y + delta.y};
    }

    /// @brief Compares within kDefaultEpsilon.
    /// @noexcept-ok
    [[nodiscard]] friend constexpr bool operator==(Point2D a, Point2D b) noexcept {
        return nearEqual(a.x, b.x) && nearEqual(a.y, b.y);
    }
};

/// @brief Returns "Point2D{x=..., y=...}" for debugging (not serialization).
/// @throws std::bad_alloc
[[nodiscard]] std::string to_string(Point2D p);

}  // namespace mycad::domain
