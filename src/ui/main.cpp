#include <mycad/application/CommandBus.hpp>
#include <mycad/application/handlers/CreateBoxCommandHandler.hpp>
#include <mycad/infrastructure/EnttRegistry.hpp>
#include <mycad/infrastructure/InMemoryEventStore.hpp>
#include <mycad/infrastructure/OcctGeometryAdapter.hpp>
#include <mycad/infrastructure/OpenGLRenderAdapter.hpp>
#include <mycad/ui/MainWindow.hpp>

#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char* argv[]) {
    // Set OpenGL 4.5 core-profile format before creating QApplication.
    QSurfaceFormat fmt;
    fmt.setVersion(4, 5);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setDepthBufferSize(24);
    fmt.setSamples(4);  // 4× MSAA
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);

    // --- Composition root ---
    auto store = std::make_shared<mycad::infrastructure::InMemoryEventStore>();
    auto geom = std::make_shared<mycad::infrastructure::OcctGeometryAdapter>();
    auto ecs = std::make_shared<mycad::infrastructure::EnttRegistry>();
    auto renderer = std::make_shared<mycad::infrastructure::OpenGLRenderAdapter>();
    auto bus = std::make_shared<mycad::application::CommandBus>(store);

    // Pass renderer so the handler tessellates and uploads the mesh after each command.
    auto handler = std::make_shared<mycad::application::handlers::CreateBoxCommandHandler>(
        geom, ecs, store, renderer);
    bus->registerHandler<mycad::application::commands::CreateBoxCommand>(handler);

    // --- Main window ---
    mycad::ui::MainWindow win(bus, renderer);
    win.resize(1280, 720);
    win.show();

    return QApplication::exec();
}
