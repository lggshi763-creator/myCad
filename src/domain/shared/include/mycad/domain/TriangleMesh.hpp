#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

/// @file Indexed triangle mesh produced by BRep tessellation.

namespace mycad::domain {

/// @brief Indexed triangle mesh produced by IGeometryConstructionPort::tessellate().
///
/// Vertices are stored as interleaved XYZ floats (float precision is sufficient
/// for GPU rendering; domain code does not perform geometric calculations on
/// this mesh). Indices form triples referencing the vertex array.
///
/// @si-units{millimeter}
struct TriangleMesh {
    /// Interleaved x, y, z floats.  Length == 3 * vertexCount().
    std::vector<float> vertices;

    /// Triangle index triples into the vertex array.  Length == 3 * triangleCount().
    std::vector<uint32_t> indices;

    /// @brief Returns true when the mesh contains no geometry.
    /// @noexcept-ok
    [[nodiscard]] bool empty() const noexcept {
        return vertices.empty();
    }

    /// @brief Number of distinct vertices.
    /// @noexcept-ok
    [[nodiscard]] std::size_t vertexCount() const noexcept {
        return vertices.size() / 3;
    }

    /// @brief Number of triangles.
    /// @noexcept-ok
    [[nodiscard]] std::size_t triangleCount() const noexcept {
        return indices.size() / 3;
    }
};

}  // namespace mycad::domain
