#pragma once

#include <mycad/application/CommandBus.hpp>
#include <mycad/infrastructure/OpenGLRenderAdapter.hpp>

#include <QMainWindow>
#include <QString>
#include <memory>

/// @file MainWindow — top-level application window.

namespace mycad::ui {

class ViewportWidget;

/// @brief Main application window.  Holds the 3-D viewport as its central widget.
///
/// Lifecycle:
///   1. Construct at composition root, injecting CommandBus and OpenGLRenderAdapter.
///   2. Call show(); initializeGL fires, emits glReady, onGlReady sends the demo box.
///   3. File → Save As persists the event store to a .mycad file (Sprint 0.5).
///   4. File → Open replays events from a .mycad file (Sprint 0.5).
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

    /// @brief Resets the scene to an empty state.
    void onFileNew();

    /// @brief Saves the current scene to a .mycad file chosen by the user.
    void onFileSaveAs();

    /// @brief Opens a .mycad file and replays its events to rebuild the scene.
    void onFileOpen();

private:
    /// @brief Creates the menu bar with File menu entries.
    void setupMenus();

    std::shared_ptr<mycad::application::CommandBus> bus_;
    std::shared_ptr<mycad::infrastructure::OpenGLRenderAdapter> renderer_;
    ViewportWidget* viewport_{nullptr};
    bool demoSent_{false};

    /// @brief Absolute path of the currently open .mycad file, or empty if unsaved.
    QString currentFilePath_;
};

}  // namespace mycad::ui
