#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <mycad/domain/Axis3D.hpp>
#include <mycad/domain/BoundingBox.hpp>
#include <mycad/domain/Point2D.hpp>
#include <mycad/domain/Point3D.hpp>
#include <mycad/domain/Tolerance.hpp>
#include <mycad/domain/Transform3D.hpp>
#include <mycad/domain/Vector3D.hpp>

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

// ---------------------------------------------------------------------------
// Vector3D
// ---------------------------------------------------------------------------

TEST_CASE("Vector3D -default construction is zero vector", "[domain][shared][value_objects]") {
    const Vector3D v;
    REQUIRE(v.x == 0.0);
    REQUIRE(v.y == 0.0);
    REQUIRE(v.z == 0.0);
}

TEST_CASE("Vector3D -dot product", "[domain][shared][value_objects]") {
    REQUIRE(Vector3D{1.0, 0.0, 0.0}.dot({0.0, 1.0, 0.0}) == 0.0);  // orthogonal
    const double d = Vector3D{1.0, 2.0, 3.0}.dot({4.0, 5.0, 6.0});
    CHECK_THAT(d, Catch::Matchers::WithinAbs(32.0, 1e-9));
}

TEST_CASE("Vector3D -cross product", "[domain][shared][value_objects]") {
    const Vector3D x{1.0, 0.0, 0.0};
    const Vector3D y{0.0, 1.0, 0.0};
    REQUIRE(x.cross(y) == Vector3D{0.0, 0.0, 1.0});  // right-hand rule
    REQUIRE(y.cross(x) == Vector3D{0.0, 0.0, -1.0});
}

TEST_CASE("Vector3D -length of 3-4-0 vector is 5", "[domain][shared][value_objects]") {
    const double len = Vector3D{3.0, 4.0, 0.0}.length();
    CHECK_THAT(len, Catch::Matchers::WithinAbs(5.0, 1e-9));
}

TEST_CASE("Vector3D -normalized returns unit vector", "[domain][shared][value_objects]") {
    const Vector3D unit = Vector3D{3.0, 4.0, 0.0}.normalized();
    CHECK_THAT(unit.length(), Catch::Matchers::WithinAbs(1.0, 1e-9));
}

TEST_CASE("Vector3D -normalized of zero vector returns zero", "[domain][shared][value_objects]") {
    REQUIRE(Vector3D{}.normalized() == Vector3D{0.0, 0.0, 0.0});
}

TEST_CASE("Vector3D -operator== epsilon boundary", "[domain][shared][value_objects]") {
    REQUIRE(Vector3D{1.0, 0.0, 0.0} == Vector3D{1.0 + 1e-10, 0.0, 0.0});
    REQUIRE_FALSE(Vector3D{1.0, 0.0, 0.0} == Vector3D{1.0 + 1e-8, 0.0, 0.0});
}

// ---------------------------------------------------------------------------
// Axis3D
// ---------------------------------------------------------------------------

TEST_CASE("Axis3D -construction and equality", "[domain][shared][value_objects]") {
    const Axis3D a{Point3D{0.0, 0.0, 0.0}, Vector3D{0.0, 0.0, 1.0}};
    const Axis3D b{Point3D{0.0, 0.0, 0.0}, Vector3D{0.0, 0.0, 1.0}};
    REQUIRE(a == b);
}

TEST_CASE("Axis3D -different origin means not equal", "[domain][shared][value_objects]") {
    const Axis3D a{Point3D{0.0, 0.0, 0.0}, Vector3D{0.0, 0.0, 1.0}};
    const Axis3D b{Point3D{1.0, 0.0, 0.0}, Vector3D{0.0, 0.0, 1.0}};
    REQUIRE_FALSE(a == b);
}

// ---------------------------------------------------------------------------
// BoundingBox
// ---------------------------------------------------------------------------

TEST_CASE("BoundingBox -contains interior point", "[domain][shared][value_objects]") {
    const BoundingBox bb{Point3D{0.0, 0.0, 0.0}, Point3D{10.0, 10.0, 10.0}};
    REQUIRE(bb.contains(Point3D{5.0, 5.0, 5.0}));
}

TEST_CASE("BoundingBox -contains boundary point", "[domain][shared][value_objects]") {
    const BoundingBox bb{Point3D{0.0, 0.0, 0.0}, Point3D{10.0, 10.0, 10.0}};
    REQUIRE(bb.contains(Point3D{0.0, 0.0, 0.0}));
    REQUIRE(bb.contains(Point3D{10.0, 10.0, 10.0}));
}

TEST_CASE("BoundingBox -does not contain exterior point", "[domain][shared][value_objects]") {
    const BoundingBox bb{Point3D{0.0, 0.0, 0.0}, Point3D{10.0, 10.0, 10.0}};
    REQUIRE_FALSE(bb.contains(Point3D{11.0, 5.0, 5.0}));
}

TEST_CASE("BoundingBox -intersects overlapping boxes", "[domain][shared][value_objects]") {
    const BoundingBox a{Point3D{0.0, 0.0, 0.0}, Point3D{5.0, 5.0, 5.0}};
    const BoundingBox b{Point3D{3.0, 3.0, 3.0}, Point3D{8.0, 8.0, 8.0}};
    REQUIRE(a.intersects(b));
    REQUIRE(b.intersects(a));
}

TEST_CASE("BoundingBox -intersects touching boxes", "[domain][shared][value_objects]") {
    const BoundingBox a{Point3D{0.0, 0.0, 0.0}, Point3D{5.0, 5.0, 5.0}};
    const BoundingBox b{Point3D{5.0, 0.0, 0.0}, Point3D{10.0, 5.0, 5.0}};
    REQUIRE(a.intersects(b));
}

TEST_CASE("BoundingBox -does not intersect separate boxes", "[domain][shared][value_objects]") {
    const BoundingBox a{Point3D{0.0, 0.0, 0.0}, Point3D{4.0, 4.0, 4.0}};
    const BoundingBox b{Point3D{5.0, 5.0, 5.0}, Point3D{9.0, 9.0, 9.0}};
    REQUIRE_FALSE(a.intersects(b));
}

TEST_CASE("BoundingBox -expanded increases all sides", "[domain][shared][value_objects]") {
    const BoundingBox bb{Point3D{1.0, 1.0, 1.0}, Point3D{3.0, 3.0, 3.0}};
    const BoundingBox exp = bb.expanded(1.0);
    REQUIRE(exp.min == Point3D{0.0, 0.0, 0.0});
    REQUIRE(exp.max == Point3D{4.0, 4.0, 4.0});
}

// ---------------------------------------------------------------------------
// Transform3D
// ---------------------------------------------------------------------------

TEST_CASE("Transform3D -identity applied to point is unchanged",
          "[domain][shared][value_objects]") {
    const Transform3D I = Transform3D::identity();
    const Point3D p{1.0, 2.0, 3.0};
    REQUIRE(I.apply(p) == p);
}

TEST_CASE("Transform3D -identity applied to vector is unchanged",
          "[domain][shared][value_objects]") {
    const Transform3D I = Transform3D::identity();
    const Vector3D v{1.0, 0.0, 0.0};
    REQUIRE(I.apply(v) == v);
}

TEST_CASE("Transform3D -translation applies to point", "[domain][shared][value_objects]") {
    Transform3D t = Transform3D::identity();
    t.m[3] = 5.0;   // tx
    t.m[7] = -3.0;  // ty
    t.m[11] = 1.0;  // tz
    REQUIRE(t.apply(Point3D{0.0, 0.0, 0.0}) == Point3D{5.0, -3.0, 1.0});
}

TEST_CASE("Transform3D -translation does not affect vectors", "[domain][shared][value_objects]") {
    Transform3D t = Transform3D::identity();
    t.m[3] = 10.0;
    const Vector3D v{1.0, 0.0, 0.0};
    REQUIRE(t.apply(v) == v);
}

TEST_CASE("Transform3D -identity composed with identity is identity",
          "[domain][shared][value_objects]") {
    const Transform3D I = Transform3D::identity();
    REQUIRE(I.composed(I) == I);
}

TEST_CASE("Transform3D -composed applies transforms in order", "[domain][shared][value_objects]") {
    // Two translations: +5 on x, then +3 on x => +8 total
    Transform3D t1 = Transform3D::identity();
    t1.m[3] = 5.0;
    Transform3D t2 = Transform3D::identity();
    t2.m[3] = 3.0;
    const Transform3D combined = t1.composed(t2);
    CHECK_THAT(combined.apply(Point3D{0.0, 0.0, 0.0}).x, Catch::Matchers::WithinAbs(8.0, 1e-9));
}

}  // namespace mycad::domain::tests
