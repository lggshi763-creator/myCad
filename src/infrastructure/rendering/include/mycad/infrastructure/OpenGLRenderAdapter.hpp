#pragma once

#include <mycad/domain/BRepHandle.hpp>
#include <mycad/domain/IRenderPort.hpp>
#include <mycad/domain/TriangleMesh.hpp>

#include <memory>

/// @file OpenGLRenderAdapter — OpenGL 4.5 implementation of IRenderPort (ADR-0005).

namespace mycad::infrastructure {

/// @brief OpenGL 4.5 rendering adapter that implements the domain IRenderPort.
///
/// All Qt and OpenGL headers are confined to OpenGLRenderAdapter.cpp (PIMPL).
/// Consumers only see the domain-layer types.
///
/// Lifecycle:
///   1. Construct the adapter.
///   2. Call initialize() from within an active OpenGL context (e.g. QOpenGLWidget::initializeGL).
///   3. Use uploadMesh() / removeMesh() to manage GPU-resident geometry.
///   4. Call render() once per frame from the active context.
///
/// @see IRenderPort for the full port contract.
class OpenGLRenderAdapter final : public domain::IRenderPort {
public:
    OpenGLRenderAdapter();
    ~OpenGLRenderAdapter() override;

    OpenGLRenderAdapter(const OpenGLRenderAdapter&) = delete;
    OpenGLRenderAdapter& operator=(const OpenGLRenderAdapter&) = delete;

    /// @brief Initialises OpenGL function pointers and creates GPU resources.
    ///
    /// Must be called exactly once from within an active OpenGL 4.5 context.
    /// Calling any other method before initialize() results in undefined behaviour.
    void initialize();

    // --- IRenderPort overrides ---

    /// @brief Uploads (or replaces) the triangle mesh for a geometry handle.
    /// @throws std::runtime_error if initialize() has not been called.
    void uploadMesh(domain::BRepHandle handle, const domain::TriangleMesh& mesh) override;

    /// @brief Removes the VAO/VBO resources for handle.  No-op if not present.
    /// @noexcept-ok
    void removeMesh(domain::BRepHandle handle) noexcept override;

    /// @brief Removes all uploaded mesh GPU resources.  No-op if not initialized.
    ///
    /// Called before replaying a new scene (File → New / Open).
    /// @noexcept-ok
    void clearAll() noexcept;

    /// @brief Releases all GPU resources (VAOs, VBOs, shader program).
    ///
    /// Must be called from within an active OpenGL context **before** the context
    /// is destroyed — typically from a slot connected to
    /// QOpenGLContext::aboutToBeDestroyed with Qt::DirectConnection.
    /// Idempotent: safe to call more than once.
    /// @noexcept-ok
    void cleanup() noexcept;

    /// @brief Sets the view matrix used for the next render() call.
    ///
    /// @param mat4  Column-major 4×4 float array.  Caller retains ownership.
    /// @noexcept-ok
    void setViewMatrix(const float* mat4) noexcept override;

    /// @brief Sets the projection matrix used for the next render() call.
    ///
    /// @param mat4  Column-major 4×4 float array.  Caller retains ownership.
    /// @noexcept-ok
    void setProjectionMatrix(const float* mat4) noexcept override;

    /// @brief Clears the framebuffer and draws all uploaded meshes.
    ///
    /// Must be called from within an active OpenGL context.
    void render() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace mycad::infrastructure
