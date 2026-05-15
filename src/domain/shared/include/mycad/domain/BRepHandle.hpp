#pragma once

#include <compare>
#include <cstdint>

/// @file Opaque handle types for boundary-representation geometry (ADR-0004).
///
/// Handles are plain integer tokens minted and owned by the geometry adapter
/// (infrastructure). Domain and application code passes them around without
/// ever touching OCCT types directly.

namespace mycad::domain {

/// @brief Opaque handle to a BRep solid owned by the geometry adapter.
///
/// A default-constructed (id == 0) handle is invalid. Handles are only valid
/// until release(BRepHandle) is called on the adapter that created them.
struct BRepHandle {
    uint64_t id{0};

    /// @brief Returns true when the handle refers to an adapter-owned shape.
    /// @noexcept-ok
    [[nodiscard]] constexpr bool valid() const noexcept {
        return id != 0;
    }

    auto operator<=>(const BRepHandle&) const noexcept = default;
};

/// @brief Opaque handle to a wire (closed or open edge loop) owned by the adapter.
struct WireHandle {
    uint64_t id{0};

    /// @brief Returns true when the handle refers to an adapter-owned wire.
    /// @noexcept-ok
    [[nodiscard]] constexpr bool valid() const noexcept {
        return id != 0;
    }

    auto operator<=>(const WireHandle&) const noexcept = default;
};

/// @brief Reference to a specific edge within a BRep solid.
///
/// @param owner  The solid that owns this edge.
/// @param index  Adapter-defined edge index within the solid.
struct EdgeRef {
    BRepHandle owner;
    uint32_t index{0};

    auto operator<=>(const EdgeRef&) const noexcept = default;
};

/// @brief Reference to a specific face within a BRep solid.
///
/// @param owner  The solid that owns this face.
/// @param index  Adapter-defined face index within the solid.
struct FaceRef {
    BRepHandle owner;
    uint32_t index{0};

    auto operator<=>(const FaceRef&) const noexcept = default;
};

}  // namespace mycad::domain
