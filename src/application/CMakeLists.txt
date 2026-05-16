# Application layer — CommandBus + command handlers (DDD application layer).
#
# This layer may #include infrastructure headers (adapters are injected via
# constructor); it must never #include Qt, OpenGL, or OCCT headers directly.

add_library(mycad_application STATIC
    CommandBus.cpp
)
add_library(mycad::application ALIAS mycad_application)

# C++ standard inherited from root CMAKE_CXX_STANDARD (currently 23).

# Public headers: consumers use #include <mycad/application/...>
target_include_directories(mycad_application PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(mycad_application
    PUBLIC  mycad::domain
    PRIVATE mycad::compile_options
)
