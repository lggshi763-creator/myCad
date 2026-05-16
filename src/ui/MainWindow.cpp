#include <mycad/application/commands/CreateBoxCommand.hpp>
#include <mycad/domain/AggregateId.hpp>
#include <mycad/domain/TessellationParams.hpp>
#include <mycad/domain/events/BoxCreatedEvent.hpp>
#include <mycad/infrastructure/MycadFileStore.hpp>
#include <mycad/ui/MainWindow.hpp>
#include <mycad/ui/ViewportWidget.hpp>

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <string>
#include <string_view>

namespace mycad::ui {

// ---------------------------------------------------------------------------
// File-local helper — extracts a double from our flat BoxCreatedEvent JSON.
// Expected format: {"dx":2.0,"dy":3.0,"dz":4.0,"handle_id":1}
// ---------------------------------------------------------------------------

namespace {

/// @brief Returns the double value for @p key from a flat JSON object string.
///
/// Designed for the specific format produced by the BoxCreatedEvent serializer.
/// Returns 0.0 on any parse failure.
double extractJsonDouble(const std::string& json, std::string_view key) {
    const std::string token = "\"" + std::string(key) + "\":";
    const auto pos = json.find(token);
    if (pos == std::string::npos) {
        return 0.0;
    }
    try {
        return std::stod(json.substr(pos + token.size()));
    } catch (...) {
        return 0.0;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

MainWindow::MainWindow(std::shared_ptr<mycad::application::CommandBus> bus,
                       std::shared_ptr<mycad::infrastructure::OpenGLRenderAdapter> renderer,
                       std::shared_ptr<mycad::infrastructure::SqliteEventStore> store,
                       std::shared_ptr<mycad::infrastructure::EnttRegistry> ecs,
                       std::shared_ptr<mycad::infrastructure::OcctGeometryAdapter> geom,
                       QWidget* parent)
    : QMainWindow(parent), bus_{std::move(bus)}, renderer_{std::move(renderer)},
      store_{std::move(store)}, ecs_{std::move(ecs)}, geom_{std::move(geom)} {
    viewport_ = new ViewportWidget(this);
    viewport_->setRenderAdapter(renderer_);
    setCentralWidget(viewport_);

    connect(viewport_, &ViewportWidget::glReady, this, &MainWindow::onGlReady);

    setupMenus();
    updateTitle();
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void MainWindow::setupMenus() {
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::onFileNew);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Save As..."), QKeySequence::SaveAs, this, &MainWindow::onFileSaveAs);
    fileMenu->addAction(tr("&Open..."), QKeySequence::Open, this, &MainWindow::onFileOpen);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), QKeySequence::Quit, qApp, &QApplication::quit);
}

void MainWindow::updateTitle() {
    if (currentFilePath_.isEmpty()) {
        setWindowTitle(tr("myCad — Untitled"));
    } else {
        setWindowTitle(tr("myCad — %1").arg(currentFilePath_));
    }
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void MainWindow::onGlReady() {
    if (demoSent_) {
        return;
    }
    demoSent_ = true;

    // Send a demo 2 × 3 × 4 mm box so the viewport shows real geometry
    // immediately after the GL context is initialised.
    mycad::domain::AggregateId id{};
    id.bytes[0] = 0x01;

    (void)bus_->send(mycad::application::commands::CreateBoxCommand{2.0, 3.0, 4.0, id});
    viewport_->update();
}

void MainWindow::onFileNew() {
    ecs_->clear();
    renderer_->clearAll();
    currentFilePath_.clear();
    updateTitle();
    viewport_->update();
}

void MainWindow::onFileSaveAs() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save As"), currentFilePath_, tr("myCad Files (*.mycad)"));
    if (path.isEmpty()) {
        return;
    }

    try {
        mycad::infrastructure::MycadFileStore::save(store_->dbPath(), path.toStdString());
        currentFilePath_ = path;
        updateTitle();
    } catch (const std::exception& ex) {
        QMessageBox::critical(
            this,
            tr("Save Failed"),
            tr("Could not save \"%1\":\n%2").arg(path, QString::fromStdString(ex.what())));
    }
}

void MainWindow::onFileOpen() {
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Open"), QString(), tr("myCad Files (*.mycad)"));
    if (path.isEmpty()) {
        return;
    }

    try {
        // Extract events.db from the .mycad ZIP into the system temp directory.
        const std::string extractDir = QDir::tempPath().toStdString();
        const std::string dbPath =
            mycad::infrastructure::MycadFileStore::load(path.toStdString(), extractDir);

        // Open the extracted event store.
        auto loadedStore = std::make_shared<mycad::infrastructure::SqliteEventStore>(dbPath);

        // Re-register the BoxCreatedEvent serializer so Save As works from the
        // opened file.
        loadedStore->registerSerializer(
            "mycad.box.BoxCreated", [](const mycad::domain::DomainEvent& base) -> std::string {
                const auto& ev = static_cast<const mycad::domain::BoxCreatedEvent&>(base);
                return "{\"dx\":" + std::to_string(ev.dx) + ",\"dy\":" + std::to_string(ev.dy) +
                       ",\"dz\":" + std::to_string(ev.dz) +
                       ",\"handle_id\":" + std::to_string(ev.handle.id) + "}";
            });

        // Clear the current scene before replay.
        ecs_->clear();
        renderer_->clearAll();

        // Replay every BoxCreatedEvent by re-building geometry from the payload.
        const mycad::domain::TessellationParams tessParams{};
        for (const auto& aid : loadedStore->allAggregateIds()) {
            for (const auto& row : loadedStore->loadRows(aid)) {
                if (row.typeName != "mycad.box.BoxCreated") {
                    continue;
                }

                const double dx = extractJsonDouble(row.payload, "dx");
                const double dy = extractJsonDouble(row.payload, "dy");
                const double dz = extractJsonDouble(row.payload, "dz");
                if (dx <= 0.0 || dy <= 0.0 || dz <= 0.0) {
                    continue;  // Skip malformed or placeholder rows.
                }

                auto boxResult = geom_->makeBox(dx, dy, dz);
                if (!boxResult) {
                    continue;
                }
                ecs_->create();

                auto meshResult = geom_->tessellate(*boxResult, tessParams);
                if (meshResult) {
                    renderer_->uploadMesh(*boxResult, *meshResult);
                }
            }
        }

        // Swap the active store and update UI state.
        store_ = std::move(loadedStore);
        demoSent_ = true;  // Prevent the startup demo box from overwriting the scene.
        currentFilePath_ = path;
        updateTitle();
        viewport_->update();

    } catch (const std::exception& ex) {
        QMessageBox::critical(
            this,
            tr("Open Failed"),
            tr("Could not open \"%1\":\n%2").arg(path, QString::fromStdString(ex.what())));
    }
}

}  // namespace mycad::ui
