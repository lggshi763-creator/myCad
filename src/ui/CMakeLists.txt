add_library(mycad_ui STATIC placeholder.cpp)
add_library(mycad::ui ALIAS mycad_ui)

target_compile_features(mycad_ui PUBLIC cxx_std_20)
target_include_directories(mycad_ui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(mycad_ui
    PUBLIC
        mycad::application
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
    PRIVATE
        mycad::compile_options
)
