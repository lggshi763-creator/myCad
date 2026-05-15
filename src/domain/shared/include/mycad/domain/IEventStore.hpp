#pragma once

#include <mycad/domain/AggregateId.hpp>
#include <mycad/domain/DomainEvent.hpp>
#include <mycad/domain/Version.hpp>

#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

/// @file Domain port for event-stream persistence (ADR-0003 §6).

namespace mycad::domain {

/// @brief Thrown by IEventStore::append when the stored version differs from expectedVersion.
class ConcurrencyError : public std::runtime_error {
public:
    explicit ConcurrencyError(AggregateId id, Version expected, Version actual)
        : std::runtime_error("Optimistic concurrency conflict"), aggregateId_(id),
          expected_(expected), actual_(actual) {}

    [[nodiscard]] AggregateId aggregateId() const noexcept {
        return aggregateId_;
    }
    [[nodiscard]] Version expected() const noexcept {
        return expected_;
    }
    [[nodiscard]] Version actual() const noexcept {
        return actual_;
    }

private:
    AggregateId aggregateId_;
    Version expected_;
    Version actual_;
};

/// @brief Port for appending and loading domain-event streams.
///
/// 乐观并发：append 时传入调用方期望的当前版本；
/// 若存储中的实际版本不符，抛出 ConcurrencyError。
///
/// @thread-safe 实现类应保证线程安全。
class IEventStore {
public:
    virtual ~IEventStore() = default;

    IEventStore(const IEventStore&) = delete;
    IEventStore& operator=(const IEventStore&) = delete;

    /// @brief Appends events atomically to the aggregate's stream.
    ///
    /// @param expectedVersion Version the caller believes is current.
    ///                        Pass Version{0} for a brand-new aggregate.
    /// @throws ConcurrencyError if the stored version != expectedVersion.
    /// @throws std::bad_alloc
    virtual void append(AggregateId aggregateId,
                        Version expectedVersion,
                        std::span<const DomainEvent* const> events) = 0;

    /// @brief Loads all events for an aggregate, ordered by version ascending.
    ///
    /// @return Empty vector if no events exist for aggregateId.
    [[nodiscard]] virtual std::vector<std::unique_ptr<DomainEvent>>
    load(AggregateId aggregateId) = 0;

    /// @brief Loads events starting at (and including) fromVersion.
    [[nodiscard]] virtual std::vector<std::unique_ptr<DomainEvent>>
    loadSince(AggregateId aggregateId, Version fromVersion) = 0;

    /// @brief Returns the latest stored version for an aggregate.
    ///
    /// @return Version{0} if no events have been stored yet.
    [[nodiscard]] virtual Version latestVersion(AggregateId aggregateId) = 0;

protected:
    IEventStore() = default;
};

}  // namespace mycad::domain
