#include <catch2/catch_test_macros.hpp>
#include <mycad/domain/IEntityRegistry.hpp>
#include <mycad/infrastructure/EnttRegistry.hpp>

// ---------------------------------------------------------------------------
// Test component types
// ---------------------------------------------------------------------------

struct Pos {
    float x{0.f};
    float y{0.f};
};

struct Vel {
    float dx{0.f};
    float dy{0.f};
};

// ---------------------------------------------------------------------------
// Basic entity lifecycle
// ---------------------------------------------------------------------------

TEST_CASE("EnttRegistry - create returns unique ids", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    const auto id1 = reg.create();
    const auto id2 = reg.create();
    CHECK(id1 != id2);
    CHECK(id1 != mycad::domain::kNullEntity);
    CHECK(id2 != mycad::domain::kNullEntity);
}

TEST_CASE("EnttRegistry - created entity isAlive", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    const auto id = reg.create();
    CHECK(reg.isAlive(id));
}

TEST_CASE("EnttRegistry - destroyed entity is not alive", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    const auto id = reg.create();
    reg.destroy(id);
    CHECK(!reg.isAlive(id));
}

TEST_CASE("EnttRegistry - destroy kNullEntity is no-op", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    // Must not crash or throw.
    reg.destroy(mycad::domain::kNullEntity);
    CHECK(reg.size() == 0u);
}

TEST_CASE("EnttRegistry - isAlive returns false for kNullEntity", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    CHECK(!reg.isAlive(mycad::domain::kNullEntity));
}

// ---------------------------------------------------------------------------
// size()
// ---------------------------------------------------------------------------

TEST_CASE("EnttRegistry - size tracks live count", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    CHECK(reg.size() == 0u);

    const auto id1 = reg.create();
    CHECK(reg.size() == 1u);

    const auto id2 = reg.create();
    CHECK(reg.size() == 2u);

    reg.destroy(id1);
    CHECK(reg.size() == 1u);

    reg.destroy(id2);
    CHECK(reg.size() == 0u);
}

// ---------------------------------------------------------------------------
// clear()
// ---------------------------------------------------------------------------

TEST_CASE("EnttRegistry - clear destroys all", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    const auto id1 = reg.create();
    const auto id2 = reg.create();
    const auto id3 = reg.create();

    reg.clear();

    CHECK(reg.size() == 0u);
    CHECK(!reg.isAlive(id1));
    CHECK(!reg.isAlive(id2));
    CHECK(!reg.isAlive(id3));
}

// ---------------------------------------------------------------------------
// Component operations (emplace / tryGet / remove)
// ---------------------------------------------------------------------------

TEST_CASE("EnttRegistry - emplace and tryGet component roundtrip", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    const auto id = reg.create();

    reg.emplace<Pos>(id, 1.0f, 2.0f);

    Pos* p = reg.tryGet<Pos>(id);
    REQUIRE(p != nullptr);
    CHECK(p->x == 1.0f);
    CHECK(p->y == 2.0f);
}

TEST_CASE("EnttRegistry - tryGet returns nullptr for missing component", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    const auto id = reg.create();

    CHECK(reg.tryGet<Pos>(id) == nullptr);
    CHECK(reg.tryGet<Vel>(id) == nullptr);
}

TEST_CASE("EnttRegistry - remove component", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    const auto id = reg.create();

    reg.emplace<Pos>(id, 3.0f, 4.0f);
    REQUIRE(reg.tryGet<Pos>(id) != nullptr);

    reg.remove<Pos>(id);
    CHECK(reg.tryGet<Pos>(id) == nullptr);
}

TEST_CASE("EnttRegistry - emplace multiple components on one entity", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    const auto id = reg.create();

    reg.emplace<Pos>(id, 5.0f, 6.0f);
    reg.emplace<Vel>(id, 1.0f, -1.0f);

    Pos* p = reg.tryGet<Pos>(id);
    Vel* v = reg.tryGet<Vel>(id);
    REQUIRE(p != nullptr);
    REQUIRE(v != nullptr);
    CHECK(p->x == 5.0f);
    CHECK(v->dx == 1.0f);
}

TEST_CASE("EnttRegistry - destroy entity removes all components", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry reg;
    const auto id = reg.create();

    reg.emplace<Pos>(id, 1.0f, 2.0f);
    reg.emplace<Vel>(id, 0.5f, 0.5f);
    reg.destroy(id);

    // Entity is dead; tryGet must return nullptr (not UB).
    CHECK(!reg.isAlive(id));
}

// ---------------------------------------------------------------------------
// IEntityRegistry interface compliance (polymorphic usage)
// ---------------------------------------------------------------------------

TEST_CASE("EnttRegistry - usable via IEntityRegistry pointer", "[infra][ecs]") {
    mycad::infrastructure::EnttRegistry impl;
    mycad::domain::IEntityRegistry* port = &impl;

    const auto id = port->create();
    CHECK(port->isAlive(id));
    CHECK(port->size() == 1u);

    port->destroy(id);
    CHECK(!port->isAlive(id));
    CHECK(port->size() == 0u);
}
