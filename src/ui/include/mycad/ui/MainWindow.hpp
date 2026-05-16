#pragma once

#include <mycad/application/CommandBus.hpp>
#include <mycad/infrastructure/OpenGLRenderAdapter.hpp>

#include <QMainWindow>
#include <memory>

/// @file MainWindow — top-level application window (Sprint 0.4 minimal shell).

namespace mycad::ui {

class ViewportWidget;

/// @brief Main application window.  Holds the 3-D viewport as its central widget.
///
/// Sprint 0.4: minimal shell (no menu bar / toolbar — added in Sprint 0.5).
/// The CommandBus and OpenGLRenderAdapter are injected at the composition root.
class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::shared_ptr<mycad::application::CommandBus> bus,
                        std::shared_ptr<mycad::infrastructure::OpenGLRenderAdapter> renderer,
                        QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    /// @brief Sends one demo CreateBoxCommand when the GL context is first ready.
    void onGlReady();

private:
    std::shared_ptr<mycad::application::CommandBus> bus_;
    std::shared_ptr<mycad::infrastructure::OpenGLRenderAdapter> renderer_;
    ViewportWidget* viewport_{nullptr};
    bool demoSent_{false};
};

}  // namespace mycad::ui
