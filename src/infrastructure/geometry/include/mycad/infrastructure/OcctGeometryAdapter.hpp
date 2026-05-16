#pragma once

#include <mycad/domain/IGeometryConstructionPort.hpp>

#include <memory>

/// @file OCCT-backed implementation of IGeometryConstructionPort (ADR-0004).
///
/// No OCCT header is included here. All TopoDS / BRep* symbols are confined
/// to OcctGeometryAdapter.cpp (PIMPL pattern).

namespace mycad::infrastructure {

/// @brief Concrete geometry adapter backed by OpenCASCADE Technology 7.9.
///
/// Sprint 0.3 implements: makeBox, tessellate, bbox, release(BRepHandle).
/// All other methods return GeomError{NotImplemented} until their sprint.
///
/// @see IGeometryConstructionPort for the full interface contract.
class OcctGeometryAdapter final : public domain::IGeometryConstructionPort {
public:
    OcctGeometryAdapter();
    ~OcctGeometryAdapter() override;

    // -------------------------------------------------------------------------
    // Sprint 0.3 — fully implemented
    // -------------------------------------------------------------------------

    [[nodiscard]] domain::GeomResult<domain::BRepHandle>
    makeBox(double dx, double dy, double dz) override;

    [[nodiscard]] domain::GeomResult<domain::BoundingBox> bbox(domain::BRepHandle handle) override;

    [[nodiscard]] domain::GeomResult<domain::TriangleMesh>
    tessellate(domain::BRepHandle handle, domain::TessellationParams params) override;

    void release(domain::BRepHandle handle) noexcept override;
    void release(domain::WireHandle handle) noexcept override;

    // -------------------------------------------------------------------------
    // Sprint 1.B and later — stub returns NotImplemented
    // -------------------------------------------------------------------------

    [[nodiscard]] domain::GeomResult<domain::BRepHandle> makeCylinder(double radius,
                                                                      double height) override;

    [[nodiscard]] domain::GeomResult<domain::BRepHandle> makeSphere(double radius) override;

    [[nodiscard]] domain::GeomResult<domain::BRepHandle>
    prismaticExtrude(domain::WireHandle profile, double distance) override;

    [[nodiscard]] domain::GeomResult<domain::BRepHandle> revolve(domain::WireHandle profile,
                                                                 double angleDeg) override;

    [[nodiscard]] domain::GeomResult<domain::BRepHandle>
    booleanUnion(domain::BRepHandle a, domain::BRepHandle b) override;

    [[nodiscard]] domain::GeomResult<domain::BRepHandle>
    booleanCut(domain::BRepHandle base, domain::BRepHandle tool) override;

    [[nodiscard]] domain::GeomResult<domain::BRepHandle> fillet(
        domain::BRepHandle solid, std::span<const domain::EdgeRef> edges, double radius) override;

    [[nodiscard]] domain::GeomResult<domain::BRepHandle> chamfer(
        domain::BRepHandle solid, std::span<const domain::EdgeRef> edges, double distance) override;

    [[nodiscard]] std::vector<domain::EdgeRef> edges(domain::BRepHandle handle) override;

    [[nodiscard]] std::vector<domain::FaceRef> faces(domain::BRepHandle handle) override;

    [[nodiscard]] domain::GeomResult<double> volume(domain::BRepHandle handle) override;

    [[nodiscard]] domain::GeomResult<double> area(domain::BRepHandle handle) override;

    [[nodiscard]] domain::GeomResult<domain::BRepHandle>
    transformed(domain::BRepHandle handle, const domain::Transform3D& tx) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace mycad::infrastructure
