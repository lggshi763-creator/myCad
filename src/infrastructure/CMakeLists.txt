# Infrastructure layer — concrete implementations of domain ports (ADR-0004, ADR-0007).
#
# OCCT headers (#include <BRepPrimAPI_MakeBox.hxx> etc.) are ONLY permitted in
# source files under geometry/.  They must never appear in any .hpp exposed via
# the PUBLIC include path.  CI step V6 (grep src/infrastructure/*/include/) verifies this.

add_library(mycad_infrastructure STATIC
    geometry/OcctGeometryAdapter.cpp
    eventsourcing/InMemoryEventStore.cpp
    ecs/EnttRegistry.cpp
    rendering/OpenGLRenderAdapter.cpp
)
add_library(mycad::infrastructure ALIAS mycad_infrastructure)

# C++ standard inherited from root CMAKE_CXX_STANDARD (currently 23).

# Public headers: consumers use #include <mycad/infrastructure/...>
target_include_directories(mycad_infrastructure PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/geometry/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/eventsourcing/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/rendering/include>
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
            EnTT::EnTT    # ECS — EnttRegistry.hpp templates require entt types
            TKernel       # Standard_Failure, basic OCCT framework
            TKMath        # Bnd_Box, Poly_Triangulation
            TKBRep        # BRep_Tool, TopoDS, BRepBndLib, TopLoc_Location
            TKTopAlgo     # TopExp_Explorer
            TKPrim        # BRepPrimAPI_MakeBox
            TKMesh        # BRepMesh_IncrementalMesh
            # Qt6 OpenGL — required by OpenGLRenderAdapter.cpp (ADR-0005):
            Qt6::OpenGL
            Qt6::OpenGLWidgets
)
