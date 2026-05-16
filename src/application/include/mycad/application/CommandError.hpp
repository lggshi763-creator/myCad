#pragma once

#include <expected>
#include <string>

/// @file CommandBus error type and result alias (ADR application layer).

namespace mycad::application {

/// @brief Reason codes for command dispatch failures.
enum class CommandErrorKind {
    HandlerNotFound,       ///< No handler registered for this command type.
    AggregateLoadFailed,   ///< Event store could not reconstruct the aggregate.
    BusinessRuleViolated,  ///< Domain invariant rejected the command.
    ConcurrencyConflict,   ///< Optimistic-concurrency version mismatch.
    HandlerThrew,          ///< Handler threw an unexpected exception.
};

/// @brief Error value carried by a failed CommandResult.
struct CommandError {
    CommandErrorKind kind{CommandErrorKind::HandlerThrew};
    std::string message;
};

/// @brief Result of dispatching a command through the CommandBus.
///
/// Commands follow CQRS: success carries no return value; callers
/// query state via the event store or ECS after a successful dispatch.
using CommandResult = std::expected<void, CommandError>;

}  // namespace mycad::application
