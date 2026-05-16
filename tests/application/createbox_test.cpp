#include <catch2/catch_test_macros.hpp>
#include <mycad/application/CommandBus.hpp>
#include <mycad/application/commands/CreateBoxCommand.hpp>
#include <mycad/application/handlers/CreateBoxCommandHandler.hpp>
#include <mycad/domain/IRenderPort.hpp>
#include <mycad/domain/Version.hpp>
#include <mycad/infrastructure/EnttRegistry.hpp>
#include <mycad/infrastructure/InMemoryEventStore.hpp>
#include <mycad/infrastructure/OcctGeometryAdapter.hpp>

#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// @brief Returns an AggregateId with a unique non-zero first byte.
mycad::domain::AggregateId freshId() {
    static std::uint8_t counter = 0;
    mycad::domain::AggregateId id{};
    id.bytes[0] = ++counter;
    return id;
}

/// @brief Minimal IRenderPort spy that records uploadMesh calls.
class MockRenderPort final : public mycad::domain::IRenderPort {
public:
    struct Call {
        mycad::domain::BRepHandle handle;
        std::size_t vertexCount{0};
        std::size_t indexCount{0};
    };

    void uploadMesh(mycad::domain::BRepHandle handle,
                    const mycad::domain::TriangleMesh& mesh) override {
        uploadCalls.push_back({handle, mesh.vertexCount(), mesh.indexCount()});
    }
    void removeMesh(mycad::domain::BRepHandle /*handle*/) noexcept override {}
    void setViewMatrix(const float* /*mat4*/) noexcept override {}
    void setProjectionMatrix(const float* /*mat4*/) noexcept override {}
    void render() override {}

    std::vector<Call> uploadCalls;
};

/// @brief Wires up a CommandBus with a CreateBoxCommandHandler.
struct Fixture {
    std::shared_ptr<mycad::infrastructure::InMemoryEventStore> store =
        std::make_shared<mycad::infrastructure::InMemoryEventStore>();

    std::shared_ptr<mycad::infrastructure::OcctGeometryAdapter> geom =
        std::make_shared<mycad::infrastructure::OcctGeometryAdapter>();

    std::shared_ptr<mycad::infrastructure::EnttRegistry> ecs =
        std::make_shared<mycad::infrastructure::EnttRegistry>();

    std::shared_ptr<mycad::application::handlers::CreateBoxCommandHandler> handler =
        std::make_shared<mycad::application::handlers::CreateBoxCommandHandler>(geom, ecs, store);

    mycad::application::CommandBus bus{store};

    Fixture() {
        bus.registerHandler<mycad::application::commands::CreateBoxCommand>(handler);
    }
};

}  // namespace

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("CreateBoxCommand - success stores event and creates entity",
          "[app][integration][createbox]") {
    Fixture f;
    const auto id = freshId();

    auto res = f.bus.send(mycad::application::commands::CreateBoxCommand{10.0, 10.0, 10.0, id});

    REQUIRE(res.has_value());
    CHECK(f.store->latestVersion(id) == mycad::domain::Version{1});
    CHECK(f.ecs->size() == 1u);
}

TEST_CASE("CreateBoxCommand - rectangular box stores version 1", "[app][integration][createbox]") {
    Fixture f;
    const auto id = freshId();

    auto res = f.bus.send(mycad::application::commands::CreateBoxCommand{2.0, 3.0, 4.0, id});

    REQUIRE(res.has_value());
    CHECK(f.store->latestVersion(id) == mycad::domain::Version{1});
}

TEST_CASE("CreateBoxCommand - second command on same id advances to version 2",
          "[app][integration][createbox]") {
    Fixture f;
    const auto id = freshId();

    REQUIRE(
        f.bus.send(mycad::application::commands::CreateBoxCommand{1.0, 1.0, 1.0, id}).has_value());
    REQUIRE(
        f.bus.send(mycad::application::commands::CreateBoxCommand{2.0, 2.0, 2.0, id}).has_value());

    CHECK(f.store->latestVersion(id) == mycad::domain::Version{2});
    CHECK(f.ecs->size() == 2u);
}

TEST_CASE("CreateBoxCommand - zero dx returns BusinessRuleViolated",
          "[app][integration][createbox]") {
    Fixture f;
    const auto id = freshId();

    auto res = f.bus.send(mycad::application::commands::CreateBoxCommand{0.0, 5.0, 5.0, id});

    REQUIRE(!res);
    CHECK(res.error().kind == mycad::application::CommandErrorKind::BusinessRuleViolated);
    CHECK(f.store->latestVersion(id) == mycad::domain::Version{0});
    CHECK(f.ecs->size() == 0u);
}

TEST_CASE("CreateBoxCommand - negative dimension returns BusinessRuleViolated",
          "[app][integration][createbox]") {
    Fixture f;
    const auto id = freshId();

    auto res = f.bus.send(mycad::application::commands::CreateBoxCommand{5.0, -1.0, 5.0, id});

    REQUIRE(!res);
    CHECK(res.error().kind == mycad::application::CommandErrorKind::BusinessRuleViolated);
}

TEST_CASE("CreateBoxCommand - two aggregates are isolated", "[app][integration][createbox]") {
    Fixture f;
    const auto id1 = freshId();
    const auto id2 = freshId();

    REQUIRE(
        f.bus.send(mycad::application::commands::CreateBoxCommand{1.0, 1.0, 1.0, id1}).has_value());
    REQUIRE(
        f.bus.send(mycad::application::commands::CreateBoxCommand{2.0, 2.0, 2.0, id1}).has_value());
    REQUIRE(
        f.bus.send(mycad::application::commands::CreateBoxCommand{3.0, 3.0, 3.0, id2}).has_value());

    CHECK(f.store->latestVersion(id1) == mycad::domain::Version{2});
    CHECK(f.store->latestVersion(id2) == mycad::domain::Version{1});
}

// ---------------------------------------------------------------------------
// T6 — tessellate + uploadMesh integration
// ---------------------------------------------------------------------------

TEST_CASE("CreateBoxCommand - renderer receives uploadMesh after success",
          "[app][integration][createbox][t6]") {
    auto store = std::make_shared<mycad::infrastructure::InMemoryEventStore>();
    auto geom = std::make_shared<mycad::infrastructure::OcctGeometryAdapter>();
    auto ecs = std::make_shared<mycad::infrastructure::EnttRegistry>();
    auto renderer = std::make_shared<MockRenderPort>();

    auto handler = std::make_shared<mycad::application::handlers::CreateBoxCommandHandler>(
        geom, ecs, store, renderer);

    mycad::application::CommandBus bus{store};
    bus.registerHandler<mycad::application::commands::CreateBoxCommand>(handler);

    const auto id = freshId();
    auto res = bus.send(mycad::application::commands::CreateBoxCommand{2.0, 3.0, 4.0, id});

    REQUIRE(res.has_value());
    REQUIRE(renderer->uploadCalls.size() == 1u);
    CHECK(renderer->uploadCalls[0].vertexCount > 0u);
    CHECK(renderer->uploadCalls[0].indexCount > 0u);
    // Index count must be a multiple of 3 (triangle soup).
    CHECK(renderer->uploadCalls[0].indexCount % 3 == 0u);
}

TEST_CASE("CreateBoxCommand - renderer receives no upload on invalid command",
          "[app][integration][createbox][t6]") {
    auto store = std::make_shared<mycad::infrastructure::InMemoryEventStore>();
    auto geom = std::make_shared<mycad::infrastructure::OcctGeometryAdapter>();
    auto ecs = std::make_shared<mycad::infrastructure::EnttRegistry>();
    auto renderer = std::make_shared<MockRenderPort>();

    auto handler = std::make_shared<mycad::application::handlers::CreateBoxCommandHandler>(
        geom, ecs, store, renderer);

    mycad::application::CommandBus bus{store};
    bus.registerHandler<mycad::application::commands::CreateBoxCommand>(handler);

    const auto id = freshId();
    auto res = bus.send(mycad::application::commands::CreateBoxCommand{0.0, 3.0, 4.0, id});

    REQUIRE(!res);
    CHECK(renderer->uploadCalls.empty());
}

TEST_CASE("CreateBoxCommand - two commands upload two distinct meshes",
          "[app][integration][createbox][t6]") {
    auto store = std::make_shared<mycad::infrastructure::InMemoryEventStore>();
    auto geom = std::make_shared<mycad::infrastructure::OcctGeometryAdapter>();
    auto ecs = std::make_shared<mycad::infrastructure::EnttRegistry>();
    auto renderer = std::make_shared<MockRenderPort>();

    auto handler = std::make_shared<mycad::application::handlers::CreateBoxCommandHandler>(
        geom, ecs, store, renderer);

    mycad::application::CommandBus bus{store};
    bus.registerHandler<mycad::application::commands::CreateBoxCommand>(handler);

    const auto id1 = freshId();
    const auto id2 = freshId();

    REQUIRE(
        bus.send(mycad::application::commands::CreateBoxCommand{1.0, 1.0, 1.0, id1}).has_value());
    REQUIRE(
        bus.send(mycad::application::commands::CreateBoxCommand{2.0, 2.0, 2.0, id2}).has_value());

    REQUIRE(renderer->uploadCalls.size() == 2u);
    // The two handles must be distinct.
    CHECK(renderer->uploadCalls[0].handle != renderer->uploadCalls[1].handle);
}
