#include <mycad/application/handlers/CreateBoxCommandHandler.hpp>
#include <mycad/domain/events/BoxCreatedEvent.hpp>

#include <chrono>

namespace mycad::application::handlers {

CreateBoxCommandHandler::CreateBoxCommandHandler(
    std::shared_ptr<domain::IGeometryConstructionPort> geom,
    std::shared_ptr<domain::IEntityRegistry> ecs,
    std::shared_ptr<domain::IEventStore> store,
    std::shared_ptr<domain::IRenderPort> renderer)
    : geom_{std::move(geom)}, ecs_{std::move(ecs)}, store_{std::move(store)},
      renderer_{std::move(renderer)} {}

CommandResult CreateBoxCommandHandler::handle(const commands::CreateBoxCommand& cmd,
                                              const CommandContext& /*ctx*/) {
    // --- 1. Validate domain invariants ---
    if (cmd.dx <= 0.0 || cmd.dy <= 0.0 || cmd.dz <= 0.0) {
        return std::unexpected(CommandError{
            CommandErrorKind::BusinessRuleViolated,
            "Box dimensions must be strictly positive (got dx=" + std::to_string(cmd.dx) +
                " dy=" + std::to_string(cmd.dy) + " dz=" + std::to_string(cmd.dz) + ")"});
    }

    // --- 2. Build geometry via port ---
    auto geomResult = geom_->makeBox(cmd.dx, cmd.dy, cmd.dz);
    if (!geomResult) {
        return std::unexpected(CommandError{CommandErrorKind::AggregateLoadFailed,
                                            "makeBox failed: " + geomResult.error().message});
    }
    const domain::BRepHandle handle = *geomResult;

    // --- 3. Create ECS entity to track this solid ---
    ecs_->create();

    // --- 4. Determine expected version (first event → Version{0}) ---
    const domain::Version expectedVersion = store_->latestVersion(cmd.targetId);
    const domain::Version nextVersion = expectedVersion.next();

    // --- 5. Append BoxCreatedEvent ---
    const auto nowMs =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count());

    const domain::BoxCreatedEvent evt{
        cmd.targetId, nextVersion, nowMs, cmd.dx, cmd.dy, cmd.dz, handle};

    try {
        store_->appendOne(cmd.targetId, expectedVersion, evt);
    } catch (const domain::ConcurrencyError& e) {
        return std::unexpected(CommandError{CommandErrorKind::ConcurrencyConflict, e.what()});
    }

    // --- 6. Tessellate + upload to renderer (optional) ---
    if (renderer_) {
        auto meshResult = geom_->tessellate(handle, tessParams);
        if (!meshResult) {
            // Tessellation failure is non-fatal: event is already persisted.
            // Log-worthy in production; for Sprint 0.4 we silently skip upload.
            return {};
        }
        renderer_->uploadMesh(handle, *meshResult);
    }

    return {};  // success
}

}  // namespace mycad::application::handlers
