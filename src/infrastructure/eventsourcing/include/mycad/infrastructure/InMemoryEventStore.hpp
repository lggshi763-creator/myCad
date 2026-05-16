#pragma once

#include <mycad/domain/IEventStore.hpp>

#include <memory>

/// @file In-memory implementation of IEventStore for testing and development.

namespace mycad::infrastructure {

/// @brief Thread-safe in-memory event store backed by std::unordered_map.
///
/// Serialises each event to a plain struct (typeName + scalar fields).
/// Deserialisation reconstructs events via EventTypeRegistry::instance();
/// all event types used with load() must be registered before the call.
///
/// Payload (JSON) is a placeholder for Sprint 0.3 — no field-level data is
/// persisted. Sprint 0.5 will replace this with FlatBuffers.
///
/// @see IEventStore for the full port contract.
class InMemoryEventStore final : public domain::IEventStore {
public:
    InMemoryEventStore();
    ~InMemoryEventStore() override;

    void append(domain::AggregateId aggregateId,
                domain::Version expectedVersion,
                std::span<const domain::DomainEvent* const> events) override;

    [[nodiscard]] std::vector<std::unique_ptr<domain::DomainEvent>>
    load(domain::AggregateId aggregateId) override;

    [[nodiscard]] std::vector<std::unique_ptr<domain::DomainEvent>>
    loadSince(domain::AggregateId aggregateId, domain::Version fromVersion) override;

    [[nodiscard]] domain::Version latestVersion(domain::AggregateId aggregateId) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace mycad::infrastructure
