#pragma once

#include <mycad/domain/IEventStore.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/// @file SQLite-backed implementation of IEventStore (Sprint 0.5).

namespace mycad::infrastructure {

/// @brief Persistent event store backed by a SQLite database file.
///
/// Schema (single table):
/// @code
///   events(id INTEGER PK, aggregate_id BLOB, version INTEGER,
///          type_name TEXT, payload TEXT, occurred_at INTEGER)
/// @endcode
///
/// Payload is a JSON string serialised at append time via a registered
/// SerializerFn.  load() reconstructs events via EventTypeRegistry
/// (metadata only).  Full payload access is via loadRows().
///
/// @thread-safe  — protected by an internal mutex.
/// @see IEventStore for the port contract.
class SqliteEventStore final : public domain::IEventStore {
public:
    /// @brief Opens or creates the SQLite database at dbPath.
    /// @throws std::runtime_error if the file cannot be opened or the schema
    ///         cannot be created.
    explicit SqliteEventStore(std::string_view dbPath);
    ~SqliteEventStore() override;

    // -------------------------------------------------------------------------
    // IEventStore overrides
    // -------------------------------------------------------------------------

    void append(domain::AggregateId aggregateId,
                domain::Version expectedVersion,
                std::span<const domain::DomainEvent* const> events) override;

    [[nodiscard]] std::vector<std::unique_ptr<domain::DomainEvent>>
    load(domain::AggregateId aggregateId) override;

    [[nodiscard]] std::vector<std::unique_ptr<domain::DomainEvent>>
    loadSince(domain::AggregateId aggregateId, domain::Version fromVersion) override;

    [[nodiscard]] domain::Version latestVersion(domain::AggregateId aggregateId) override;

    // -------------------------------------------------------------------------
    // Payload serialisation
    // -------------------------------------------------------------------------

    /// @brief JSON serialiser for a concrete event type.
    ///
    /// Called during append() to produce the payload column value.
    /// Register at the composition root for each concrete event type that
    /// carries business data beyond the base metadata.
    using SerializerFn = std::function<std::string(const domain::DomainEvent&)>;

    /// @brief Registers a JSON serialiser for the given event type name.
    ///
    /// If no serialiser is registered for a type, payload defaults to "{}".
    void registerSerializer(std::string typeName, SerializerFn fn);

    // -------------------------------------------------------------------------
    // Raw access for payload-aware replay (used by T5 File → Open)
    // -------------------------------------------------------------------------

    /// @brief Raw event row as stored in SQLite.
    struct RawRow {
        domain::AggregateId aggregateId;
        domain::Version version;
        std::string typeName;
        std::string payload;  ///< JSON string; "{}" when no serialiser was registered.
        std::uint64_t occurredAtMs;
    };

    /// @brief Returns all stored rows for an aggregate, ordered by version ascending.
    ///
    /// Provides access to the JSON payload so replay logic can reconstruct
    /// full business-data fields (e.g. BoxCreatedEvent::dx / dy / dz).
    [[nodiscard]] std::vector<RawRow> loadRows(domain::AggregateId aggregateId) const;

    /// @brief Returns the database file path passed to the constructor.
    [[nodiscard]] std::string_view dbPath() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace mycad::infrastructure
