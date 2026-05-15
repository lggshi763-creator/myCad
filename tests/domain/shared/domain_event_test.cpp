#include <catch2/catch_test_macros.hpp>
#include <mycad/domain/DomainEvent.hpp>
#include <mycad/domain/EventTypeRegistry.hpp>

#include <memory>

namespace mycad::domain::tests {

// ---------------------------------------------------------------------------
// Concrete event stubs used throughout this file
// ---------------------------------------------------------------------------

class OrderPlaced : public DomainEvent {
    MYCAD_DOMAIN_EVENT("mycad.test.OrderPlaced")
public:
    explicit OrderPlaced(AggregateId id, Version ver, std::uint64_t ts)
        : DomainEvent(id, ver, ts) {}
};

class OrderCancelled : public DomainEvent {
    MYCAD_DOMAIN_EVENT("mycad.test.OrderCancelled")
public:
    explicit OrderCancelled(AggregateId id, Version ver, std::uint64_t ts)
        : DomainEvent(id, ver, ts) {}
};

// Helper: build an event with known fields for inspection.
static OrderPlaced makePlaced(std::uint64_t ts = 1000) {
    AggregateId aid;
    aid.bytes[0] = 0x11;
    return OrderPlaced{aid, Version{3}, ts};
}

// ---------------------------------------------------------------------------
// DomainEvent - typeName
// ---------------------------------------------------------------------------

TEST_CASE("DomainEvent -typeName returns declared string", "[domain][shared][events]") {
    const auto ev = makePlaced();
    REQUIRE(ev.typeName() == "mycad.test.OrderPlaced");
}

// ---------------------------------------------------------------------------
// DomainEvent - accessors
// ---------------------------------------------------------------------------

TEST_CASE("DomainEvent -aggregateVersion matches construction arg", "[domain][shared][events]") {
    const auto ev = makePlaced();
    REQUIRE(ev.aggregateVersion().value == 3);
}

TEST_CASE("DomainEvent -occurredAtMs matches construction arg", "[domain][shared][events]") {
    const auto ev = makePlaced(42000);
    REQUIRE(ev.occurredAtMs() == 42000);
}

TEST_CASE("DomainEvent -aggregateId matches construction arg", "[domain][shared][events]") {
    AggregateId aid;
    aid.bytes[0] = 0x11;
    const OrderPlaced ev{aid, Version{}, 0};
    REQUIRE(ev.aggregateId().bytes[0] == 0x11);
}

TEST_CASE("DomainEvent -eventId is non-null object", "[domain][shared][events]") {
    // Phase 0: generate() returns zero ID, but the field must be present.
    const auto ev = makePlaced();
    REQUIRE(ev.eventId() == EventId{});
}

// ---------------------------------------------------------------------------
// DomainEvent - immutability (compile-time; verified by design, not runtime)
// ---------------------------------------------------------------------------

TEST_CASE("DomainEvent -two instances have independent fields", "[domain][shared][events]") {
    const OrderPlaced a{AggregateId{}, Version{1}, 111};
    const OrderPlaced b{AggregateId{}, Version{2}, 222};
    REQUIRE(a.aggregateVersion().value == 1);
    REQUIRE(b.aggregateVersion().value == 2);
    REQUIRE(a.occurredAtMs() == 111);
    REQUIRE(b.occurredAtMs() == 222);
}

// ---------------------------------------------------------------------------
// EventTypeRegistry - register and find
// ---------------------------------------------------------------------------

TEST_CASE("EventTypeRegistry -findFactory returns nullptr for unknown type",
          "[domain][shared][events]") {
    EventTypeRegistry reg;
    REQUIRE(reg.findFactory("mycad.test.Ghost") == nullptr);
}

TEST_CASE("EventTypeRegistry -registerType and findFactory roundtrip", "[domain][shared][events]") {
    EventTypeRegistry reg;
    bool called = false;
    reg.registerType("mycad.test.Foo", [&](AggregateId, Version, std::uint64_t) {
        called = true;
        return std::make_unique<OrderPlaced>(AggregateId{}, Version{}, 0);
    });
    const auto* fn = reg.findFactory("mycad.test.Foo");
    REQUIRE(fn != nullptr);
    auto ev = (*fn)(AggregateId{}, Version{}, 0);
    REQUIRE(called);
    REQUIRE(ev != nullptr);
}

TEST_CASE("EventTypeRegistry -re-registering same name overwrites factory",
          "[domain][shared][events]") {
    EventTypeRegistry reg;
    reg.registerType("mycad.test.T", [](AggregateId, Version, std::uint64_t) {
        return std::make_unique<OrderPlaced>(AggregateId{}, Version{}, 0);
    });
    reg.registerType("mycad.test.T", [](AggregateId, Version, std::uint64_t) {
        return std::make_unique<OrderCancelled>(AggregateId{}, Version{}, 0);
    });
    REQUIRE(reg.size() == 1);
    const auto* fn = reg.findFactory("mycad.test.T");
    REQUIRE(fn != nullptr);
    auto ev = (*fn)(AggregateId{}, Version{}, 0);
    // The second registration should have won — result is OrderCancelled.
    REQUIRE(ev->typeName() == "mycad.test.OrderCancelled");
}

TEST_CASE("EventTypeRegistry -size reflects registered count", "[domain][shared][events]") {
    EventTypeRegistry reg;
    REQUIRE(reg.size() == 0);
    reg.registerType("mycad.test.A", [](AggregateId, Version, std::uint64_t) { return nullptr; });
    reg.registerType("mycad.test.B", [](AggregateId, Version, std::uint64_t) { return nullptr; });
    REQUIRE(reg.size() == 2);
}

TEST_CASE("EventTypeRegistry -clear removes all entries", "[domain][shared][events]") {
    EventTypeRegistry reg;
    reg.registerType("mycad.test.X", [](AggregateId, Version, std::uint64_t) { return nullptr; });
    reg.clear();
    REQUIRE(reg.size() == 0);
    REQUIRE(reg.findFactory("mycad.test.X") == nullptr);
}

// ---------------------------------------------------------------------------
// EventTypeRegistry - factory produces correct concrete type
// ---------------------------------------------------------------------------

TEST_CASE("EventTypeRegistry -factory produces event with correct typeName",
          "[domain][shared][events]") {
    EventTypeRegistry reg;
    reg.registerType("mycad.test.OrderPlaced", [](AggregateId id, Version ver, std::uint64_t ts) {
        return std::make_unique<OrderPlaced>(id, ver, ts);
    });

    const auto* fn = reg.findFactory("mycad.test.OrderPlaced");
    REQUIRE(fn != nullptr);
    const auto ev = (*fn)(AggregateId{}, Version{5}, 9999);
    REQUIRE(ev->typeName() == "mycad.test.OrderPlaced");
    REQUIRE(ev->aggregateVersion().value == 5);
    REQUIRE(ev->occurredAtMs() == 9999);
}

}  // namespace mycad::domain::tests
