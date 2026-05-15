#include <catch2/catch_test_macros.hpp>
#include <mycad/domain/BRepHandle.hpp>
#include <mycad/domain/GeomError.hpp>
#include <mycad/domain/IGeometryConstructionPort.hpp>
#include <mycad/domain/TessellationParams.hpp>
#include <mycad/domain/TriangleMesh.hpp>

#include <type_traits>

// ---------------------------------------------------------------------------
// Compile-time trait assertions
// ---------------------------------------------------------------------------

// BRepHandle / WireHandle must be trivially copyable so they can be passed
// cheaply by value across the adapter boundary.
static_assert(std::is_trivially_copyable_v<mycad::domain::BRepHandle>,
              "BRepHandle must be trivially copyable");
static_assert(std::is_trivially_copyable_v<mycad::domain::WireHandle>,
              "WireHandle must be trivially copyable");
static_assert(std::is_trivially_copyable_v<mycad::domain::EdgeRef>,
              "EdgeRef must be trivially copyable");
static_assert(std::is_trivially_copyable_v<mycad::domain::FaceRef>,
              "FaceRef must be trivially copyable");
static_assert(std::is_trivially_copyable_v<mycad::domain::TessellationParams>,
              "TessellationParams must be trivially copyable");

// Interface must be abstract (no concrete instances).
static_assert(std::is_abstract_v<mycad::domain::IGeometryConstructionPort>,
              "IGeometryConstructionPort must be abstract");

// IGeometryConstructionPort must not be copyable or movable.
static_assert(!std::is_copy_constructible_v<mycad::domain::IGeometryConstructionPort>,
              "IGeometryConstructionPort must not be copy-constructible");

// ---------------------------------------------------------------------------
// BRepHandle runtime behaviour
// ---------------------------------------------------------------------------

TEST_CASE("BRepHandle - default-constructed handle is invalid", "[domain][geom][BRepHandle]") {
    mycad::domain::BRepHandle h;
    CHECK(!h.valid());
    CHECK(h.id == 0u);
}

TEST_CASE("BRepHandle - non-zero id is valid", "[domain][geom][BRepHandle]") {
    mycad::domain::BRepHandle h{42u};
    CHECK(h.valid());
}

TEST_CASE("BRepHandle - equality and ordering", "[domain][geom][BRepHandle]") {
    mycad::domain::BRepHandle a{1u};
    mycad::domain::BRepHandle b{2u};
    mycad::domain::BRepHandle c{1u};
    CHECK(a == c);
    CHECK(a != b);
    CHECK(a < b);
}

TEST_CASE("WireHandle - default-constructed handle is invalid", "[domain][geom][WireHandle]") {
    mycad::domain::WireHandle w;
    CHECK(!w.valid());
}

// ---------------------------------------------------------------------------
// GeomError
// ---------------------------------------------------------------------------

TEST_CASE("GeomError - fields are accessible", "[domain][geom][GeomError]") {
    mycad::domain::GeomError e{mycad::domain::GeomErrorKind::InvalidInput, "dx must be > 0"};
    CHECK(e.kind == mycad::domain::GeomErrorKind::InvalidInput);
    CHECK(e.message == "dx must be > 0");
}

TEST_CASE("GeomResult - ok path holds value", "[domain][geom][GeomError]") {
    mycad::domain::GeomResult<int> r = 42;
    REQUIRE(r.has_value());
    CHECK(*r == 42);
}

TEST_CASE("GeomResult - error path holds GeomError", "[domain][geom][GeomError]") {
    mycad::domain::GeomResult<int> r = std::unexpected(mycad::domain::GeomError{
        mycad::domain::GeomErrorKind::AlgorithmFailed, "OCCT reported failure"});
    REQUIRE(!r.has_value());
    CHECK(r.error().kind == mycad::domain::GeomErrorKind::AlgorithmFailed);
}

// ---------------------------------------------------------------------------
// TriangleMesh
// ---------------------------------------------------------------------------

TEST_CASE("TriangleMesh - default constructed is empty", "[domain][geom][TriangleMesh]") {
    mycad::domain::TriangleMesh m;
    CHECK(m.empty());
    CHECK(m.vertexCount() == 0u);
    CHECK(m.triangleCount() == 0u);
}

TEST_CASE("TriangleMesh - vertex and triangle counts", "[domain][geom][TriangleMesh]") {
    mycad::domain::TriangleMesh m;
    // One triangle: 3 vertices (9 floats), 3 indices.
    m.vertices = {0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f};
    m.indices = {0, 1, 2};
    CHECK(!m.empty());
    CHECK(m.vertexCount() == 3u);
    CHECK(m.triangleCount() == 1u);
}

// ---------------------------------------------------------------------------
// TessellationParams
// ---------------------------------------------------------------------------

TEST_CASE("TessellationParams - default values", "[domain][geom][TessellationParams]") {
    mycad::domain::TessellationParams p;
    CHECK(p.linearDeflection == 0.1);
    CHECK(p.angularDeflection == 0.5);
    CHECK(!p.relative);
}
