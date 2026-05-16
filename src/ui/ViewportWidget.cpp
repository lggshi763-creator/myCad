#include <mycad/ui/ViewportWidget.hpp>

#include <QMouseEvent>
#include <QWheelEvent>

namespace mycad::ui {

ViewportWidget::ViewportWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setMouseTracking(false);
    // Request an OpenGL 4.5 core-profile context.  The default surface format
    // is set globally in main() before QApplication is constructed.
}

ViewportWidget::~ViewportWidget() = default;

void ViewportWidget::setRenderAdapter(
    std::shared_ptr<mycad::infrastructure::OpenGLRenderAdapter> adapter) {
    adapter_ = std::move(adapter);
}

// ---------------------------------------------------------------------------
// QOpenGLWidget overrides
// ---------------------------------------------------------------------------

void ViewportWidget::initializeGL() {
    if (adapter_) {
        adapter_->initialize();
    }
    emit glReady();
}

void ViewportWidget::resizeGL(int w, int h) {
    aspect_ = (h > 0) ? static_cast<float>(w) / static_cast<float>(h) : 1.f;
}

void ViewportWidget::paintGL() {
    if (!adapter_) {
        return;
    }

    const Eigen::Matrix4f view = camera_.viewMatrix();
    const Eigen::Matrix4f proj = camera_.projMatrix(aspect_);

    // Eigen stores matrices column-major; pass with GL_FALSE (no transpose).
    adapter_->setViewMatrix(view.data());
    adapter_->setProjectionMatrix(proj.data());
    adapter_->render();
}

// ---------------------------------------------------------------------------
// Input events
// ---------------------------------------------------------------------------

void ViewportWidget::mousePressEvent(QMouseEvent* event) {
    lastMousePos_ = event->pos();
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    const QPoint delta = event->pos() - lastMousePos_;
    lastMousePos_ = event->pos();

    // Scale pixels → radians (0.005 rad/px is a comfortable sensitivity).
    constexpr float kSensitivity = 0.005f;
    camera_.orbit(static_cast<float>(delta.x()) * kSensitivity,
                  static_cast<float>(-delta.y()) * kSensitivity);
    update();
}

void ViewportWidget::wheelEvent(QWheelEvent* event) {
    const float steps = static_cast<float>(event->angleDelta().y()) / 120.f;
    camera_.zoom(steps);
    update();
}

}  // namespace mycad::ui
