#pragma once

#include <mycad/domain/BoundingBox.hpp>
#include <mycad/domain/Point2D.hpp>
#include <mycad/domain/Point3D.hpp>
#include <mycad/domain/Vector3D.hpp>

#include <optional>
#include <span>

/// @file Domain port for computational geometry (ADR-0004).
///
/// Domain 层通过此纯虚接口请求几何计算，不直接依赖 OCCT。
/// infrastructure/geometry/OcctGeometryAdapter 实现此端口。

namespace mycad::domain {

/// @brief Pure interface for geometry computations needed by the domain.
///
/// 所有方法均 noexcept，返回 std::optional 表示"无解"情形。
/// 调用方应检查 optional 有效性再使用结果。
///
/// @thread-safe 实现类应保证线程安全。
class IGeometryPort {
public:
    virtual ~IGeometryPort() = default;

    IGeometryPort(const IGeometryPort&) = default;
    IGeometryPort& operator=(const IGeometryPort&) = default;
    IGeometryPort(IGeometryPort&&) = default;
    IGeometryPort& operator=(IGeometryPort&&) = default;

    // ------------------------------------------------------------------
    // 2-D operations  (sketch domain)
    // ------------------------------------------------------------------

    /// @brief Euclidean distance between two 2-D points.
    [[nodiscard]] virtual double distanceBetween(Point2D a, Point2D b) const noexcept = 0;

    /// @brief Midpoint of segment [a, b].
    [[nodiscard]] virtual Point2D midpoint(Point2D a, Point2D b) const noexcept = 0;

    /// @brief Closest point on segment [segA, segB] to point p.
    [[nodiscard]] virtual Point2D
    closestPointOnSegment(Point2D p, Point2D segA, Point2D segB) const noexcept = 0;

    /// @brief Intersection of two infinite lines defined by (origin, direction).
    ///
    /// 两线平行或共线时返回 std::nullopt。
    /// @param d1 Direction of line 1 (need not be normalised).
    /// @param d2 Direction of line 2 (need not be normalised).
    [[nodiscard]] virtual std::optional<Point2D>
    intersectLines(Point2D origin1, Vector3D d1, Point2D origin2, Vector3D d2) const noexcept = 0;

    /// @brief Signed angle in radians at vertex from ray (vertex→from) to ray (vertex→to).
    ///
    /// Range: [-π, π].
    [[nodiscard]] virtual double
    angleBetween(Point2D from, Point2D vertex, Point2D to) const noexcept = 0;

    // ------------------------------------------------------------------
    // 3-D operations  (feature domain)
    // ------------------------------------------------------------------

    /// @brief Euclidean distance between two 3-D points.
    [[nodiscard]] virtual double distanceBetween(Point3D a, Point3D b) const noexcept = 0;

    /// @brief Closest point on segment [segA, segB] to point p in 3-D.
    [[nodiscard]] virtual Point3D
    closestPointOnSegment(Point3D p, Point3D segA, Point3D segB) const noexcept = 0;

    /// @brief Projects p onto the plane defined by (origin, normal).
    ///
    /// @param normal Need not be normalised; must not be zero.
    [[nodiscard]] virtual Point3D
    projectPointOntoPlane(Point3D p, Point3D planeOrigin, Vector3D planeNormal) const noexcept = 0;

    /// @brief Intersection of a ray with a plane.
    ///
    /// 射线平行于平面时返回 std::nullopt。
    /// @param rayDir    Need not be normalised; must not be zero.
    /// @param planeNormal Need not be normalised; must not be zero.
    [[nodiscard]] virtual std::optional<Point3D>
    intersectRayWithPlane(Point3D rayOrigin,
                          Vector3D rayDir,
                          Point3D planeOrigin,
                          Vector3D planeNormal) const noexcept = 0;

    // ------------------------------------------------------------------
    // Bounding
    // ------------------------------------------------------------------

    /// @brief Axis-aligned bounding box enclosing all given points.
    ///
    /// @param points Must not be empty; behaviour undefined for empty span.
    [[nodiscard]] virtual BoundingBox
    computeBoundingBox(std::span<const Point3D> points) const noexcept = 0;

protected:
    IGeometryPort() = default;
};

}  // namespace mycad::domain
