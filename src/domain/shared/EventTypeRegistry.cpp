#include <mycad/domain/EventTypeRegistry.hpp>

namespace mycad::domain {

EventTypeRegistry& EventTypeRegistry::instance() noexcept {
    static EventTypeRegistry inst;
    return inst;
}

void EventTypeRegistry::registerType(std::string typeName, FactoryFn factory) {
    registry_[std::move(typeName)] = std::move(factory);
}

const EventTypeRegistry::FactoryFn*
EventTypeRegistry::findFactory(const std::string& typeName) const {
    const auto it = registry_.find(typeName);
    return it != registry_.end() ? &it->second : nullptr;
}

}  // namespace mycad::domain
