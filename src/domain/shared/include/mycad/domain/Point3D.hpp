#pragma once

#include <mycad/domain/Tolerance.hpp>

#include <cmath>
#include <string>

/// @file Immutable 3D point value object (ADR-0013).

namespace mycad::domain {

/// @brief Immutable 3D point in model space (mm).
/// @thread-safe
struct Point3D {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    constexpr Point3D() noexcept = default;
    constexpr Point3D(double x, double y, double z) noexcept : x{x}, y{y}, z{z} {}

    /// @brief Returns a new point offset by delta.
    /// @noexcept-ok
    [[nodiscard]] constexpr Point3D translated(Point3D delta) const noexcept {
        return Point3D{x + delta.x, y + delta.y, z + delta.z};
    }

    /// @brief Returns the Euclidean distance to other (mm).
    /// @noexcept-ok
    [[nodiscard]] double distanceTo(Point3D other) const noexcept {
        const double dx = x - other.x;
        const double dy = y - other.y;
        const double dz = z - other.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /// @brief Compares within kDefaultEpsilon.
    /// @noexcept-ok
    [[nodiscard]] friend constexpr bool operator==(Point3D a, Point3D b) noexcept {
        return nearEqual(a.x, b.x) && nearEqual(a.y, b.y) && nearEqual(a.z, b.z);
    }
};

/// @brief Returns "Point3D{x=..., y=..., z=...}" for debugging (not serialization).
/// @throws std::bad_alloc
[[nodiscard]] std::string to_string(Point3D p);

}  // namespace mycad::domain
