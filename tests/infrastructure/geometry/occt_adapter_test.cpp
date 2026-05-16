#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <mycad/domain/BoundingBox.hpp>
#include <mycad/domain/GeomError.hpp>
#include <mycad/domain/TriangleMesh.hpp>
#include <mycad/infrastructure/OcctGeometryAdapter.hpp>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Fixture: creates a fresh adapter for each test section.
mycad::infrastructure::OcctGeometryAdapter makeAdapter() {
    return mycad::infrastructure::OcctGeometryAdapter{};
}

}  // namespace

// ---------------------------------------------------------------------------
// makeBox
// ---------------------------------------------------------------------------

TEST_CASE("OcctGeometryAdapter - makeBox returns valid handle", "[infra][occt][makeBox]") {
    auto adapter = makeAdapter();
    auto result = adapter.makeBox(10.0, 10.0, 10.0);
    REQUIRE(result.has_value());
    CHECK(result->valid());
}

TEST_CASE("OcctGeometryAdapter - makeBox zero dx returns InvalidInput", "[infra][occt][makeBox]") {
    auto adapter = makeAdapter();
    auto result = adapter.makeBox(0.0, 10.0, 10.0);
    REQUIRE(!result.has_value());
    CHECK(result.error().kind == mycad::domain::GeomErrorKind::InvalidInput);
}

TEST_CASE("OcctGeometryAdapter - makeBox negative dimension returns InvalidInput",
          "[infra][occt][makeBox]") {
    auto adapter = makeAdapter();
    auto result = adapter.makeBox(10.0, -1.0, 10.0);
    REQUIRE(!result.has_value());
    CHECK(result.error().kind == mycad::domain::GeomErrorKind::InvalidInput);
}

TEST_CASE("OcctGeometryAdapter - successive makeBox calls return distinct handles",
          "[infra][occt][makeBox]") {
    auto adapter = makeAdapter();
    auto h1 = adapter.makeBox(10.0, 10.0, 10.0);
    auto h2 = adapter.makeBox(20.0, 20.0, 20.0);
    REQUIRE(h1.has_value());
    REQUIRE(h2.has_value());
    CHECK(h1->id != h2->id);
}

// ---------------------------------------------------------------------------
// tessellate
// ---------------------------------------------------------------------------

TEST_CASE("OcctGeometryAdapter - tessellate box produces non-empty mesh",
          "[infra][occt][tessellate]") {
    auto adapter = makeAdapter();
    auto handle = adapter.makeBox(10.0, 10.0, 10.0);
    REQUIRE(handle.has_value());

    mycad::domain::TessellationParams params;
    auto mesh = adapter.tessellate(*handle, params);
    REQUIRE(mesh.has_value());
    CHECK(!mesh->empty());
    CHECK(mesh->vertexCount() > 0u);
    CHECK(mesh->triangleCount() > 0u);
}

TEST_CASE("OcctGeometryAdapter - tessellate indices form valid triangles",
          "[infra][occt][tessellate]") {
    auto adapter = makeAdapter();
    auto handle = adapter.makeBox(5.0, 5.0, 5.0);
    REQUIRE(handle.has_value());

    mycad::domain::TessellationParams params;
    auto mesh = adapter.tessellate(*handle, params);
    REQUIRE(mesh.has_value());

    // Triangle count must be integral.
    CHECK(mesh->indices.size() % 3 == 0u);

    // Every index must be within the vertex buffer.
    const auto maxIdx = static_cast<uint32_t>(mesh->vertexCount());
    for (const uint32_t idx : mesh->indices) {
        CHECK(idx < maxIdx);
    }
}

TEST_CASE("OcctGeometryAdapter - tessellate unknown handle returns InvalidInput",
          "[infra][occt][tessellate]") {
    auto adapter = makeAdapter();
    mycad::domain::BRepHandle bogus{999u};
    mycad::domain::TessellationParams params;
    auto result = adapter.tessellate(bogus, params);
    REQUIRE(!result.has_value());
    CHECK(result.error().kind == mycad::domain::GeomErrorKind::InvalidInput);
}

// ---------------------------------------------------------------------------
// bbox
// ---------------------------------------------------------------------------

TEST_CASE("OcctGeometryAdapter - bbox of unit cube", "[infra][occt][bbox]") {
    auto adapter = makeAdapter();
    auto handle = adapter.makeBox(1.0, 1.0, 1.0);
    REQUIRE(handle.has_value());

    auto bb = adapter.bbox(*handle);
    REQUIRE(bb.has_value());

    using Catch::Matchers::WithinAbs;
    // OCCT BRepPrimAPI_MakeBox places the corner at the origin.
    CHECK_THAT(bb->min.x, WithinAbs(0.0, 1e-6));
    CHECK_THAT(bb->min.y, WithinAbs(0.0, 1e-6));
    CHECK_THAT(bb->min.z, WithinAbs(0.0, 1e-6));
    CHECK_THAT(bb->max.x, WithinAbs(1.0, 1e-6));
    CHECK_THAT(bb->max.y, WithinAbs(1.0, 1e-6));
    CHECK_THAT(bb->max.z, WithinAbs(1.0, 1e-6));
}

TEST_CASE("OcctGeometryAdapter - bbox of rectangular box", "[infra][occt][bbox]") {
    auto adapter = makeAdapter();
    auto handle = adapter.makeBox(3.0, 5.0, 7.0);
    REQUIRE(handle.has_value());

    auto bb = adapter.bbox(*handle);
    REQUIRE(bb.has_value());

    using Catch::Matchers::WithinAbs;
    CHECK_THAT(bb->max.x, WithinAbs(3.0, 1e-6));
    CHECK_THAT(bb->max.y, WithinAbs(5.0, 1e-6));
    CHECK_THAT(bb->max.z, WithinAbs(7.0, 1e-6));
}

TEST_CASE("OcctGeometryAdapter - bbox unknown handle returns InvalidInput", "[infra][occt][bbox]") {
    auto adapter = makeAdapter();
    mycad::domain::BRepHandle bogus{42u};
    auto result = adapter.bbox(bogus);
    REQUIRE(!result.has_value());
    CHECK(result.error().kind == mycad::domain::GeomErrorKind::InvalidInput);
}

// ---------------------------------------------------------------------------
// release
// ---------------------------------------------------------------------------

TEST_CASE("OcctGeometryAdapter - release frees handle", "[infra][occt][release]") {
    auto adapter = makeAdapter();
    auto handle = adapter.makeBox(10.0, 10.0, 10.0);
    REQUIRE(handle.has_value());

    adapter.release(*handle);

    // After release, tessellate should return InvalidInput.
    mycad::domain::TessellationParams params;
    auto result = adapter.tessellate(*handle, params);
    REQUIRE(!result.has_value());
    CHECK(result.error().kind == mycad::domain::GeomErrorKind::InvalidInput);
}

TEST_CASE("OcctGeometryAdapter - double release is a no-op", "[infra][occt][release]") {
    auto adapter = makeAdapter();
    auto handle = adapter.makeBox(10.0, 10.0, 10.0);
    REQUIRE(handle.has_value());

    // Must not crash.
    adapter.release(*handle);
    adapter.release(*handle);
}

// ---------------------------------------------------------------------------
// Not-implemented stubs
// ---------------------------------------------------------------------------

TEST_CASE("OcctGeometryAdapter - makeCylinder returns NotImplemented", "[infra][occt][stubs]") {
    auto adapter = makeAdapter();
    auto result = adapter.makeCylinder(5.0, 10.0);
    REQUIRE(!result.has_value());
    CHECK(result.error().kind == mycad::domain::GeomErrorKind::NotImplemented);
}
