#include <catch2/catch_test_macros.hpp>
#include <mycad/domain/AggregateId.hpp>
#include <mycad/domain/DomainEvent.hpp>
#include <mycad/domain/EventTypeRegistry.hpp>
#include <mycad/domain/Version.hpp>
#include <mycad/infrastructure/SqliteEventStore.hpp>

#include <cstdio>
#include <memory>

// ---------------------------------------------------------------------------
// Test-local event types
// ---------------------------------------------------------------------------

class ThingRecorded final : public mycad::domain::DomainEvent {
    MYCAD_DOMAIN_EVENT("test.sqlite.ThingRecorded")
public:
    explicit ThingRecorded(mycad::domain::AggregateId aid,
                           mycad::domain::Version ver,
                           std::uint64_t ts)
        : DomainEvent(aid, ver, ts) {}
};

class AnotherThingRecorded final : public mycad::domain::DomainEvent {
    MYCAD_DOMAIN_EVENT("test.sqlite.AnotherThingRecorded")
public:
    explicit AnotherThingRecorded(mycad::domain::AggregateId aid,
                                  mycad::domain::Version ver,
                                  std::uint64_t ts)
        : DomainEvent(aid, ver, ts) {}
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

mycad::domain::AggregateId freshId() {
    static std::uint8_t counter = 0;
    mycad::domain::AggregateId id{};
    id.bytes[0] = ++counter;
    // Shift into a different byte range to avoid collisions with other test files.
    id.bytes[1] = 0xDB;
    return id;
}

void registerTestEvents() {
    auto& reg = mycad::domain::EventTypeRegistry::instance();
    reg.registerType("test.sqlite.ThingRecorded",
                     [](mycad::domain::AggregateId id,
                        mycad::domain::Version ver,
                        std::uint64_t ts) -> std::unique_ptr<mycad::domain::DomainEvent> {
                         return std::make_unique<ThingRecorded>(id, ver, ts);
                     });
    reg.registerType("test.sqlite.AnotherThingRecorded",
                     [](mycad::domain::AggregateId id,
                        mycad::domain::Version ver,
                        std::uint64_t ts) -> std::unique_ptr<mycad::domain::DomainEvent> {
                         return std::make_unique<AnotherThingRecorded>(id, ver, ts);
                     });
}

/// @brief RAII wrapper that deletes the temp DB file on destruction.
struct TempDb {
    std::string path;
    explicit TempDb(std::string p) : path(std::move(p)) {}
    ~TempDb() {
        std::remove(path.c_str());
    }
};

}  // namespace

// ---------------------------------------------------------------------------
// latestVersion — empty store
// ---------------------------------------------------------------------------

TEST_CASE("SqliteEventStore - empty store returns v0", "[infra][sqlite]") {
    registerTestEvents();
    TempDb tmp{"test_sqlite_v0.db"};
    mycad::infrastructure::SqliteEventStore store{tmp.path};

    CHECK(store.latestVersion(freshId()) == mycad::domain::Version{0});
}

// ---------------------------------------------------------------------------
// append + latestVersion
// ---------------------------------------------------------------------------

TEST_CASE("SqliteEventStore - append single event advances version", "[infra][sqlite]") {
    registerTestEvents();
    TempDb tmp{"test_sqlite_single.db"};
    mycad::infrastructure::SqliteEventStore store{tmp.path};
    const auto aid = freshId();

    ThingRecorded ev{aid, mycad::domain::Version{1}, 1000u};
    store.appendOne(aid, mycad::domain::Version{0}, ev);

    CHECK(store.latestVersion(aid) == mycad::domain::Version{1});
}

TEST_CASE("SqliteEventStore - append batch of 3 events", "[infra][sqlite]") {
    registerTestEvents();
    TempDb tmp{"test_sqlite_batch.db"};
    mycad::infrastructure::SqliteEventStore store{tmp.path};
    const auto aid = freshId();

    ThingRecorded e1{aid, mycad::domain::Version{1}, 100u};
    ThingRecorded e2{aid, mycad::domain::Version{2}, 200u};
    ThingRecorded e3{aid, mycad::domain::Version{3}, 300u};

    const mycad::domain::DomainEvent* ptrs[3] = {&e1, &e2, &e3};
    store.append(aid,
                 mycad::domain::Version{0},
                 std::span<const mycad::domain::DomainEvent* const>{ptrs, 3});

    CHECK(store.latestVersion(aid) == mycad::domain::Version{3});
}

// ---------------------------------------------------------------------------
// Optimistic concurrency
// ---------------------------------------------------------------------------

TEST_CASE("SqliteEventStore - ConcurrencyError on stale expectedVersion", "[infra][sqlite]") {
    registerTestEvents();
    TempDb tmp{"test_sqlite_occ.db"};
    mycad::infrastructure::SqliteEventStore store{tmp.path};
    const auto aid = freshId();

    ThingRecorded e1{aid, mycad::domain::Version{1}, 10u};
    store.appendOne(aid, mycad::domain::Version{0}, e1);

    // Second append with wrong expectedVersion.
    ThingRecorded e2{aid, mycad::domain::Version{2}, 20u};
    REQUIRE_THROWS_AS(store.appendOne(aid, mycad::domain::Version{0}, e2),
                      mycad::domain::ConcurrencyError);
}

// ---------------------------------------------------------------------------
// load — order and reconstruction
// ---------------------------------------------------------------------------

TEST_CASE("SqliteEventStore - load returns events in version order", "[infra][sqlite]") {
    registerTestEvents();
    TempDb tmp{"test_sqlite_load.db"};
    mycad::infrastructure::SqliteEventStore store{tmp.path};
    const auto aid = freshId();

    ThingRecorded e1{aid, mycad::domain::Version{1}, 100u};
    ThingRecorded e2{aid, mycad::domain::Version{2}, 200u};
    ThingRecorded e3{aid, mycad::domain::Version{3}, 300u};

    const mycad::domain::DomainEvent* ptrs[3] = {&e1, &e2, &e3};
    store.append(aid,
                 mycad::domain::Version{0},
                 std::span<const mycad::domain::DomainEvent* const>{ptrs, 3});

    const auto loaded = store.load(aid);
    REQUIRE(loaded.size() == 3u);
    CHECK(loaded[0]->aggregateVersion() == mycad::domain::Version{1});
    CHECK(loaded[1]->aggregateVersion() == mycad::domain::Version{2});
    CHECK(loaded[2]->aggregateVersion() == mycad::domain::Version{3});
    CHECK(loaded[0]->typeName() == "test.sqlite.ThingRecorded");
}

TEST_CASE("SqliteEventStore - load empty aggregate returns empty vector", "[infra][sqlite]") {
    registerTestEvents();
    TempDb tmp{"test_sqlite_empty.db"};
    mycad::infrastructure::SqliteEventStore store{tmp.path};

    CHECK(store.load(freshId()).empty());
}

// ---------------------------------------------------------------------------
// loadSince
// ---------------------------------------------------------------------------

TEST_CASE("SqliteEventStore - loadSince filters correctly", "[infra][sqlite]") {
    registerTestEvents();
    TempDb tmp{"test_sqlite_since.db"};
    mycad::infrastructure::SqliteEventStore store{tmp.path};
    const auto aid = freshId();

    ThingRecorded e1{aid, mycad::domain::Version{1}, 10u};
    ThingRecorded e2{aid, mycad::domain::Version{2}, 20u};
    ThingRecorded e3{aid, mycad::domain::Version{3}, 30u};

    const mycad::domain::DomainEvent* ptrs[3] = {&e1, &e2, &e3};
    store.append(aid,
                 mycad::domain::Version{0},
                 std::span<const mycad::domain::DomainEvent* const>{ptrs, 3});

    const auto since2 = store.loadSince(aid, mycad::domain::Version{2});
    REQUIRE(since2.size() == 2u);
    CHECK(since2[0]->aggregateVersion() == mycad::domain::Version{2});
    CHECK(since2[1]->aggregateVersion() == mycad::domain::Version{3});
}

// ---------------------------------------------------------------------------
// Aggregate isolation
// ---------------------------------------------------------------------------

TEST_CASE("SqliteEventStore - two aggregates are isolated", "[infra][sqlite]") {
    registerTestEvents();
    TempDb tmp{"test_sqlite_iso.db"};
    mycad::infrastructure::SqliteEventStore store{tmp.path};
    const auto aid1 = freshId();
    const auto aid2 = freshId();

    ThingRecorded e1{aid1, mycad::domain::Version{1}, 1u};
    store.appendOne(aid1, mycad::domain::Version{0}, e1);

    CHECK(store.latestVersion(aid1) == mycad::domain::Version{1});
    CHECK(store.latestVersion(aid2) == mycad::domain::Version{0});
}

// ---------------------------------------------------------------------------
// Reopen — persistence across process restart (key test for SQLite)
// ---------------------------------------------------------------------------

TEST_CASE("SqliteEventStore - events survive store close and reopen", "[infra][sqlite]") {
    registerTestEvents();
    TempDb tmp{"test_sqlite_reopen.db"};
    const auto aid = freshId();

    // --- Write ---
    {
        mycad::infrastructure::SqliteEventStore store{tmp.path};
        ThingRecorded e1{aid, mycad::domain::Version{1}, 111u};
        ThingRecorded e2{aid, mycad::domain::Version{2}, 222u};
        store.appendOne(aid, mycad::domain::Version{0}, e1);
        store.appendOne(aid, mycad::domain::Version{1}, e2);
    }  // store destroyed here

    // --- Reopen ---
    {
        mycad::infrastructure::SqliteEventStore store{tmp.path};

        CHECK(store.latestVersion(aid) == mycad::domain::Version{2});

        const auto loaded = store.load(aid);
        REQUIRE(loaded.size() == 2u);
        CHECK(loaded[0]->typeName() == "test.sqlite.ThingRecorded");
        CHECK(loaded[0]->occurredAtMs() == 111u);
        CHECK(loaded[1]->occurredAtMs() == 222u);
    }
}

// ---------------------------------------------------------------------------
// Payload serialiser + loadRows
// ---------------------------------------------------------------------------

TEST_CASE("SqliteEventStore - registerSerializer stores custom payload", "[infra][sqlite]") {
    registerTestEvents();
    TempDb tmp{"test_sqlite_payload.db"};
    const auto aid = freshId();

    {
        mycad::infrastructure::SqliteEventStore store{tmp.path};
        store.registerSerializer(
            "test.sqlite.ThingRecorded",
            [](const mycad::domain::DomainEvent&) { return R"({"value":42})"; });

        ThingRecorded ev{aid, mycad::domain::Version{1}, 0u};
        store.appendOne(aid, mycad::domain::Version{0}, ev);
    }

    {
        mycad::infrastructure::SqliteEventStore store{tmp.path};
        const auto rows = store.loadRows(aid);
        REQUIRE(rows.size() == 1u);
        CHECK(rows[0].typeName == "test.sqlite.ThingRecorded");
        CHECK(rows[0].payload == R"({"value":42})");
        CHECK(rows[0].version == mycad::domain::Version{1});
    }
}

TEST_CASE("SqliteEventStore - default payload is empty JSON object", "[infra][sqlite]") {
    registerTestEvents();
    TempDb tmp{"test_sqlite_defpayload.db"};
    const auto aid = freshId();

    mycad::infrastructure::SqliteEventStore store{tmp.path};
    ThingRecorded ev{aid, mycad::domain::Version{1}, 0u};
    store.appendOne(aid, mycad::domain::Version{0}, ev);

    const auto rows = store.loadRows(aid);
    REQUIRE(rows.size() == 1u);
    CHECK(rows[0].payload == "{}");
}
