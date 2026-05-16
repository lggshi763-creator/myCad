#pragma once

#include <mycad/domain/BRepHandle.hpp>
#include <mycad/domain/DomainEvent.hpp>

#include <cstdint>

/// @file BoxCreatedEvent — domain event emitted when a box solid is created.

namespace mycad::domain {

/// @brief Emitted by CreateBoxCommandHandler when a parametric box is created.
///
/// Carries the three dimensions and the infrastructure BRepHandle (opaque id).
/// The handle is stored so that projections (render adapter, ECS) can later
/// retrieve or tessellate the geometry without re-running makeBox.
///
/// Payload serialization is deferred to Sprint 0.5 (FlatBuffers); the
/// InMemoryEventStore stores the fields in-process only.
class BoxCreatedEvent final : public DomainEvent {
    MYCAD_DOMAIN_EVENT("mycad.box.BoxCreated")

public:
    /// @param aggregateId      The aggregate this box belongs to.
    /// @param aggregateVersion Version after applying this event.
    /// @param occurredAtMs     Unix timestamp in milliseconds.
    /// @param dx dy dz         Box dimensions (mm, must be > 0).
    /// @param handle           Opaque geometry handle from IGeometryConstructionPort.
    BoxCreatedEvent(AggregateId aggregateId,
                    Version aggregateVersion,
                    std::uint64_t occurredAtMs,
                    double dx_,
                    double dy_,
                    double dz_,
                    BRepHandle handle_) noexcept
        : DomainEvent{aggregateId, aggregateVersion, occurredAtMs}, dx{dx_}, dy{dy_}, dz{dz_},
          handle{handle_} {}

    double dx{0.0};     ///< Width  (mm).
    double dy{0.0};     ///< Depth  (mm).
    double dz{0.0};     ///< Height (mm).
    BRepHandle handle;  ///< Opaque geometry reference.
};

}  // namespace mycad::domain
