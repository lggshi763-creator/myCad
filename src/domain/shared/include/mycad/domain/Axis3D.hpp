#pragma once

#include <mycad/domain/Point3D.hpp>
#include <mycad/domain/Vector3D.hpp>

#include <string>

/// @file Immutable 3D axis (ray) value object (ADR-0013).

namespace mycad::domain {

/// @brief Immutable ray defined by an origin and a direction (mm).
///
/// Caller is responsible for passing a normalised direction vector.
/// @thread-safe
struct Axis3D {
    Point3D origin;
    Vector3D direction;

    constexpr Axis3D() noexcept = default;
    constexpr Axis3D(Point3D origin, Vector3D direction) noexcept
        : origin{origin}, direction{direction} {}

    /// @brief Compares origin and direction within kDefaultEpsilon.
    /// @noexcept-ok
    [[nodiscard]] friend constexpr bool operator==(const Axis3D& a, const Axis3D& b) noexcept {
        return a.origin == b.origin && a.direction == b.direction;
    }
};

/// @brief Returns "Axis3D{origin=..., dir=...}" for debugging.
/// @throws std::bad_alloc
[[nodiscard]] std::string to_string(const Axis3D& a);

}  // namespace mycad::domain
