#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <mycad/domain/IGeometryPort.hpp>

#include <cmath>
#include <numbers>

namespace mycad::domain::tests {

// ---------------------------------------------------------------------------
// Minimal stub that implements IGeometryPort with correct pure math.
// Tests verify the stub, which validates the interface contract.
// ---------------------------------------------------------------------------

class StubGeometryPort final : public IGeometryPort {
public:
    // 2-D ---------------------------------------------------------------

    double distanceBetween(Point2D a, Point2D b) const noexcept override {
        const double dx = b.x - a.x;
        const double dy = b.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    Point2D midpoint(Point2D a, Point2D b) const noexcept override {
        return Point2D{(a.x + b.x) * 0.5, (a.y + b.y) * 0.5};
    }

    Point2D closestPointOnSegment(Point2D p, Point2D segA, Point2D segB) const noexcept override {
        const double dx = segB.x - segA.x;
        const double dy = segB.y - segA.y;
        const double len2 = dx * dx + dy * dy;
        if (len2 < 1e-18)
            return segA;
        const double t =
            std::max(0.0, std::min(1.0, ((p.x - segA.x) * dx + (p.y - segA.y) * dy) / len2));
        return Point2D{segA.x + t * dx, segA.y + t * dy};
    }

    std::optional<Point2D>
    intersectLines(Point2D o1, Vector3D d1, Point2D o2, Vector3D d2) const noexcept override {
        // 2-D cross product of (d1.x, d1.y) and (d2.x, d2.y)
        const double cross = d1.x * d2.y - d1.y * d2.x;
        if (std::abs(cross) < 1e-12)
            return std::nullopt;  // parallel
        const double dx = o2.x - o1.x;
        const double dy = o2.y - o1.y;
        const double t = (dx * d2.y - dy * d2.x) / cross;
        return Point2D{o1.x + t * d1.x, o1.y + t * d1.y};
    }

    double angleBetween(Point2D from, Point2D vertex, Point2D to) const noexcept override {
        const double ax = from.x - vertex.x;
        const double ay = from.y - vertex.y;
        const double bx = to.x - vertex.x;
        const double by = to.y - vertex.y;
        return std::atan2(ax * by - ay * bx, ax * bx + ay * by);
    }

    // 3-D ---------------------------------------------------------------

    double distanceBetween(Point3D a, Point3D b) const noexcept override {
        const double dx = b.x - a.x;
        const double dy = b.y - a.y;
        const double dz = b.z - a.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    Point3D closestPointOnSegment(Point3D p, Point3D segA, Point3D segB) const noexcept override {
        const double dx = segB.x - segA.x;
        const double dy = segB.y - segA.y;
        const double dz = segB.z - segA.z;
        const double len2 = dx * dx + dy * dy + dz * dz;
        if (len2 < 1e-18)
            return segA;
        const double t = std::max(
            0.0,
            std::min(1.0,
                     ((p.x - segA.x) * dx + (p.y - segA.y) * dy + (p.z - segA.z) * dz) / len2));
        return Point3D{segA.x + t * dx, segA.y + t * dy, segA.z + t * dz};
    }

    Point3D
    projectPointOntoPlane(Point3D p, Point3D origin, Vector3D normal) const noexcept override {
        const double nx = normal.x, ny = normal.y, nz = normal.z;
        const double len2 = nx * nx + ny * ny + nz * nz;
        if (len2 < 1e-18)
            return p;
        const double dot = (p.x - origin.x) * nx + (p.y - origin.y) * ny + (p.z - origin.z) * nz;
        const double t = dot / len2;
        return Point3D{p.x - t * nx, p.y - t * ny, p.z - t * nz};
    }

    std::optional<Point3D> intersectRayWithPlane(Point3D rayOrigin,
                                                 Vector3D rayDir,
                                                 Point3D planeOrigin,
                                                 Vector3D planeNormal) const noexcept override {
        const double denom =
            rayDir.x * planeNormal.x + rayDir.y * planeNormal.y + rayDir.z * planeNormal.z;
        if (std::abs(denom) < 1e-12)
            return std::nullopt;
        const double t = ((planeOrigin.x - rayOrigin.x) * planeNormal.x +
                          (planeOrigin.y - rayOrigin.y) * planeNormal.y +
                          (planeOrigin.z - rayOrigin.z) * planeNormal.z) /
                         denom;
        return Point3D{
            rayOrigin.x + t * rayDir.x, rayOrigin.y + t * rayDir.y, rayOrigin.z + t * rayDir.z};
    }

    BoundingBox computeBoundingBox(std::span<const Point3D> pts) const noexcept override {
        Point3D lo = pts[0];
        Point3D hi = pts[0];
        for (const auto& p : pts) {
            if (p.x < lo.x)
                lo.x = p.x;
            if (p.y < lo.y)
                lo.y = p.y;
            if (p.z < lo.z)
                lo.z = p.z;
            if (p.x > hi.x)
                hi.x = p.x;
            if (p.y > hi.y)
                hi.y = p.y;
            if (p.z > hi.z)
                hi.z = p.z;
        }
        return BoundingBox{lo, hi};
    }
};

// ---------------------------------------------------------------------------
// 2-D tests
// ---------------------------------------------------------------------------

TEST_CASE("IGeometryPort -distanceBetween 2D origin to (3,4) is 5", "[domain][shared][port]") {
    const StubGeometryPort g;
    const double d = g.distanceBetween(Point2D{0, 0}, Point2D{3, 4});
    REQUIRE_THAT(d, Catch::Matchers::WithinAbs(5.0, 1e-9));
}

TEST_CASE("IGeometryPort -distanceBetween 2D same point is zero", "[domain][shared][port]") {
    const StubGeometryPort g;
    const double d = g.distanceBetween(Point2D{1, 2}, Point2D{1, 2});
    REQUIRE_THAT(d, Catch::Matchers::WithinAbs(0.0, 1e-9));
}

TEST_CASE("IGeometryPort -midpoint 2D", "[domain][shared][port]") {
    const StubGeometryPort g;
    const Point2D m = g.midpoint(Point2D{0, 0}, Point2D{4, 6});
    REQUIRE_THAT(m.x, Catch::Matchers::WithinAbs(2.0, 1e-9));
    REQUIRE_THAT(m.y, Catch::Matchers::WithinAbs(3.0, 1e-9));
}

TEST_CASE("IGeometryPort -closestPointOnSegment 2D projection inside segment",
          "[domain][shared][port]") {
    const StubGeometryPort g;
    // Segment (0,0)-(4,0), point (2,3) -> closest is (2,0)
    const Point2D c = g.closestPointOnSegment(Point2D{2, 3}, Point2D{0, 0}, Point2D{4, 0});
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(2.0, 1e-9));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(0.0, 1e-9));
}

TEST_CASE("IGeometryPort -closestPointOnSegment 2D clamped to segA", "[domain][shared][port]") {
    const StubGeometryPort g;
    // Point behind segA
    const Point2D c = g.closestPointOnSegment(Point2D{-5, 0}, Point2D{0, 0}, Point2D{4, 0});
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(0.0, 1e-9));
}

TEST_CASE("IGeometryPort -intersectLines 2D crossing at (2,2)", "[domain][shared][port]") {
    const StubGeometryPort g;
    // Line 1: origin (0,0), dir (1,1)
    // Line 2: origin (4,0), dir (-1,1)
    const auto result =
        g.intersectLines(Point2D{0, 0}, Vector3D{1, 1, 0}, Point2D{4, 0}, Vector3D{-1, 1, 0});
    REQUIRE(result.has_value());
    REQUIRE_THAT(result->x, Catch::Matchers::WithinAbs(2.0, 1e-9));
    REQUIRE_THAT(result->y, Catch::Matchers::WithinAbs(2.0, 1e-9));
}

TEST_CASE("IGeometryPort -intersectLines 2D parallel returns nullopt", "[domain][shared][port]") {
    const StubGeometryPort g;
    const auto result =
        g.intersectLines(Point2D{0, 0}, Vector3D{1, 0, 0}, Point2D{0, 1}, Vector3D{1, 0, 0});
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("IGeometryPort -angleBetween 2D right angle is pi/2", "[domain][shared][port]") {
    const StubGeometryPort g;
    const double angle = g.angleBetween(Point2D{1, 0}, Point2D{0, 0}, Point2D{0, 1});
    REQUIRE_THAT(std::abs(angle), Catch::Matchers::WithinAbs(std::numbers::pi / 2.0, 1e-9));
}

// ---------------------------------------------------------------------------
// 3-D tests
// ---------------------------------------------------------------------------

TEST_CASE("IGeometryPort -distanceBetween 3D (0,0,0) to (1,2,2) is 3", "[domain][shared][port]") {
    const StubGeometryPort g;
    const double d = g.distanceBetween(Point3D{0, 0, 0}, Point3D{1, 2, 2});
    REQUIRE_THAT(d, Catch::Matchers::WithinAbs(3.0, 1e-9));
}

TEST_CASE("IGeometryPort -projectPointOntoPlane XY plane", "[domain][shared][port]") {
    const StubGeometryPort g;
    // XY plane: origin (0,0,0), normal (0,0,1)
    const Point3D proj =
        g.projectPointOntoPlane(Point3D{3, 4, 7}, Point3D{0, 0, 0}, Vector3D{0, 0, 1});
    REQUIRE_THAT(proj.x, Catch::Matchers::WithinAbs(3.0, 1e-9));
    REQUIRE_THAT(proj.y, Catch::Matchers::WithinAbs(4.0, 1e-9));
    REQUIRE_THAT(proj.z, Catch::Matchers::WithinAbs(0.0, 1e-9));
}

TEST_CASE("IGeometryPort -intersectRayWithPlane hits XY plane at z=0", "[domain][shared][port]") {
    const StubGeometryPort g;
    // Ray from (0,0,5) going down (0,0,-1), XY plane z=0
    const auto result = g.intersectRayWithPlane(
        Point3D{0, 0, 5}, Vector3D{0, 0, -1}, Point3D{0, 0, 0}, Vector3D{0, 0, 1});
    REQUIRE(result.has_value());
    REQUIRE_THAT(result->x, Catch::Matchers::WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(result->y, Catch::Matchers::WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(result->z, Catch::Matchers::WithinAbs(0.0, 1e-9));
}

TEST_CASE("IGeometryPort -intersectRayWithPlane parallel returns nullopt",
          "[domain][shared][port]") {
    const StubGeometryPort g;
    const auto result = g.intersectRayWithPlane(
        Point3D{0, 0, 1}, Vector3D{1, 0, 0}, Point3D{0, 0, 0}, Vector3D{0, 0, 1});
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("IGeometryPort -closestPointOnSegment 3D midpoint of diagonal",
          "[domain][shared][port]") {
    const StubGeometryPort g;
    // Segment (0,0,0)-(2,2,2), point (3,0,0) -> closest is somewhere on segment
    const Point3D c = g.closestPointOnSegment(Point3D{3, 0, 0}, Point3D{0, 0, 0}, Point3D{2, 2, 2});
    // t = dot((3,0,0),(2,2,2)) / |(2,2,2)|^2 = 6/12 = 0.5 -> (1,1,1)
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(1.0, 1e-9));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(1.0, 1e-9));
    REQUIRE_THAT(c.z, Catch::Matchers::WithinAbs(1.0, 1e-9));
}

TEST_CASE("IGeometryPort -computeBoundingBox single point", "[domain][shared][port]") {
    const StubGeometryPort g;
    const std::vector<Point3D> pts{{3, 1, 4}};
    const BoundingBox bb = g.computeBoundingBox(pts);
    REQUIRE_THAT(bb.min.x, Catch::Matchers::WithinAbs(3.0, 1e-9));
    REQUIRE_THAT(bb.max.x, Catch::Matchers::WithinAbs(3.0, 1e-9));
}

TEST_CASE("IGeometryPort -computeBoundingBox multiple points", "[domain][shared][port]") {
    const StubGeometryPort g;
    const std::vector<Point3D> pts{{1, 2, 3}, {-1, 5, 0}, {4, -2, 6}};
    const BoundingBox bb = g.computeBoundingBox(pts);
    REQUIRE_THAT(bb.min.x, Catch::Matchers::WithinAbs(-1.0, 1e-9));
    REQUIRE_THAT(bb.max.x, Catch::Matchers::WithinAbs(4.0, 1e-9));
    REQUIRE_THAT(bb.min.y, Catch::Matchers::WithinAbs(-2.0, 1e-9));
    REQUIRE_THAT(bb.max.y, Catch::Matchers::WithinAbs(5.0, 1e-9));
    REQUIRE_THAT(bb.min.z, Catch::Matchers::WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(bb.max.z, Catch::Matchers::WithinAbs(6.0, 1e-9));
}

}  // namespace mycad::domain::tests
