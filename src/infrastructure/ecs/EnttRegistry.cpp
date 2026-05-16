#include <mycad/infrastructure/EnttRegistry.hpp>

namespace mycad::infrastructure {

EnttRegistry::EnttRegistry() = default;
EnttRegistry::~EnttRegistry() = default;

domain::EntityId EnttRegistry::create() {
    ++aliveCount_;
    return static_cast<domain::EntityId>(registry_.create());
}

void EnttRegistry::destroy(domain::EntityId id) noexcept {
    if (id == domain::kNullEntity)
        return;
    const auto entity = static_cast<entt::entity>(id);
    if (registry_.valid(entity)) {
        registry_.destroy(entity);
        --aliveCount_;
    }
}

bool EnttRegistry::isAlive(domain::EntityId id) const noexcept {
    if (id == domain::kNullEntity)
        return false;
    return registry_.valid(static_cast<entt::entity>(id));
}

std::size_t EnttRegistry::size() const noexcept {
    return aliveCount_;
}

void EnttRegistry::clear() noexcept {
    registry_.clear();
    aliveCount_ = 0;
}

}  // namespace mycad::infrastructure
