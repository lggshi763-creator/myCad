# Infrastructure layer — concrete implementations of domain ports (ADR-0004, ADR-0007).
#
# OCCT headers (#include <BRepPrimAPI_MakeBox.hxx> etc.) are ONLY permitted in
# source files under geometry/.  They must never appear in any .hpp exposed via
# the PUBLIC include path.  CI step V6 (grep src/infrastructure/*/include/) verifies this.

add_library(mycad_infrastructure STATIC
    geometry/OcctGeometryAdapter.cpp
)
add_library(mycad::infrastructure ALIAS mycad_infrastructure)

target_compile_features(mycad_infrastructure PUBLIC cxx_std_23)

# Public headers: consumers use #include <mycad/infrastructure/...>
target_include_directories(mycad_infrastructure PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/geometry/include>
)

# Private OCCT include path — not exposed to consumers.
# All OCCT headers live under the vcpkg-installed opencascade/ sub-directory.
target_include_directories(mycad_infrastructure PRIVATE
    $<BUILD_INTERFACE:${OpenCASCADE_INCLUDE_DIR}>
)

target_link_libraries(mycad_infrastructure
    PUBLIC  mycad::domain
    PRIVATE mycad::compile_options
            # OCCT toolkits required by OcctGeometryAdapter (ADR-0004):
            TKernel       # Standard_Failure, basic OCCT framework
            TKMath        # Bnd_Box, Poly_Triangulation
            TKBRep        # BRep_Tool, TopoDS, BRepBndLib, TopLoc_Location
            TKTopAlgo     # TopExp_Explorer
            TKPrim        # BRepPrimAPI_MakeBox
            TKMesh        # BRepMesh_IncrementalMesh
)
