#include <catch2/catch_test_macros.hpp>
#include <mycad/domain/AggregateId.hpp>
#include <mycad/domain/EventId.hpp>
#include <mycad/domain/SketchId.hpp>
#include <mycad/domain/Version.hpp>

#include <unordered_map>

namespace mycad::domain::tests {

// ---------------------------------------------------------------------------
// EventId
// ---------------------------------------------------------------------------

TEST_CASE("EventId -default construction yields all-zero bytes", "[domain][shared][identity]") {
    const EventId id;
    for (auto b : id.bytes) {
        REQUIRE(b == 0);
    }
}

TEST_CASE("EventId -generate returns valid object", "[domain][shared][identity]") {
    const EventId id = EventId::generate();
    REQUIRE(id == EventId{});  // Phase 0: generate() returns zero ID
}

TEST_CASE("EventId -equality and inequality", "[domain][shared][identity]") {
    EventId a;
    EventId b;
    REQUIRE(a == b);
    b.bytes[0] = 1;
    REQUIRE(a != b);
}

TEST_CASE("EventId -ordering", "[domain][shared][identity]") {
    EventId lo;
    EventId hi;
    hi.bytes[0] = 1;
    REQUIRE(lo < hi);
    REQUIRE(hi > lo);
}

TEST_CASE("EventId -to_string format is 8-4-4-4-12 hex", "[domain][shared][identity]") {
    REQUIRE(to_string(EventId{}) == "00000000-0000-0000-0000-000000000000");
}

TEST_CASE("EventId -to_string reflects byte values", "[domain][shared][identity]") {
    EventId id;
    id.bytes[0] = 0xAB;
    id.bytes[15] = 0xCD;
    const std::string s = to_string(id);
    REQUIRE(s.substr(0, 2) == "ab");
    REQUIRE(s.substr(34, 2) == "cd");
}

TEST_CASE("EventId -usable as unordered_map key", "[domain][shared][identity]") {
    std::unordered_map<EventId, int> m;
    const EventId id = EventId::generate();
    m[id] = 42;
    REQUIRE(m.at(id) == 42);
}

// ---------------------------------------------------------------------------
// AggregateId
// ---------------------------------------------------------------------------

TEST_CASE("AggregateId -default construction yields all-zero bytes", "[domain][shared][identity]") {
    const AggregateId id;
    for (auto b : id.bytes) {
        REQUIRE(b == 0);
    }
}

TEST_CASE("AggregateId -to_string format is 8-4-4-4-12 hex", "[domain][shared][identity]") {
    REQUIRE(to_string(AggregateId{}) == "00000000-0000-0000-0000-000000000000");
}

TEST_CASE("AggregateId -usable as unordered_map key", "[domain][shared][identity]") {
    std::unordered_map<AggregateId, int> m;
    const AggregateId id = AggregateId::generate();
    m[id] = 7;
    REQUIRE(m.at(id) == 7);
}

// ---------------------------------------------------------------------------
// SketchId
// ---------------------------------------------------------------------------

TEST_CASE("SketchId -default construction yields all-zero bytes", "[domain][shared][identity]") {
    const SketchId id;
    for (auto b : id.bytes) {
        REQUIRE(b == 0);
    }
}

TEST_CASE("SketchId -to_string format is 8-4-4-4-12 hex", "[domain][shared][identity]") {
    REQUIRE(to_string(SketchId{}) == "00000000-0000-0000-0000-000000000000");
}

// ---------------------------------------------------------------------------
// Version
// ---------------------------------------------------------------------------

TEST_CASE("Version -default construction is v0", "[domain][shared][identity]") {
    REQUIRE(Version{}.value == 0);
}

TEST_CASE("Version -next increments by one", "[domain][shared][identity]") {
    const Version v0;
    const Version v1 = v0.next();
    const Version v2 = v1.next();
    REQUIRE(v1.value == 1);
    REQUIRE(v2.value == 2);
}

TEST_CASE("Version -original unchanged after next", "[domain][shared][identity]") {
    const Version v0;
    v0.next();
    REQUIRE(v0.value == 0);
}

TEST_CASE("Version -ordering", "[domain][shared][identity]") {
    REQUIRE(Version{0} < Version{1});
    REQUIRE(Version{1} > Version{0});
    REQUIRE(Version{1} == Version{1});
}

TEST_CASE("Version -to_string format", "[domain][shared][identity]") {
    REQUIRE(to_string(Version{0}) == "v0");
    REQUIRE(to_string(Version{42}) == "v42");
}

TEST_CASE("Version -usable as unordered_map key", "[domain][shared][identity]") {
    std::unordered_map<Version, int> m;
    m[Version{1}] = 100;
    REQUIRE(m.at(Version{1}) == 100);
}

}  // namespace mycad::domain::tests
