#pragma once

#include <mycad/infrastructure/OpenGLRenderAdapter.hpp>
#include <mycad/ui/Camera.hpp>

#include <QOpenGLWidget>
#include <QPoint>
#include <memory>

/// @file ViewportWidget — QOpenGLWidget that drives the rendering adapter.

namespace mycad::ui {

/// @brief Qt OpenGL widget that owns a camera and drives OpenGLRenderAdapter.
///
/// Lifecycle:
///   1. Construct and call setRenderAdapter() before the widget is shown.
///   2. initializeGL() calls adapter->initialize() once the context is active.
///   3. Each paintGL() uploads the current camera matrices and calls render().
///
/// Mouse controls:
///   - Left-button drag  → orbit (azimuth / elevation).
///   - Wheel             → zoom (radius).
class ViewportWidget final : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit ViewportWidget(QWidget* parent = nullptr);
    ~ViewportWidget() override;

    /// @brief Sets the rendering adapter.  Must be called before show().
    void setRenderAdapter(std::shared_ptr<mycad::infrastructure::OpenGLRenderAdapter> adapter);

    /// @brief Read-only access to the camera (e.g. for debug overlays).
    /// @noexcept-ok
    [[nodiscard]] const Camera& camera() const noexcept {
        return camera_;
    }

signals:
    /// @brief Emitted once after the OpenGL context is initialised and the
    ///        adapter has been set up.  Connect to trigger initial scene load.
    void glReady();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    std::shared_ptr<mycad::infrastructure::OpenGLRenderAdapter> adapter_;
    Camera camera_{};
    float aspect_{1.f};
    QPoint lastMousePos_{};
};

}  // namespace mycad::ui
