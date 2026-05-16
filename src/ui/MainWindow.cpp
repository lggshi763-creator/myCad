#include <mycad/application/commands/CreateBoxCommand.hpp>
#include <mycad/domain/AggregateId.hpp>
#include <mycad/ui/MainWindow.hpp>
#include <mycad/ui/ViewportWidget.hpp>

#include <QApplication>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>

namespace mycad::ui {

MainWindow::MainWindow(std::shared_ptr<mycad::application::CommandBus> bus,
                       std::shared_ptr<mycad::infrastructure::OpenGLRenderAdapter> renderer,
                       QWidget* parent)
    : QMainWindow(parent), bus_{std::move(bus)}, renderer_{std::move(renderer)} {
    setWindowTitle(QStringLiteral("myCad"));

    viewport_ = new ViewportWidget(this);
    viewport_->setRenderAdapter(renderer_);
    setCentralWidget(viewport_);

    connect(viewport_, &ViewportWidget::glReady, this, &MainWindow::onGlReady);

    setupMenus();
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
    // TODO(@dev, sprint-0.5): clear ECS + event store, reset viewport
    QMessageBox::information(this, tr("New"), tr("New scene — not yet implemented."));
}

void MainWindow::onFileSaveAs() {
    // TODO(@dev, sprint-0.5): persist event store → .mycad ZIP via MycadFileStore
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save As"), currentFilePath_, tr("myCad Files (*.mycad)"));
    if (path.isEmpty()) {
        return;
    }
    QMessageBox::information(
        this, tr("Save As"), tr("Save As not yet implemented.\nChosen path: %1").arg(path));
}

void MainWindow::onFileOpen() {
    // TODO(@dev, sprint-0.5): unpack .mycad ZIP, replay events, rebuild scene
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Open"), QString(), tr("myCad Files (*.mycad)"));
    if (path.isEmpty()) {
        return;
    }
    QMessageBox::information(
        this, tr("Open"), tr("Open not yet implemented.\nChosen path: %1").arg(path));
}

}  // namespace mycad::ui
