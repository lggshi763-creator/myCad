#pragma once

#include <mycad/domain/Point3D.hpp>

#include <string>

/// @file Immutable axis-aligned bounding box (ADR-0013).

namespace mycad::domain {

/// @brief Immutable axis-aligned bounding box (mm).
/// @thread-safe
struct BoundingBox {
    Point3D min;
    Point3D max;

    constexpr BoundingBox() noexcept = default;
    constexpr BoundingBox(Point3D min_, Point3D max_) noexcept : min{min_}, max{max_} {}

    /// @brief Returns true if p lies inside or on the boundary.
    /// @noexcept-ok
    [[nodiscard]] constexpr bool contains(Point3D p) const noexcept {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y && p.z >= min.z &&
               p.z <= max.z;
    }

    /// @brief Returns true if this box and other overlap or touch.
    /// @noexcept-ok
    [[nodiscard]] constexpr bool intersects(const BoundingBox& other) const noexcept {
        return min.x <= other.max.x && max.x >= other.min.x && min.y <= other.max.y &&
               max.y >= other.min.y && min.z <= other.max.z && max.z >= other.min.z;
    }

    /// @brief Returns a new box expanded by margin on all sides.
    /// @param margin  Expansion amount (mm). May be negative to shrink.
    /// @noexcept-ok
    [[nodiscard]] constexpr BoundingBox expanded(double margin) const noexcept {
        return BoundingBox{Point3D{min.x - margin, min.y - margin, min.z - margin},
                           Point3D{max.x + margin, max.y + margin, max.z + margin}};
    }

    /// @brief Compares min and max within kDefaultEpsilon.
    /// @noexcept-ok
    [[nodiscard]] friend constexpr bool operator==(const BoundingBox& a,
                                                   const BoundingBox& b) noexcept {
        return a.min == b.min && a.max == b.max;
    }
};

/// @brief Returns "BoundingBox{min=..., max=...}" for debugging.
/// @throws std::bad_alloc
[[nodiscard]] std::string to_string(const BoundingBox& bb);

}  // namespace mycad::domain
