#pragma once

#include <mycad/domain/AggregateId.hpp>
#include <mycad/domain/EventId.hpp>
#include <mycad/domain/Version.hpp>

#include <cstdint>
#include <string>

/// @file Abstract base for all domain events (ADR-0003).

namespace mycad::domain {

/// @brief Immutable base class for all domain events.
///
/// 所有领域事件继承此类。一旦构造只读 —— 不提供 setter，拷贝/赋值被删除。
/// 时间戳使用 Unix 毫秒 (uint64_t)，避免引入 <chrono>（ADR-0002）。
///
/// @thread-safe
class DomainEvent {
public:
    DomainEvent(const DomainEvent&) = delete;
    DomainEvent& operator=(const DomainEvent&) = delete;
    DomainEvent(DomainEvent&&) = delete;
    DomainEvent& operator=(DomainEvent&&) = delete;

    virtual ~DomainEvent() = default;

    /// @brief Returns the fully-qualified event type name, e.g. "mycad.sketch.PointAdded".
    [[nodiscard]] virtual std::string typeName() const noexcept = 0;

    [[nodiscard]] EventId eventId() const noexcept {
        return eventId_;
    }
    [[nodiscard]] AggregateId aggregateId() const noexcept {
        return aggregateId_;
    }
    [[nodiscard]] Version aggregateVersion() const noexcept {
        return aggregateVersion_;
    }
    /// @brief Unix timestamp in milliseconds.
    [[nodiscard]] std::uint64_t occurredAtMs() const noexcept {
        return occurredAtMs_;
    }

protected:
    /// @param aggregateId      The aggregate that raised this event.
    /// @param aggregateVersion The aggregate version after applying this event.
    /// @param occurredAtMs     Unix timestamp in milliseconds (caller-supplied for testability).
    explicit DomainEvent(AggregateId aggregateId,
                         Version aggregateVersion,
                         std::uint64_t occurredAtMs) noexcept
        : eventId_(EventId::generate()), aggregateId_(aggregateId),
          aggregateVersion_(aggregateVersion), occurredAtMs_(occurredAtMs) {}

private:
    EventId eventId_;
    AggregateId aggregateId_;
    Version aggregateVersion_;
    std::uint64_t occurredAtMs_;
};

}  // namespace mycad::domain

/// @brief Declares typeName() for a concrete event class.
///
/// Always forces public access for typeName(). Place anywhere in the class body.
///
/// @code
///   class PointAdded : public DomainEvent {
///       MYCAD_DOMAIN_EVENT("mycad.sketch.PointAdded")
///   public:
///       explicit PointAdded(...);
///   };
/// @endcode
#define MYCAD_DOMAIN_EVENT(type_string)                            \
public:                                                            \
    [[nodiscard]] std::string typeName() const noexcept override { \
        return type_string;                                        \
    }
