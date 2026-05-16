#include <mycad/domain/EventTypeRegistry.hpp>
#include <mycad/infrastructure/InMemoryEventStore.hpp>

#include <mutex>
#include <stdexcept>
#include <unordered_map>

namespace mycad::infrastructure {

// ---------------------------------------------------------------------------
// Internal storage types
// ---------------------------------------------------------------------------

namespace {

/// @brief Wire format used to persist a single event in memory.
///
/// Sprint 0.3 stores only the scalar fields needed for reconstruction.
/// payload is reserved for Sprint 0.5 (FlatBuffers).
struct SerializedEvent {
    std::string typeName;
    domain::AggregateId aggregateId;
    domain::Version aggregateVersion;
    std::uint64_t occurredAtMs{0};
    std::string payload;  ///< JSON placeholder; empty in Sprint 0.3.
};

/// @brief One aggregate's event stream.
struct Stream {
    std::vector<SerializedEvent> events;
    domain::Version latestVersion;  ///< Version{0} when empty.
};

}  // namespace

// ---------------------------------------------------------------------------
// PIMPL implementation record
// ---------------------------------------------------------------------------

struct InMemoryEventStore::Impl {
    std::unordered_map<domain::AggregateId, Stream> streams;
    mutable std::mutex mu;
};

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

InMemoryEventStore::InMemoryEventStore() : impl_(std::make_unique<Impl>()) {}

InMemoryEventStore::~InMemoryEventStore() = default;

// ---------------------------------------------------------------------------
// append
// ---------------------------------------------------------------------------

void InMemoryEventStore::append(domain::AggregateId aggregateId,
                                domain::Version expectedVersion,
                                std::span<const domain::DomainEvent* const> events) {
    if (events.empty())
        return;

    std::lock_guard lock(impl_->mu);

    Stream& stream = impl_->streams[aggregateId];  // default-inserts Version{0} if absent

    if (stream.latestVersion != expectedVersion) {
        throw domain::ConcurrencyError(aggregateId, expectedVersion, stream.latestVersion);
    }

    for (const domain::DomainEvent* ev : events) {
        stream.events.push_back(SerializedEvent{
            ev->typeName(), ev->aggregateId(), ev->aggregateVersion(), ev->occurredAtMs(), {}});
        stream.latestVersion = ev->aggregateVersion();
    }
}

// ---------------------------------------------------------------------------
// Internal helper: reconstruct events from a range
// ---------------------------------------------------------------------------

namespace {

std::vector<std::unique_ptr<domain::DomainEvent>>
reconstructRange(const std::vector<SerializedEvent>& serialized, domain::Version fromVersion) {
    domain::EventTypeRegistry& reg = domain::EventTypeRegistry::instance();
    std::vector<std::unique_ptr<domain::DomainEvent>> result;

    for (const SerializedEvent& se : serialized) {
        if (se.aggregateVersion < fromVersion)
            continue;

        const domain::EventTypeRegistry::FactoryFn* factory = reg.findFactory(se.typeName);
        if (!factory) {
            throw std::runtime_error("InMemoryEventStore::load: no factory registered for '" +
                                     se.typeName + "'");
        }
        result.push_back((*factory)(se.aggregateId, se.aggregateVersion, se.occurredAtMs));
    }
    return result;
}

}  // namespace

// ---------------------------------------------------------------------------
// load
// ---------------------------------------------------------------------------

std::vector<std::unique_ptr<domain::DomainEvent>>
InMemoryEventStore::load(domain::AggregateId aggregateId) {
    std::lock_guard lock(impl_->mu);
    const auto it = impl_->streams.find(aggregateId);
    if (it == impl_->streams.end())
        return {};
    return reconstructRange(it->second.events, domain::Version{0});
}

// ---------------------------------------------------------------------------
// loadSince
// ---------------------------------------------------------------------------

std::vector<std::unique_ptr<domain::DomainEvent>>
InMemoryEventStore::loadSince(domain::AggregateId aggregateId, domain::Version fromVersion) {
    std::lock_guard lock(impl_->mu);
    const auto it = impl_->streams.find(aggregateId);
    if (it == impl_->streams.end())
        return {};
    return reconstructRange(it->second.events, fromVersion);
}

// ---------------------------------------------------------------------------
// latestVersion
// ---------------------------------------------------------------------------

domain::Version InMemoryEventStore::latestVersion(domain::AggregateId aggregateId) {
    std::lock_guard lock(impl_->mu);
    const auto it = impl_->streams.find(aggregateId);
    if (it == impl_->streams.end())
        return domain::Version{0};
    return it->second.latestVersion;
}

}  // namespace mycad::infrastructure
