add_library(mycad_domain STATIC
    shared/Hello.cpp
    shared/Point2D.cpp
    shared/Point3D.cpp
    shared/Vector3D.cpp
    shared/Axis3D.cpp
    shared/BoundingBox.cpp
    shared/Transform3D.cpp
    shared/EventId.cpp
    shared/AggregateId.cpp
    shared/SketchId.cpp
    shared/Version.cpp
)
add_library(mycad::domain ALIAS mycad_domain)

target_compile_features(mycad_domain PUBLIC cxx_std_20)

# Public headers live under shared/include/mycad/domain/ so consumers
# always include with the project namespace prefix:
#     #include <mycad/domain/Hello.hpp>
# As more sub-modules (sketch, feature, assembly, constraint) land, each
# brings its own <module>/include/mycad/domain/ tree appended here.
target_include_directories(mycad_domain PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/shared/include>
)

# Domain layer is constrained to zero external dependencies (ADR-0002).
# Only mycad::compile_options (warnings/sanitizer toggles) is allowed.
target_link_libraries(mycad_domain PRIVATE mycad::compile_options)

# ---------------------------------------------------------------------------
# Drawing sub-module — sibling bounded context within the domain layer.
# Depends on mycad::domain (shared types, events). See ADR-0010.
# ---------------------------------------------------------------------------
add_subdirectory(drawing)
