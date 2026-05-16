#pragma once

#include <mycad/application/ICommandHandler.hpp>
#include <mycad/application/commands/CreateBoxCommand.hpp>
#include <mycad/domain/IEntityRegistry.hpp>
#include <mycad/domain/IEventStore.hpp>
#include <mycad/domain/IGeometryConstructionPort.hpp>
#include <mycad/domain/IRenderPort.hpp>
#include <mycad/domain/TessellationParams.hpp>

#include <memory>

/// @file CreateBoxCommandHandler — application handler for CreateBoxCommand.

namespace mycad::application::handlers {

/// @brief Handles CreateBoxCommand: validate → makeBox → entity → event → (optional) render.
///
/// Depends only on domain ports (IGeometryConstructionPort, IEntityRegistry,
/// IEventStore, IRenderPort). Concrete adapters are injected at the composition root.
///
/// When a renderer is supplied, the handler tessellates the new solid and uploads
/// the resulting TriangleMesh to the render adapter immediately after the event
/// is persisted.  Passing nullptr for renderer skips tessellation (useful in tests
/// that do not exercise the rendering path).
class CreateBoxCommandHandler final : public ICommandHandler<commands::CreateBoxCommand> {
public:
    /// @param geom      Port for building B-Rep geometry.
    /// @param ecs       Port for creating/tracking ECS entities.
    /// @param store     Port for persisting domain events.
    /// @param renderer  Optional render port.  Pass nullptr to skip mesh upload.
    CreateBoxCommandHandler(std::shared_ptr<domain::IGeometryConstructionPort> geom,
                            std::shared_ptr<domain::IEntityRegistry> ecs,
                            std::shared_ptr<domain::IEventStore> store,
                            std::shared_ptr<domain::IRenderPort> renderer = nullptr);

    /// @brief Validates dimensions, builds geometry, persists event, uploads mesh.
    ///
    /// @return CommandError{BusinessRuleViolated} if any dimension ≤ 0.
    /// @return CommandError{AggregateLoadFailed}  if makeBox or tessellate fails.
    /// @return CommandError{ConcurrencyConflict}  if version mismatch on append.
    CommandResult handle(const commands::CreateBoxCommand& cmd, const CommandContext& ctx) override;

    /// @brief Tessellation quality used when uploading meshes to the renderer.
    domain::TessellationParams tessParams;

private:
    std::shared_ptr<domain::IGeometryConstructionPort> geom_;
    std::shared_ptr<domain::IEntityRegistry> ecs_;
    std::shared_ptr<domain::IEventStore> store_;
    std::shared_ptr<domain::IRenderPort> renderer_;  ///< May be nullptr.
};

}  // namespace mycad::application::handlers
