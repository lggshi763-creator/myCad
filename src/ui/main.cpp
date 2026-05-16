#include <mycad/application/CommandBus.hpp>
#include <mycad/application/handlers/CreateBoxCommandHandler.hpp>
#include <mycad/domain/BRepHandle.hpp>
#include <mycad/domain/EventTypeRegistry.hpp>
#include <mycad/domain/events/BoxCreatedEvent.hpp>
#include <mycad/infrastructure/EnttRegistry.hpp>
#include <mycad/infrastructure/OcctGeometryAdapter.hpp>
#include <mycad/infrastructure/OpenGLRenderAdapter.hpp>
#include <mycad/infrastructure/SqliteEventStore.hpp>
#include <mycad/ui/MainWindow.hpp>

#include <QApplication>
#include <QDir>
#include <QSurfaceFormat>

int main(int argc, char* argv[]) {
    // Set OpenGL 4.5 core-profile format before creating QApplication.
    QSurfaceFormat fmt;
    fmt.setVersion(4, 5);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setDepthBufferSize(24);
    fmt.setSamples(4);  // 4x MSAA
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);

    // --- Composition root ---

    // Persist the session database in the OS temp directory.
    const std::string dbPath =
        (QDir::tempPath() + QStringLiteral("/mycad_session.db")).toStdString();
    auto store = std::make_shared<mycad::infrastructure::SqliteEventStore>(dbPath);

    // Register JSON serializer for BoxCreatedEvent so Save As persists dimensions.
    store->registerSerializer(
        "mycad.box.BoxCreated", [](const mycad::domain::DomainEvent& base) -> std::string {
            const auto& ev = static_cast<const mycad::domain::BoxCreatedEvent&>(base);
            return "{\"dx\":" + std::to_string(ev.dx) + ",\"dy\":" + std::to_string(ev.dy) +
                   ",\"dz\":" + std::to_string(ev.dz) +
                   ",\"handle_id\":" + std::to_string(ev.handle.id) + "}";
        });

    // Register BoxCreatedEvent factory so load() can reconstruct event shells.
    // T5 (File -> Open) will use loadRows() + payload parsing for full geometry replay.
    mycad::domain::EventTypeRegistry::instance().registerType(
        "mycad.box.BoxCreated",
        [](mycad::domain::AggregateId id,
           mycad::domain::Version ver,
           std::uint64_t ts) -> std::unique_ptr<mycad::domain::DomainEvent> {
            return std::make_unique<mycad::domain::BoxCreatedEvent>(
                id, ver, ts, 0.0, 0.0, 0.0, mycad::domain::BRepHandle{});
        });

    auto geom = std::make_shared<mycad::infrastructure::OcctGeometryAdapter>();
    auto ecs = std::make_shared<mycad::infrastructure::EnttRegistry>();
    auto renderer = std::make_shared<mycad::infrastructure::OpenGLRenderAdapter>();
    auto bus = std::make_shared<mycad::application::CommandBus>(store);

    // Pass renderer so the handler tessellates and uploads the mesh after each command.
    auto handler = std::make_shared<mycad::application::handlers::CreateBoxCommandHandler>(
        geom, ecs, store, renderer);
    bus->registerHandler<mycad::application::commands::CreateBoxCommand>(handler);

    // --- Main window ---
    mycad::ui::MainWindow win(bus, renderer, store, ecs, geom);
    win.resize(1280, 720);
    win.show();

    return QApplication::exec();
}
