#include <catch2/catch_test_macros.hpp>
#include <mycad/domain/AggregateId.hpp>
#include <mycad/domain/DomainEvent.hpp>
#include <mycad/domain/EventTypeRegistry.hpp>
#include <mycad/domain/Version.hpp>
#include <mycad/infrastructure/InMemoryEventStore.hpp>

#include <memory>

// ---------------------------------------------------------------------------
// Test fixture event types
// ---------------------------------------------------------------------------

/// Minimal domain event used exclusively in this test file.
class ThingHappened final : public mycad::domain::DomainEvent {
    MYCAD_DOMAIN_EVENT("test.ThingHappened")
public:
    explicit ThingHappened(mycad::domain::AggregateId aid,
                           mycad::domain::Version ver,
                           std::uint64_t ts)
        : DomainEvent(aid, ver, ts) {}
};

class OtherThingHappened final : public mycad::domain::DomainEvent {
    MYCAD_DOMAIN_EVENT("test.OtherThingHappened")
public:
    explicit OtherThingHappened(mycad::domain::AggregateId aid,
                                mycad::domain::Version ver,
                                std::uint64_t ts)
        : DomainEvent(aid, ver, ts) {}
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Returns a unique AggregateId per call (byte[0] increments as a counter).
/// AggregateId::generate() is a stub that always returns zeros, so we mint
/// distinct IDs manually here.
mycad::domain::AggregateId freshId() {
    static std::uint8_t counter = 0;
    mycad::domain::AggregateId id{};
    id.bytes[0] = ++counter;
    return id;
}

/// Registers both test event types in the given registry.
void registerTestEvents(mycad::domain::EventTypeRegistry& reg) {
    reg.registerType("test.ThingHappened",
                     [](mycad::domain::AggregateId id,
                        mycad::domain::Version ver,
                        std::uint64_t ts) -> std::unique_ptr<mycad::domain::DomainEvent> {
                         return std::make_unique<ThingHappened>(id, ver, ts);
                     });
    reg.registerType("test.OtherThingHappened",
                     [](mycad::domain::AggregateId id,
                        mycad::domain::Version ver,
                        std::uint64_t ts) -> std::unique_ptr<mycad::domain::DomainEvent> {
                         return std::make_unique<OtherThingHappened>(id, ver, ts);
                     });
}

}  // namespace

// ---------------------------------------------------------------------------
// latestVersion / empty store
// ---------------------------------------------------------------------------

TEST_CASE("InMemoryEventStore - empty store returns v0", "[infra][eventsource]") {
    mycad::infrastructure::InMemoryEventStore store;
    CHECK(store.latestVersion(freshId()) == mycad::domain::Version{0});
}

// ---------------------------------------------------------------------------
// append + latestVersion
// ---------------------------------------------------------------------------

TEST_CASE("InMemoryEventStore - append single event advances version", "[infra][eventsource]") {
    mycad::infrastructure::InMemoryEventStore store;
    auto aid = freshId();

    ThingHappened ev(aid, mycad::domain::Version{1}, 1000u);
    store.appendOne(aid, mycad::domain::Version{0}, ev);

    CHECK(store.latestVersion(aid) == mycad::domain::Version{1});
}

TEST_CASE("InMemoryEventStore - append batch of 3 events", "[infra][eventsource]") {
    mycad::infrastructure::InMemoryEventStore store;
    auto aid = freshId();

    ThingHappened e1(aid, mycad::domain::Version{1}, 1000u);
    ThingHappened e2(aid, mycad::domain::Version{2}, 2000u);
    ThingHappened e3(aid, mycad::domain::Version{3}, 3000u);

    const mycad::domain::DomainEvent* ptrs[3] = {&e1, &e2, &e3};
    store.append(aid,
                 mycad::domain::Version{0},
                 std::span<const mycad::domain::DomainEvent* const>{ptrs, 3});

    CHECK(store.latestVersion(aid) == mycad::domain::Version{3});
}

// ---------------------------------------------------------------------------
// Optimistic concurrency
// ---------------------------------------------------------------------------

TEST_CASE("InMemoryEventStore - ConcurrencyError on stale version", "[infra][eventsource]") {
    mycad::infrastructure::InMemoryEventStore store;
    auto aid = freshId();

    ThingHappened e1(aid, mycad::domain::Version{1}, 1000u);
    store.appendOne(aid, mycad::domain::Version{0}, e1);

    // Attempt a second append with the wrong expected version (still 0).
    ThingHappened e2(aid, mycad::domain::Version{2}, 2000u);
    REQUIRE_THROWS_AS(store.appendOne(aid, mycad::domain::Version{0}, e2),
                      mycad::domain::ConcurrencyError);
}

// ---------------------------------------------------------------------------
// load
// ---------------------------------------------------------------------------

TEST_CASE("InMemoryEventStore - load returns events in version order", "[infra][eventsource]") {
    mycad::domain::EventTypeRegistry reg;
    registerTestEvents(reg);

    // Temporarily override the singleton for this test.
    // We use a local registry; load() calls EventTypeRegistry::instance().
    // Register in the singleton so load() can find the factories.
    registerTestEvents(mycad::domain::EventTypeRegistry::instance());

    mycad::infrastructure::InMemoryEventStore store;
    auto aid = freshId();

    ThingHappened e1(aid, mycad::domain::Version{1}, 100u);
    ThingHappened e2(aid, mycad::domain::Version{2}, 200u);
    ThingHappened e3(aid, mycad::domain::Version{3}, 300u);

    const mycad::domain::DomainEvent* ptrs[3] = {&e1, &e2, &e3};
    store.append(aid,
                 mycad::domain::Version{0},
                 std::span<const mycad::domain::DomainEvent* const>{ptrs, 3});

    auto loaded = store.load(aid);
    REQUIRE(loaded.size() == 3u);
    CHECK(loaded[0]->aggregateVersion() == mycad::domain::Version{1});
    CHECK(loaded[1]->aggregateVersion() == mycad::domain::Version{2});
    CHECK(loaded[2]->aggregateVersion() == mycad::domain::Version{3});
}

TEST_CASE("InMemoryEventStore - load empty aggregate returns empty vector",
          "[infra][eventsource]") {
    mycad::infrastructure::InMemoryEventStore store;
    auto loaded = store.load(freshId());
    CHECK(loaded.empty());
}

// ---------------------------------------------------------------------------
// loadSince
// ---------------------------------------------------------------------------

TEST_CASE("InMemoryEventStore - loadSince filters by fromVersion", "[infra][eventsource]") {
    registerTestEvents(mycad::domain::EventTypeRegistry::instance());

    mycad::infrastructure::InMemoryEventStore store;
    auto aid = freshId();

    ThingHappened e1(aid, mycad::domain::Version{1}, 100u);
    ThingHappened e2(aid, mycad::domain::Version{2}, 200u);
    ThingHappened e3(aid, mycad::domain::Version{3}, 300u);

    const mycad::domain::DomainEvent* ptrs[3] = {&e1, &e2, &e3};
    store.append(aid,
                 mycad::domain::Version{0},
                 std::span<const mycad::domain::DomainEvent* const>{ptrs, 3});

    auto since2 = store.loadSince(aid, mycad::domain::Version{2});
    REQUIRE(since2.size() == 2u);
    CHECK(since2[0]->aggregateVersion() == mycad::domain::Version{2});
    CHECK(since2[1]->aggregateVersion() == mycad::domain::Version{3});
}

// ---------------------------------------------------------------------------
// Registry round-trip (V2 acceptance criterion)
// ---------------------------------------------------------------------------

TEST_CASE("InMemoryEventStore - load reconstructs event typeName via registry",
          "[infra][eventsource]") {
    registerTestEvents(mycad::domain::EventTypeRegistry::instance());

    mycad::infrastructure::InMemoryEventStore store;
    auto aid = freshId();

    ThingHappened ev(aid, mycad::domain::Version{1}, 42u);
    store.appendOne(aid, mycad::domain::Version{0}, ev);

    auto loaded = store.load(aid);
    REQUIRE(loaded.size() == 1u);
    CHECK(loaded[0]->typeName() == "test.ThingHappened");
    CHECK(loaded[0]->occurredAtMs() == 42u);
    CHECK(loaded[0]->aggregateVersion() == mycad::domain::Version{1});
}

// ---------------------------------------------------------------------------
// Aggregate isolation
// ---------------------------------------------------------------------------

TEST_CASE("InMemoryEventStore - two aggregates are isolated", "[infra][eventsource]") {
    mycad::infrastructure::InMemoryEventStore store;
    auto aid1 = freshId();
    auto aid2 = freshId();

    ThingHappened e1(aid1, mycad::domain::Version{1}, 1000u);
    store.appendOne(aid1, mycad::domain::Version{0}, e1);

    CHECK(store.latestVersion(aid1) == mycad::domain::Version{1});
    CHECK(store.latestVersion(aid2) == mycad::domain::Version{0});
}

// ---------------------------------------------------------------------------
// appendOne convenience overload
// ---------------------------------------------------------------------------

TEST_CASE("InMemoryEventStore - appendOne convenience overload", "[infra][eventsource]") {
    registerTestEvents(mycad::domain::EventTypeRegistry::instance());

    mycad::infrastructure::InMemoryEventStore store;
    auto aid = freshId();

    ThingHappened e1(aid, mycad::domain::Version{1}, 10u);
    OtherThingHappened e2(aid, mycad::domain::Version{2}, 20u);

    store.appendOne(aid, mycad::domain::Version{0}, e1);
    store.appendOne(aid, mycad::domain::Version{1}, e2);

    CHECK(store.latestVersion(aid) == mycad::domain::Version{2});

    auto loaded = store.load(aid);
    REQUIRE(loaded.size() == 2u);
    CHECK(loaded[0]->typeName() == "test.ThingHappened");
    CHECK(loaded[1]->typeName() == "test.OtherThingHappened");
}
