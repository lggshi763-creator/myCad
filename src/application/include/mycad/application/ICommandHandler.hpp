#pragma once

#include <mycad/application/CommandContext.hpp>
#include <mycad/application/CommandError.hpp>

#include <concepts>
#include <type_traits>

/// @file ICommandHandler — per-command handler interface.

namespace mycad::application {

/// @brief Concept satisfied by any movable class type usable as a command.
template <typename C>
concept Command = std::is_class_v<C> && std::is_move_constructible_v<C>;

/// @brief Pure-virtual handler for a single command type C.
///
/// One handler per command type (DDD principle).  Handlers live in the
/// application layer and may depend on infrastructure adapters through
/// constructor injection.
///
/// @tparam C  The concrete command struct this handler processes.
template <Command C>
class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;

    /// @brief Executes the command within the given context.
    ///
    /// @param cmd  The command value to process.
    /// @param ctx  Ambient caller context (userId, source, …).
    /// @return Empty success or a CommandError describing the failure.
    virtual CommandResult handle(const C& cmd, const CommandContext& ctx) = 0;

protected:
    ICommandHandler() = default;
    ICommandHandler(const ICommandHandler&) = default;
    ICommandHandler& operator=(const ICommandHandler&) = default;
};

}  // namespace mycad::application
