#pragma once

#include <cstddef>
#include <cstdint>

/// @file Domain port for ECS entity lifecycle (ADR-0007).
///
/// EnTT 实例仅由 infrastructure/ecs/ 持有；domain 通过此接口请求实体管理。

namespace mycad::domain {

/// @brief Opaque entity identifier (mirrors entt::entity underlying type).
using EntityId = std::uint32_t;

/// @brief Sentinel value meaning "no entity".
inline constexpr EntityId kNullEntity = static_cast<EntityId>(~std::uint32_t{0});

/// @brief Port for creating, destroying, and querying ECS entities.
///
/// Domain 只感知实体的存在与否，不直接操作 EnTT registry 或 component 数据。
///
/// @thread-safe 实现类应保证线程安全。
class IEntityRegistry {
public:
    virtual ~IEntityRegistry() = default;

    IEntityRegistry(const IEntityRegistry&) = delete;
    IEntityRegistry& operator=(const IEntityRegistry&) = delete;

    /// @brief Creates a new entity and returns its ID.
    /// @throws std::bad_alloc
    [[nodiscard]] virtual EntityId create() = 0;

    /// @brief Destroys the entity with the given ID.
    ///
    /// No-op if id == kNullEntity or the entity is already destroyed.
    virtual void destroy(EntityId id) noexcept = 0;

    /// @brief Returns true if the entity exists and has not been destroyed.
    [[nodiscard]] virtual bool isAlive(EntityId id) const noexcept = 0;

    /// @brief Returns the number of currently alive entities.
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;

    /// @brief Destroys all entities.
    virtual void clear() noexcept = 0;

protected:
    IEntityRegistry() = default;
};

}  // namespace mycad::domain
