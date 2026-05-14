#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <mycad/domain/Point2D.hpp>
#include <mycad/domain/Point3D.hpp>
#include <mycad/domain/Tolerance.hpp>

namespace mycad::domain::tests {

// ---------------------------------------------------------------------------
// Tolerance
// ---------------------------------------------------------------------------

TEST_CASE("nearEqual -same value is equal", "[domain][shared][value_objects]") {
    REQUIRE(nearEqual(1.0, 1.0));
}

TEST_CASE("nearEqual -difference within epsilon is equal", "[domain][shared][value_objects]") {
    REQUIRE(nearEqual(1.0, 1.0 + 1e-10));
    REQUIRE(nearEqual(1.0, 1.0 - 1e-10));
}

TEST_CASE("nearEqual -difference beyond epsilon is not equal", "[domain][shared][value_objects]") {
    REQUIRE_FALSE(nearEqual(1.0, 1.0 + 1e-8));
    REQUIRE_FALSE(nearEqual(1.0, 1.0 - 1e-8));
}

TEST_CASE("nearEqual -custom tolerance respected", "[domain][shared][value_objects]") {
    REQUIRE(nearEqual(1.0, 2.0, 1.5));
    REQUIRE_FALSE(nearEqual(1.0, 2.0, 0.5));
}

TEST_CASE("bitwiseEqual -identical values are equal", "[domain][shared][value_objects]") {
    REQUIRE(bitwiseEqual(0.0, 0.0));
    REQUIRE(bitwiseEqual(3.14, 3.14));
}

TEST_CASE("bitwiseEqual -positive and negative zero differ", "[domain][shared][value_objects]") {
    // IEEE 754: +0.0 and -0.0 are bitwise distinct
    REQUIRE_FALSE(bitwiseEqual(0.0, -0.0));
}

// ---------------------------------------------------------------------------
// Point2D
// ---------------------------------------------------------------------------

TEST_CASE("Point2D -default construction yields origin", "[domain][shared][value_objects]") {
    const Point2D p;
    REQUIRE(p.x == 0.0);
    REQUIRE(p.y == 0.0);
}

TEST_CASE("Point2D -construction sets fields correctly", "[domain][shared][value_objects]") {
    const Point2D p{1.5, 2.5};
    REQUIRE(p.x == 1.5);
    REQUIRE(p.y == 2.5);
}

TEST_CASE("Point2D -translated returns new point, original unchanged",
          "[domain][shared][value_objects]") {
    const Point2D origin{1.0, 2.0};
    const Point2D moved = origin.translated({3.0, 4.0});

    REQUIRE(moved == Point2D{4.0, 6.0});
    // Original must be unchanged (immutability, ADR-0013 decision 2)
    REQUIRE(origin == Point2D{1.0, 2.0});
}

TEST_CASE("Point2D -operator== identical points are equal", "[domain][shared][value_objects]") {
    REQUIRE(Point2D{1.0, 2.0} == Point2D{1.0, 2.0});
    REQUIRE(Point2D{} == Point2D{});
}

TEST_CASE("Point2D -operator== epsilon boundary", "[domain][shared][value_objects]") {
    // Within epsilon: equal
    REQUIRE(Point2D{1.0, 1.0} == Point2D{1.0 + 1e-10, 1.0 - 1e-10});
    // Beyond epsilon: not equal
    REQUIRE_FALSE(Point2D{1.0, 1.0} == Point2D{1.0 + 1e-8, 1.0});
}

TEST_CASE("Point2D -to_string format", "[domain][shared][value_objects]") {
    const std::string s = to_string(Point2D{1.5, 2.5});
    REQUIRE(s == "Point2D{x=1.500000, y=2.500000}");
}

TEST_CASE("Point2D -to_string origin", "[domain][shared][value_objects]") {
    REQUIRE(to_string(Point2D{}) == "Point2D{x=0.000000, y=0.000000}");
}

// ---------------------------------------------------------------------------
// Point3D
// ---------------------------------------------------------------------------

TEST_CASE("Point3D -default construction yields origin", "[domain][shared][value_objects]") {
    const Point3D p;
    REQUIRE(p.x == 0.0);
    REQUIRE(p.y == 0.0);
    REQUIRE(p.z == 0.0);
}

TEST_CASE("Point3D -construction sets fields correctly", "[domain][shared][value_objects]") {
    const Point3D p{1.0, 2.0, 3.0};
    REQUIRE(p.x == 1.0);
    REQUIRE(p.y == 2.0);
    REQUIRE(p.z == 3.0);
}

TEST_CASE("Point3D -translated returns new point, original unchanged",
          "[domain][shared][value_objects]") {
    const Point3D origin{1.0, 2.0, 3.0};
    const Point3D moved = origin.translated({1.0, 1.0, 1.0});

    REQUIRE(moved == Point3D{2.0, 3.0, 4.0});
    REQUIRE(origin == Point3D{1.0, 2.0, 3.0});
}

TEST_CASE("Point3D -distanceTo known 3-4-5 triangle", "[domain][shared][value_objects]") {
    const Point3D a{0.0, 0.0, 0.0};
    const Point3D b{3.0, 4.0, 0.0};
    CHECK_THAT(a.distanceTo(b), Catch::Matchers::WithinAbs(5.0, 1e-9));
}

TEST_CASE("Point3D -distanceTo self is zero", "[domain][shared][value_objects]") {
    const Point3D p{1.0, 2.0, 3.0};
    CHECK_THAT(p.distanceTo(p), Catch::Matchers::WithinAbs(0.0, 1e-9));
}

TEST_CASE("Point3D -distanceTo is symmetric", "[domain][shared][value_objects]") {
    const Point3D a{1.0, 0.0, 0.0};
    const Point3D b{4.0, 4.0, 0.0};
    CHECK_THAT(a.distanceTo(b), Catch::Matchers::WithinAbs(b.distanceTo(a), 1e-9));
}

TEST_CASE("Point3D -operator== epsilon boundary", "[domain][shared][value_objects]") {
    REQUIRE(Point3D{1.0, 1.0, 1.0} == Point3D{1.0 + 1e-10, 1.0, 1.0});
    REQUIRE_FALSE(Point3D{1.0, 1.0, 1.0} == Point3D{1.0 + 1e-8, 1.0, 1.0});
}

TEST_CASE("Point3D -to_string format", "[domain][shared][value_objects]") {
    REQUIRE(to_string(Point3D{3.0, 4.0, 0.0}) == "Point3D{x=3.000000, y=4.000000, z=0.000000}");
}

TEST_CASE("Point3D -to_string origin", "[domain][shared][value_objects]") {
    REQUIRE(to_string(Point3D{}) == "Point3D{x=0.000000, y=0.000000, z=0.000000}");
}

}  // namespace mycad::domain::tests
