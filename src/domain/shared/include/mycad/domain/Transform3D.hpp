#pragma once

#include <mycad/domain/Point3D.hpp>
#include <mycad/domain/Tolerance.hpp>
#include <mycad/domain/Vector3D.hpp>

#include <array>
#include <string>

/// @file Immutable 4x4 affine transformation matrix (ADR-0013).

namespace mycad::domain {

/// @brief Immutable row-major 4x4 affine transform (mm).
///
/// Index layout: m[row*4 + col]. Translation in column 3 (m[3], m[7], m[11]).
/// @thread-safe
struct Transform3D {
    std::array<double, 16> m{};  ///< Zero-initialised by default; use identity().

    /// @brief Returns the identity transform.
    [[nodiscard]] static Transform3D identity() noexcept;

    /// @brief Applies this transform to a point (translation included).
    /// @noexcept-ok
    [[nodiscard]] Point3D apply(Point3D p) const noexcept;

    /// @brief Applies this transform to a direction vector (translation excluded).
    /// @noexcept-ok
    [[nodiscard]] Vector3D apply(Vector3D v) const noexcept;

    /// @brief Returns the composition this * other.
    /// @noexcept-ok
    [[nodiscard]] Transform3D composed(const Transform3D& other) const noexcept;

    /// @brief Compares all 16 elements within kDefaultEpsilon.
    /// @noexcept-ok
    [[nodiscard]] friend bool operator==(const Transform3D& a, const Transform3D& b) noexcept {
        for (int i = 0; i < 16; ++i) {
            if (!nearEqual(a.m[i], b.m[i]))
                return false;
        }
        return true;
    }
};

/// @brief Returns a row-per-line matrix string for debugging.
/// @throws std::bad_alloc
[[nodiscard]] std::string to_string(const Transform3D& t);

}  // namespace mycad::domain
