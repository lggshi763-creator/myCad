#pragma once

#include <mycad/domain/AggregateId.hpp>

/// @file CreateBoxCommand — command that creates a parametric box solid.

namespace mycad::application::commands {

/// @brief Requests creation of an axis-aligned box with the given dimensions.
///
/// The handler validates that all dimensions are strictly positive, calls
/// IGeometryConstructionPort::makeBox, creates an ECS entity, and appends
/// a BoxCreatedEvent to the event store.
struct CreateBoxCommand {
    double dx{1.0};                ///< Width  (mm, must be > 0).
    double dy{1.0};                ///< Depth  (mm, must be > 0).
    double dz{1.0};                ///< Height (mm, must be > 0).
    domain::AggregateId targetId;  ///< Aggregate id for the new solid.
};

}  // namespace mycad::application::commands
