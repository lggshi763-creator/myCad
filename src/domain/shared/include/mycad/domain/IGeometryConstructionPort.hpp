#pragma once

#include <mycad/domain/BRepHandle.hpp>
#include <mycad/domain/BoundingBox.hpp>
#include <mycad/domain/GeomError.hpp>
#include <mycad/domain/TessellationParams.hpp>
#include <mycad/domain/Transform3D.hpp>
#include <mycad/domain/TriangleMesh.hpp>

#include <span>
#include <vector>

/// @file Port interface for solid geometry construction and tessellation (ADR-0004).
///
/// Implemented by OcctGeometryAdapter (infrastructure/geometry). No OCCT header
/// is required by consumers of this interface.

namespace mycad::domain {

/// @brief Port for solid geometry construction, query, tessellation, and lifecycle.
///
/// Domain and application code depends only on this interface.
/// The concrete implementation (OcctGeometryAdapter) lives entirely in
/// infrastructure/geometry and #includes OCCT only in its .cpp files (ADR-0004).
///
/// Methods tagged [Sprint X] are implemented in that sprint; earlier sprints
/// return GeomError{NotImplemented} from the adapter.
///
/// @see IGeometryPort for pure spatial-query operations (distance, projection).
class IGeometryConstructionPort {
public:
    virtual ~IGeometryConstructionPort() = default;

    IGeometryConstructionPort(const IGeometryConstructionPort&) = delete;
    IGeometryConstructionPort& operator=(const IGeometryConstructionPort&) = delete;
    IGeometryConstructionPort(IGeometryConstructionPort&&) = delete;
    IGeometryConstructionPort& operator=(IGeometryConstructionPort&&) = delete;

    // -------------------------------------------------------------------------
    // Primitive construction
    // -------------------------------------------------------------------------

    /// @brief Creates an axis-aligned box with the given dimensions. [Sprint 0.3]
    ///
    /// @param dx  Width  (mm, must be > 0).
    /// @param dy  Depth  (mm, must be > 0).
    /// @param dz  Height (mm, must be > 0).
    /// @return Handle to the new solid, or GeomError{InvalidInput} if any
    ///         dimension is <= 0.
    [[nodiscard]] virtual GeomResult<BRepHandle> makeBox(double dx, double dy, double dz) = 0;

    /// @brief Creates a cylinder aligned with the Z axis. [Sprint 1.B]
    ///
    /// @param radius  Cylinder radius (mm, must be > 0).
    /// @param height  Cylinder height (mm, must be > 0).
    [[nodiscard]] virtual GeomResult<BRepHandle> makeCylinder(double radius, double height) = 0;

    /// @brief Creates a sphere centred at the origin. [Sprint 1.B]
    ///
    /// @param radius  Sphere radius (mm, must be > 0).
    [[nodiscard]] virtual GeomResult<BRepHandle> makeSphere(double radius) = 0;

    // -------------------------------------------------------------------------
    // Feature operations (Sprint 1.B)
    // -------------------------------------------------------------------------

    /// @brief Linearly extrudes a wire profile. [Sprint 1.B]
    ///
    /// @param profile   Wire to extrude.
    /// @param distance  Extrusion distance along Z (mm, positive = upward).
    [[nodiscard]] virtual GeomResult<BRepHandle> prismaticExtrude(WireHandle profile,
                                                                  double distance) = 0;

    /// @brief Revolves a wire profile around the Z axis. [Sprint 1.B]
    ///
    /// @param profile   Wire to revolve.
    /// @param angleDeg  Revolution angle in degrees (0 < angleDeg <= 360).
    [[nodiscard]] virtual GeomResult<BRepHandle> revolve(WireHandle profile, double angleDeg) = 0;

    /// @brief Fuses two solids into one. [Sprint 1.B]
    [[nodiscard]] virtual GeomResult<BRepHandle> booleanUnion(BRepHandle a, BRepHandle b) = 0;

    /// @brief Subtracts tool from base. [Sprint 1.B]
    [[nodiscard]] virtual GeomResult<BRepHandle> booleanCut(BRepHandle base, BRepHandle tool) = 0;

    /// @brief Rounds specified edges with a constant radius fillet. [Sprint 1.B]
    ///
    /// @param solid   Solid to fillet.
    /// @param edges   Edge references within solid (must all belong to solid).
    /// @param radius  Fillet radius (mm, must be > 0).
    [[nodiscard]] virtual GeomResult<BRepHandle>
    fillet(BRepHandle solid, std::span<const EdgeRef> edges, double radius) = 0;

    /// @brief Bevels specified edges with a symmetric chamfer. [Sprint 1.B]
    ///
    /// @param solid     Solid to chamfer.
    /// @param edges     Edge references within solid (must all belong to solid).
    /// @param distance  Chamfer distance (mm, must be > 0).
    [[nodiscard]] virtual GeomResult<BRepHandle>
    chamfer(BRepHandle solid, std::span<const EdgeRef> edges, double distance) = 0;

    // -------------------------------------------------------------------------
    // Topology query (Sprint 1.B)
    // -------------------------------------------------------------------------

    /// @brief Returns all edge references for the solid. [Sprint 1.B]
    /// @throws std::bad_alloc
    [[nodiscard]] virtual std::vector<EdgeRef> edges(BRepHandle handle) = 0;

    /// @brief Returns all face references for the solid. [Sprint 1.B]
    /// @throws std::bad_alloc
    [[nodiscard]] virtual std::vector<FaceRef> faces(BRepHandle handle) = 0;

    // -------------------------------------------------------------------------
    // Measurement
    // -------------------------------------------------------------------------

    /// @brief Computes solid volume in mm³. [Sprint 1.B]
    [[nodiscard]] virtual GeomResult<double> volume(BRepHandle handle) = 0;

    /// @brief Computes surface area in mm². [Sprint 1.B]
    [[nodiscard]] virtual GeomResult<double> area(BRepHandle handle) = 0;

    /// @brief Computes the axis-aligned bounding box. [Sprint 0.3]
    [[nodiscard]] virtual GeomResult<BoundingBox> bbox(BRepHandle handle) = 0;

    // -------------------------------------------------------------------------
    // Tessellation
    // -------------------------------------------------------------------------

    /// @brief Tessellates a solid into an indexed triangle mesh. [Sprint 0.3]
    ///
    /// @param handle  Solid to tessellate (must be valid).
    /// @param params  Quality parameters controlling mesh density.
    [[nodiscard]] virtual GeomResult<TriangleMesh> tessellate(BRepHandle handle,
                                                              TessellationParams params) = 0;

    // -------------------------------------------------------------------------
    // Transform
    // -------------------------------------------------------------------------

    /// @brief Returns a new solid with the given affine transform applied. [Sprint 1.B]
    [[nodiscard]] virtual GeomResult<BRepHandle> transformed(BRepHandle handle,
                                                             const Transform3D& tx) = 0;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Releases a BRep solid, freeing adapter-owned resources. [Sprint 0.3]
    ///
    /// Calling release() on an invalid or already-released handle is a no-op.
    /// @noexcept-ok
    virtual void release(BRepHandle handle) noexcept = 0;

    /// @brief Releases a Wire, freeing adapter-owned resources. [Sprint 1.A]
    ///
    /// Calling release() on an invalid or already-released handle is a no-op.
    /// @noexcept-ok
    virtual void release(WireHandle handle) noexcept = 0;

protected:
    IGeometryConstructionPort() = default;
};

}  // namespace mycad::domain
