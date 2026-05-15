#pragma once

#include <mycad/domain/DomainEvent.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

/// @file Singleton registry mapping event type names to factory functions (ADR-0003).

namespace mycad::domain {

/// @brief Maps event type-name strings to factory functions for deserialisation.
///
/// 单例。使用前通过 MYCAD_REGISTER_EVENT 宏注册各具体事件的工厂；
/// 反序列化路径通过 findFactory() 查询。
class EventTypeRegistry {
public:
    /// @brief Factory signature: (aggregateId, version, occurredAtMs) → owning pointer.
    using FactoryFn =
        std::function<std::unique_ptr<DomainEvent>(AggregateId, Version, std::uint64_t)>;

    /// @brief Returns the process-wide singleton instance.
    [[nodiscard]] static EventTypeRegistry& instance() noexcept;

    EventTypeRegistry(const EventTypeRegistry&) = delete;
    EventTypeRegistry& operator=(const EventTypeRegistry&) = delete;

    /// @brief Registers a factory for the given type name.
    ///
    /// 若已注册同名类型，新注册覆盖旧值（幂等）。
    void registerType(std::string typeName, FactoryFn factory);

    /// @brief Returns the factory for typeName, or nullptr if not registered.
    [[nodiscard]] const FactoryFn* findFactory(const std::string& typeName) const;

    /// @brief Returns the number of registered event types.
    [[nodiscard]] std::size_t size() const noexcept {
        return registry_.size();
    }

    /// @brief Removes all registrations (useful in tests).
    void clear() noexcept {
        registry_.clear();
    }

    /// @brief Default-constructs an empty registry (for test-local instances).
    EventTypeRegistry() = default;

private:
    std::unordered_map<std::string, FactoryFn> registry_;
};

}  // namespace mycad::domain

/// @brief Registers a concrete DomainEvent subclass with EventTypeRegistry.
///
/// Place this macro in the .cpp translation unit of the concrete event.
/// The event class must have a constructor (AggregateId, Version, uint64_t).
///
/// @code
///   MYCAD_REGISTER_EVENT(PointAdded)
/// @endcode
#define MYCAD_REGISTER_EVENT(ClassName)                                                          \
    namespace {                                                                                  \
    [[maybe_unused]] const bool kRegistered_##ClassName = []() {                                 \
        ::mycad::domain::EventTypeRegistry::instance().registerType(                             \
            ClassName{::mycad::domain::AggregateId{}, ::mycad::domain::Version{}, 0}.typeName(), \
            [](::mycad::domain::AggregateId id,                                                  \
               ::mycad::domain::Version ver,                                                     \
               std::uint64_t ts) -> std::unique_ptr<::mycad::domain::DomainEvent> {              \
                return std::make_unique<ClassName>(id, ver, ts);                                 \
            });                                                                                  \
        return true;                                                                             \
    }();                                                                                         \
    }  // namespace
