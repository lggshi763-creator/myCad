#pragma once

#include <mycad/application/CommandBus.hpp>
#include <mycad/infrastructure/EnttRegistry.hpp>
#include <mycad/infrastructure/OcctGeometryAdapter.hpp>
#include <mycad/infrastructure/OpenGLRenderAdapter.hpp>
#include <mycad/infrastructure/SqliteEventStore.hpp>

#include <QMainWindow>
#include <QString>
#include <memory>

/// @file MainWindow — top-level application window.

namespace mycad::ui {

class ViewportWidget;

/// @brief Main application window.  Holds the 3-D viewport as its central widget.
///
/// Lifecycle:
///   1. Construct at composition root with all infrastructure dependencies injected.
///   2. Call show(); initializeGL fires, emits glReady, onGlReady sends the demo box.
///   3. File → Save As packs the live SqliteEventStore DB into a .mycad ZIP.
///   4. File → Open unpacks a .mycad ZIP, replays events, and rebuilds the scene.
class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::shared_ptr<mycad::application::CommandBus> bus,
                        std::shared_ptr<mycad::infrastructure::OpenGLRenderAdapter> renderer,
                        std::shared_ptr<mycad::infrastructure::SqliteEventStore> store,
                        std::shared_ptr<mycad::infrastructure::EnttRegistry> ecs,
                        std::shared_ptr<mycad::infrastructure::OcctGeometryAdapter> geom,
                        QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    /// @brief Sends one demo CreateBoxCommand when the GL context is first ready.
    void onGlReady();

    /// @brief Resets the scene to an empty state.
    void onFileNew();

    /// @brief Packs the live event store into a .mycad file chosen by the user.
    void onFileSaveAs();

    /// @brief Unpacks a .mycad file and replays its events to rebuild the scene.
    void onFileOpen();

private:
    /// @brief Creates the menu bar with File menu entries.
    void setupMenus();

    /// @brief Updates the window title to reflect the current file path.
    void updateTitle();

    std::shared_ptr<mycad::application::CommandBus> bus_;
    std::shared_ptr<mycad::infrastructure::OpenGLRenderAdapter> renderer_;
    std::shared_ptr<mycad::infrastructure::SqliteEventStore> store_;
    std::shared_ptr<mycad::infrastructure::EnttRegistry> ecs_;
    std::shared_ptr<mycad::infrastructure::OcctGeometryAdapter> geom_;
    ViewportWidget* viewport_{nullptr};
    bool demoSent_{false};

    /// @brief Absolute path of the currently saved .mycad file, or empty if unsaved.
    QString currentFilePath_;
};

}  // namespace mycad::ui
