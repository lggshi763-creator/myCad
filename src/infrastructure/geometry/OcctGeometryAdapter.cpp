/// @file OcctGeometryAdapter.cpp
///
/// OCCT headers are ONLY included in this translation unit (ADR-0004 PIMPL).
/// No OCCT symbol may leak into OcctGeometryAdapter.hpp.

#include <mycad/infrastructure/OcctGeometryAdapter.hpp>

// --- OCCT includes (confined to this .cpp) ----------------------------------
#include <BRepBndLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
// ----------------------------------------------------------------------------

#include <mycad/domain/BoundingBox.hpp>
#include <mycad/domain/Point3D.hpp>

#include <algorithm>
#include <string>
#include <unordered_map>

namespace mycad::infrastructure {

// ---------------------------------------------------------------------------
// PIMPL implementation record
// ---------------------------------------------------------------------------

struct OcctGeometryAdapter::Impl {
    /// Shapes owned by this adapter, keyed by the opaque BRepHandle id.
    std::unordered_map<uint64_t, TopoDS_Shape> shapes;

    /// Monotonically increasing id counter.  Starts at 1 so that id==0 stays
    /// the "invalid" sentinel (matches BRepHandle::valid() == id != 0).
    uint64_t nextId{1};
};

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

OcctGeometryAdapter::OcctGeometryAdapter() : impl_(std::make_unique<Impl>()) {}

OcctGeometryAdapter::~OcctGeometryAdapter() = default;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Builds a NotImplemented error result with a short description.
template <typename T>
domain::GeomResult<T> notImpl(const char* method) {
    return std::unexpected(
        domain::GeomError{domain::GeomErrorKind::NotImplemented,
                          std::string(method) + ": not implemented (see sprint tag)"});
}

}  // namespace

// ---------------------------------------------------------------------------
// Sprint 0.3 — makeBox
// ---------------------------------------------------------------------------

domain::GeomResult<domain::BRepHandle>
OcctGeometryAdapter::makeBox(double dx, double dy, double dz) {
    if (dx <= 0.0 || dy <= 0.0 || dz <= 0.0) {
        return std::unexpected(domain::GeomError{domain::GeomErrorKind::InvalidInput,
                                                 "makeBox: dx, dy, dz must all be > 0"});
    }
    try {
        BRepPrimAPI_MakeBox builder(dx, dy, dz);
        builder.Build();
        if (!builder.IsDone()) {
            return std::unexpected(
                domain::GeomError{domain::GeomErrorKind::AlgorithmFailed,
                                  "makeBox: BRepPrimAPI_MakeBox reported failure"});
        }
        const uint64_t id = impl_->nextId++;
        impl_->shapes.emplace(id, builder.Shape());
        return domain::BRepHandle{id};
    } catch (const Standard_Failure& e) {
        return std::unexpected(domain::GeomError{domain::GeomErrorKind::AlgorithmFailed,
                                                 std::string(e.GetMessageString())});
    }
}

// ---------------------------------------------------------------------------
// Sprint 0.3 — tessellate
// ---------------------------------------------------------------------------

domain::GeomResult<domain::TriangleMesh>
OcctGeometryAdapter::tessellate(domain::BRepHandle handle, domain::TessellationParams params) {
    const auto it = impl_->shapes.find(handle.id);
    if (it == impl_->shapes.end()) {
        return std::unexpected(
            domain::GeomError{domain::GeomErrorKind::InvalidInput, "tessellate: unknown handle"});
    }

    try {
        // Mesh the shape into the OCCT internal representation.
        // The BRepMesh_IncrementalMesh constructor performs meshing automatically.
        BRepMesh_IncrementalMesh mesher(it->second,
                                        params.linearDeflection,
                                        static_cast<Standard_Boolean>(params.relative),
                                        params.angularDeflection);
        (void)mesher;  // meshing is triggered by constructor; result stored in shape

        domain::TriangleMesh mesh;

        // Iterate over every face of the solid and collect triangles.
        for (TopExp_Explorer exp(it->second, TopAbs_FACE); exp.More(); exp.Next()) {
            const TopoDS_Face face = TopoDS::Face(exp.Current());
            TopLoc_Location loc;
            const Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
            if (tri.IsNull() || tri->NbTriangles() == 0)
                continue;

            // Apply the face's local-to-world transform (identity for simple primitives).
            const gp_Trsf trsf = loc;  // implicit TopLoc_Location -> gp_Trsf conversion

            // Base vertex index for this face's contribution.
            const auto vertBase = static_cast<uint32_t>(mesh.vertexCount());

            // Append vertices.
            for (Standard_Integer i = 1; i <= tri->NbNodes(); ++i) {
                const gp_Pnt p = tri->Node(i).Transformed(trsf);
                mesh.vertices.push_back(static_cast<float>(p.X()));
                mesh.vertices.push_back(static_cast<float>(p.Y()));
                mesh.vertices.push_back(static_cast<float>(p.Z()));
            }

            // Append triangle indices.
            // Poly_Triangulation uses 1-based indices; convert to 0-based.
            // For REVERSED faces, swap n1/n2 to maintain consistent outward normals.
            const bool reversed = (face.Orientation() == TopAbs_REVERSED);
            for (Standard_Integer i = 1; i <= tri->NbTriangles(); ++i) {
                Standard_Integer n1 = 0, n2 = 0, n3 = 0;
                tri->Triangle(i).Get(n1, n2, n3);
                if (reversed)
                    std::swap(n1, n2);
                mesh.indices.push_back(vertBase + static_cast<uint32_t>(n1 - 1));
                mesh.indices.push_back(vertBase + static_cast<uint32_t>(n2 - 1));
                mesh.indices.push_back(vertBase + static_cast<uint32_t>(n3 - 1));
            }
        }

        return mesh;
    } catch (const Standard_Failure& e) {
        return std::unexpected(domain::GeomError{domain::GeomErrorKind::AlgorithmFailed,
                                                 std::string(e.GetMessageString())});
    }
}

// ---------------------------------------------------------------------------
// Sprint 0.3 — bbox
// ---------------------------------------------------------------------------

domain::GeomResult<domain::BoundingBox> OcctGeometryAdapter::bbox(domain::BRepHandle handle) {
    const auto it = impl_->shapes.find(handle.id);
    if (it == impl_->shapes.end()) {
        return std::unexpected(
            domain::GeomError{domain::GeomErrorKind::InvalidInput, "bbox: unknown handle"});
    }
    try {
        Bnd_Box box;
        BRepBndLib::Add(it->second, box);
        if (box.IsVoid()) {
            return std::unexpected(domain::GeomError{domain::GeomErrorKind::DegenerateGeometry,
                                                     "bbox: shape has no geometry"});
        }
        Standard_Real xMin{}, yMin{}, zMin{}, xMax{}, yMax{}, zMax{};
        box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        return domain::BoundingBox{domain::Point3D{xMin, yMin, zMin},
                                   domain::Point3D{xMax, yMax, zMax}};
    } catch (const Standard_Failure& e) {
        return std::unexpected(domain::GeomError{domain::GeomErrorKind::AlgorithmFailed,
                                                 std::string(e.GetMessageString())});
    }
}

// ---------------------------------------------------------------------------
// Sprint 0.3 — release
// ---------------------------------------------------------------------------

void OcctGeometryAdapter::release(domain::BRepHandle handle) noexcept {
    impl_->shapes.erase(handle.id);
}

void OcctGeometryAdapter::release(domain::WireHandle /*handle*/) noexcept {
    // Wires are introduced in Sprint 1.A; no-op for now.
}

// ---------------------------------------------------------------------------
// Not-yet-implemented stubs (Sprint 1.B / later)
// ---------------------------------------------------------------------------

domain::GeomResult<domain::BRepHandle> OcctGeometryAdapter::makeCylinder(double /*radius*/,
                                                                         double /*height*/) {
    return notImpl<domain::BRepHandle>("makeCylinder [Sprint 1.B]");
}

domain::GeomResult<domain::BRepHandle> OcctGeometryAdapter::makeSphere(double /*radius*/) {
    return notImpl<domain::BRepHandle>("makeSphere [Sprint 1.B]");
}

domain::GeomResult<domain::BRepHandle>
OcctGeometryAdapter::prismaticExtrude(domain::WireHandle /*profile*/, double /*distance*/) {
    return notImpl<domain::BRepHandle>("prismaticExtrude [Sprint 1.B]");
}

domain::GeomResult<domain::BRepHandle> OcctGeometryAdapter::revolve(domain::WireHandle /*profile*/,
                                                                    double /*angleDeg*/) {
    return notImpl<domain::BRepHandle>("revolve [Sprint 1.B]");
}

domain::GeomResult<domain::BRepHandle> OcctGeometryAdapter::booleanUnion(domain::BRepHandle /*a*/,
                                                                         domain::BRepHandle /*b*/) {
    return notImpl<domain::BRepHandle>("booleanUnion [Sprint 1.B]");
}

domain::GeomResult<domain::BRepHandle>
OcctGeometryAdapter::booleanCut(domain::BRepHandle /*base*/, domain::BRepHandle /*tool*/) {
    return notImpl<domain::BRepHandle>("booleanCut [Sprint 1.B]");
}

domain::GeomResult<domain::BRepHandle> OcctGeometryAdapter::fillet(
    domain::BRepHandle /*solid*/, std::span<const domain::EdgeRef> /*edges*/, double /*radius*/) {
    return notImpl<domain::BRepHandle>("fillet [Sprint 1.B]");
}

domain::GeomResult<domain::BRepHandle> OcctGeometryAdapter::chamfer(
    domain::BRepHandle /*solid*/, std::span<const domain::EdgeRef> /*edges*/, double /*distance*/) {
    return notImpl<domain::BRepHandle>("chamfer [Sprint 1.B]");
}

std::vector<domain::EdgeRef> OcctGeometryAdapter::edges(domain::BRepHandle /*handle*/) {
    return {};
}

std::vector<domain::FaceRef> OcctGeometryAdapter::faces(domain::BRepHandle /*handle*/) {
    return {};
}

domain::GeomResult<double> OcctGeometryAdapter::volume(domain::BRepHandle /*handle*/) {
    return notImpl<double>("volume [Sprint 1.B]");
}

domain::GeomResult<double> OcctGeometryAdapter::area(domain::BRepHandle /*handle*/) {
    return notImpl<double>("area [Sprint 1.B]");
}

domain::GeomResult<domain::BRepHandle>
OcctGeometryAdapter::transformed(domain::BRepHandle /*handle*/, const domain::Transform3D& /*tx*/) {
    return notImpl<domain::BRepHandle>("transformed [Sprint 1.B]");
}

}  // namespace mycad::infrastructure
