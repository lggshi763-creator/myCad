add_library(mycad_drawing STATIC placeholder.cpp)
add_library(mycad::drawing ALIAS mycad_drawing)

# C++ standard inherited from root CMAKE_CXX_STANDARD (currently 23).
target_include_directories(mycad_drawing PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/shared/include>
)

# ADR-0010: drawing is a sibling sub-module of the domain layer.
# It depends on mycad::domain (shared value objects, identity types,
# DomainEvent base) but is independently versioned and event-streamed.
target_link_libraries(mycad_drawing
    PUBLIC  mycad::domain
    PRIVATE mycad::compile_options
)
