#include <mycad/application/CommandBus.hpp>

namespace mycad::application {

CommandBus::CommandBus(std::shared_ptr<domain::IEventStore> store) : store_{std::move(store)} {}

CommandBus::~CommandBus() = default;

std::shared_ptr<domain::IEventStore> CommandBus::eventStore() const noexcept {
    return store_;
}

}  // namespace mycad::application
