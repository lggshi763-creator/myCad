add_library(mycad_plugin STATIC placeholder.cpp)
add_library(mycad::plugin ALIAS mycad_plugin)

# C++ standard inherited from root CMAKE_CXX_STANDARD (currently 23).
target_include_directories(mycad_plugin PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(mycad_plugin
    PUBLIC  mycad::domain
    PRIVATE mycad::compile_options
)
