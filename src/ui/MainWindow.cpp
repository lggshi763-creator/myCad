#include <mycad/application/commands/CreateBoxCommand.hpp>
#include <mycad/domain/AggregateId.hpp>
#include <mycad/ui/MainWindow.hpp>
#include <mycad/ui/ViewportWidget.hpp>

namespace mycad::ui {

MainWindow::MainWindow(std::shared_ptr<mycad::application::CommandBus> bus,
                       std::shared_ptr<mycad::infrastructure::OpenGLRenderAdapter> renderer,
                       QWidget* parent)
    : QMainWindow(parent), bus_{std::move(bus)}, renderer_{std::move(renderer)} {
    setWindowTitle(QStringLiteral("myCad — Sprint 0.4"));

    viewport_ = new ViewportWidget(this);
    viewport_->setRenderAdapter(renderer_);
    setCentralWidget(viewport_);

    connect(viewport_, &ViewportWidget::glReady, this, &MainWindow::onGlReady);
}

MainWindow::~MainWindow() = default;

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

    // Trigger a repaint to show the uploaded mesh.
    viewport_->update();
}

}  // namespace mycad::ui
