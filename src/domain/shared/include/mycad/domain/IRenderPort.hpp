#pragma once

#include <mycad/domain/BRepHandle.hpp>
#include <mycad/domain/TriangleMesh.hpp>

/// @file IRenderPort — rendering port in the domain layer (ADR-0005).

namespace mycad::domain {

/// @brief Abstract port for the rendering subsystem.
///
/// Implementors (e.g. OpenGLRenderAdapter) live in infrastructure/rendering/.
/// Domain and application code depend only on this interface.
///
/// Lifecycle: uploadMesh() transfers mesh data to the GPU; render() draws all
/// uploaded meshes in one pass; removeMesh() frees GPU resources.
///
/// View and projection matrices are supplied as column-major 4×4 float arrays
/// (same convention as GLSL / OpenGL).
///
/// @see OcctGeometryAdapter for the companion geometry-construction port.
class IRenderPort {
public:
    virtual ~IRenderPort() = default;

    IRenderPort(const IRenderPort&) = delete;
    IRenderPort& operator=(const IRenderPort&) = delete;

    /// @brief Uploads (or replaces) the triangle mesh for a geometry handle.
    ///
    /// @param handle  The BRep geometry whose mesh is being registered.
    /// @param mesh    Triangle soup: interleaved XYZ vertices + index triples.
    /// @throws std::bad_alloc or implementation-specific GPU OOM exception.
    virtual void uploadMesh(BRepHandle handle, const TriangleMesh& mesh) = 0;

    /// @brief Removes the mesh registered for handle.  No-op if not present.
    /// @noexcept-ok
    virtual void removeMesh(BRepHandle handle) noexcept = 0;

    /// @brief Sets the view matrix (world → camera transform).
    ///
    /// @param mat4  Column-major 4×4 float array.  Caller retains ownership.
    /// @noexcept-ok
    virtual void setViewMatrix(const float* mat4) noexcept = 0;

    /// @brief Sets the projection matrix (camera → clip transform).
    ///
    /// @param mat4  Column-major 4×4 float array.  Caller retains ownership.
    /// @noexcept-ok
    virtual void setProjectionMatrix(const float* mat4) noexcept = 0;

    /// @brief Clears the framebuffer and draws all uploaded meshes.
    ///
    /// Must be called from within an active OpenGL context.
    virtual void render() = 0;

protected:
    IRenderPort() = default;
};

}  // namespace mycad::domain
