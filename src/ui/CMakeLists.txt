# UI layer — Qt6 widgets, viewport, camera, and application entry point.
#
# AUTOMOC is ON at directory level so Q_OBJECT macros in headers are
# processed automatically (no manual moc_ step required).

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# --- Library: reusable UI components (ViewportWidget, MainWindow, …) ---

add_library(mycad_ui STATIC
    ViewportWidget.cpp
    MainWindow.cpp
)
add_library(mycad::ui ALIAS mycad_ui)

# C++ standard inherited from root CMAKE_CXX_STANDARD (currently 23).

target_include_directories(mycad_ui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(mycad_ui
    PUBLIC
        mycad::application
        mycad::infrastructure
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        Eigen3::Eigen          # Camera.hpp is a public header that uses Eigen types
    PRIVATE
        mycad::compile_options
)

# --- Executable: application entry point ---

qt_add_executable(mycad_app WIN32
    main.cpp
)

target_link_libraries(mycad_app
    PRIVATE
        mycad::ui
        mycad::compile_options
)
