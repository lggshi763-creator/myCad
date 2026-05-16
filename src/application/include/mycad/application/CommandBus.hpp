#pragma once

#include <mycad/application/CommandContext.hpp>
#include <mycad/application/CommandError.hpp>
#include <mycad/application/ICommandHandler.hpp>
#include <mycad/domain/IEventStore.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>

/// @file CommandBus — type-safe command dispatcher (DDD application layer).

namespace mycad::application {

/// @brief Routes commands to their registered handlers.
///
/// Handlers are registered once at startup via registerHandler<C>().
/// Commands are dispatched synchronously via send<C>().  The bus is
/// thread-safe: concurrent send() calls from different threads are
/// serialised with an internal mutex.
///
/// Design:
/// - Type-erased internally (unordered_map<type_index, HandlerEntry>).
/// - Type-safe externally: send<C> / registerHandler<C> use the
///   concrete command type as the key — no dynamic_cast at call sites.
/// - CQRS: commands carry no return data; callers query state via
///   eventStore() or ECS after a successful dispatch.
class CommandBus {
public:
    /// @brief Constructs the bus with an event store it exposes to handlers.
    explicit CommandBus(std::shared_ptr<domain::IEventStore> store);
    ~CommandBus();

    // Non-copyable, non-movable (handlers hold raw refs internally).
    CommandBus(const CommandBus&) = delete;
    CommandBus& operator=(const CommandBus&) = delete;

    /// @brief Registers a handler for command type C.
    ///
    /// Replaces any previously registered handler for C.
    /// @tparam C  Command type (must satisfy the Command concept).
    template <Command C>
    void registerHandler(std::shared_ptr<ICommandHandler<C>> handler) {
        std::lock_guard lock{mutex_};
        handlers_[std::type_index(typeid(C))] =
            HandlerEntry{handler,
                         [h = std::move(handler)](const void* cmdPtr,
                                                  const CommandContext& ctx) -> CommandResult {
                             return h->handle(*static_cast<const C*>(cmdPtr), ctx);
                         }};
    }

    /// @brief Dispatches a command synchronously to its registered handler.
    ///
    /// @return CommandError{HandlerNotFound} if no handler is registered.
    ///         Otherwise the handler's own CommandResult.
    template <Command C>
    CommandResult send(C cmd, CommandContext ctx = {}) {
        std::lock_guard lock{mutex_};
        auto it = handlers_.find(std::type_index(typeid(C)));
        if (it == handlers_.end()) {
            return std::unexpected(CommandError{CommandErrorKind::HandlerNotFound,
                                                std::string("no handler for ") + typeid(C).name()});
        }
        return it->second.invoker(&cmd, ctx);
    }

    /// @brief Returns the event store injected at construction.
    [[nodiscard]] std::shared_ptr<domain::IEventStore> eventStore() const noexcept;

private:
    struct HandlerEntry {
        std::shared_ptr<void> handler;  ///< Keeps handler alive.
        std::function<CommandResult(const void*, const CommandContext&)> invoker;
    };

    std::shared_ptr<domain::IEventStore> store_;
    std::unordered_map<std::type_index, HandlerEntry> handlers_;
    std::mutex mutex_;
};

}  // namespace mycad::application
