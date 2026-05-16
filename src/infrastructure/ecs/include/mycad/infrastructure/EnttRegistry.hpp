#pragma once

#include <mycad/domain/IEntityRegistry.hpp>

// entt is included here because emplace<T>/tryGet<T>/remove<T> are templates
// whose bodies must be visible at call sites.  The ADR-0007 constraint is that
// entt::registry INSTANCES must not live outside infrastructure/ecs/;
// including the header in infrastructure consumers is acceptable.
#include <entt/entt.hpp>

#include <cstddef>

/// @file EnTT-backed implementation of IEntityRegistry (ADR-0007).

namespace mycad::infrastructure {

/// @brief Wraps entt::registry and implements the IEntityRegistry port.
///
/// The five IEntityRegistry virtuals are implemented in EnttRegistry.cpp.
/// Three ECS extension templates (emplace / tryGet / remove) are defined
/// inline here so callers can use arbitrary component types.
///
/// Domain and application code must depend on IEntityRegistry only.
/// Only infrastructure code that already transitively includes this header
/// should call emplace / tryGet / remove.
///
/// @see IEntityRegistry for the port contract visible to domain / application.
class EnttRegistry final : public domain::IEntityRegistry {
public:
    EnttRegistry();
    ~EnttRegistry() override;

    // -------------------------------------------------------------------------
    // IEntityRegistry virtuals — implemented in EnttRegistry.cpp
    // -------------------------------------------------------------------------

    /// @brief Creates a new entity.
    /// @throws std::bad_alloc
    [[nodiscard]] domain::EntityId create() override;

    /// @brief Destroys the entity.  No-op for kNullEntity or already-dead ids.
    /// @noexcept-ok
    void destroy(domain::EntityId id) noexcept override;

    /// @brief Returns true if the entity is alive.
    /// @noexcept-ok
    [[nodiscard]] bool isAlive(domain::EntityId id) const noexcept override;

    /// @brief Returns the number of currently alive entities.
    /// @noexcept-ok
    [[nodiscard]] std::size_t size() const noexcept override;

    /// @brief Destroys all entities and clears all component pools.
    /// @noexcept-ok
    void clear() noexcept override;

    // -------------------------------------------------------------------------
    // ECS extension templates — infrastructure-layer only
    // -------------------------------------------------------------------------

    /// @brief Adds (or replaces) component T on the entity.
    ///
    /// @tparam T   Component type.
    /// @param  id  A live entity id (must be valid; UB otherwise).
    /// @param  args Constructor arguments forwarded to T.
    /// @return Reference to the emplaced component.
    template <typename T, typename... Args>
    T& emplace(domain::EntityId id, Args&&... args) {
        return registry_.emplace_or_replace<T>(static_cast<entt::entity>(id),
                                               std::forward<Args>(args)...);
    }

    /// @brief Returns a pointer to component T on the entity, or nullptr.
    /// @noexcept-ok
    template <typename T>
    [[nodiscard]] T* tryGet(domain::EntityId id) noexcept {
        return registry_.try_get<T>(static_cast<entt::entity>(id));
    }

    /// @brief Removes component T from the entity.  No-op if not present.
    /// @noexcept-ok
    template <typename T>
    void remove(domain::EntityId id) noexcept {
        registry_.remove<T>(static_cast<entt::entity>(id));
    }

private:
    entt::registry registry_;
    std::size_t aliveCount_{0};
};

}  // namespace mycad::infrastructure
