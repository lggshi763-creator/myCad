#pragma once

#include <mycad/application/ICommandHandler.hpp>
#include <mycad/application/commands/CreateBoxCommand.hpp>
#include <mycad/domain/IEntityRegistry.hpp>
#include <mycad/domain/IEventStore.hpp>
#include <mycad/domain/IGeometryConstructionPort.hpp>

#include <memory>

/// @file CreateBoxCommandHandler — application handler for CreateBoxCommand.

namespace mycad::application::handlers {

/// @brief Handles CreateBoxCommand: validate → makeBox → create entity → store event.
///
/// Depends only on domain ports (IGeometryConstructionPort, IEntityRegistry,
/// IEventStore). Concrete adapters (OcctGeometryAdapter, EnttRegistry,
/// InMemoryEventStore) are injected at the composition root.
class CreateBoxCommandHandler final : public ICommandHandler<commands::CreateBoxCommand> {
public:
    /// @param geom   Port for building B-Rep geometry.
    /// @param ecs    Port for creating/tracking ECS entities.
    /// @param store  Port for persisting domain events.
    CreateBoxCommandHandler(std::shared_ptr<domain::IGeometryConstructionPort> geom,
                            std::shared_ptr<domain::IEntityRegistry> ecs,
                            std::shared_ptr<domain::IEventStore> store);

    /// @brief Validates dimensions, builds geometry, and appends BoxCreatedEvent.
    ///
    /// @return CommandError{BusinessRuleViolated} if any dimension ≤ 0.
    /// @return CommandError{AggregateLoadFailed}  if makeBox fails.
    /// @return CommandError{ConcurrencyConflict}  if version mismatch on append.
    CommandResult handle(const commands::CreateBoxCommand& cmd, const CommandContext& ctx) override;

private:
    std::shared_ptr<domain::IGeometryConstructionPort> geom_;
    std::shared_ptr<domain::IEntityRegistry> ecs_;
    std::shared_ptr<domain::IEventStore> store_;
};

}  // namespace mycad::application::handlers
