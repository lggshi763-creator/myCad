#pragma once

#include <mycad/domain/Tolerance.hpp>

#include <string>

/// @file Immutable 3D vector value object (ADR-0013).

namespace mycad::domain {

/// @brief Immutable 3D direction/displacement vector (mm).
/// @thread-safe
struct Vector3D {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    constexpr Vector3D() noexcept = default;
    constexpr Vector3D(double x_, double y_, double z_) noexcept : x{x_}, y{y_}, z{z_} {}

    /// @brief Returns the dot product with v.
    /// @noexcept-ok
    [[nodiscard]] constexpr double dot(Vector3D v) const noexcept {
        return x * v.x + y * v.y + z * v.z;
    }

    /// @brief Returns the cross product with v (right-hand rule).
    /// @noexcept-ok
    [[nodiscard]] constexpr Vector3D cross(Vector3D v) const noexcept {
        return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
    }

    /// @brief Returns the Euclidean length (mm).
    /// @noexcept-ok
    [[nodiscard]] double length() const noexcept;

    /// @brief Returns a unit vector in the same direction, or {0,0,0} if length < kDefaultEpsilon.
    /// @noexcept-ok
    [[nodiscard]] Vector3D normalized() const noexcept;

    /// @brief Compares within kDefaultEpsilon.
    /// @noexcept-ok
    [[nodiscard]] friend constexpr bool operator==(Vector3D a, Vector3D b) noexcept {
        return nearEqual(a.x, b.x) && nearEqual(a.y, b.y) && nearEqual(a.z, b.z);
    }
};

/// @brief Returns "Vector3D{x=..., y=..., z=...}" for debugging (not serialization).
/// @throws std::bad_alloc
[[nodiscard]] std::string to_string(Vector3D v);

}  // namespace mycad::domain
