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
    # Headers with Q_OBJECT must be listed explicitly so AUTOMOC
    # processes them regardless of include style (<mycad/ui/...>).
    include/mycad/ui/ViewportWidget.hpp
    include/mycad/ui/MainWindow.hpp
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

# Deploy Qt runtime (DLLs + platform plugins) next to the executable after
# every build.  Two steps:
#
# Step 1 — windeployqt6 (non-fatal): copies most Qt DLLs.  Runs via PowerShell
#   with "; exit 0" so a vcpkg debug/release layout mismatch does NOT abort the
#   build.
#
# Step 2 — cmake -E copy (guaranteed): copies the platforms plugin directly
#   from the vcpkg tree so the app can always start, even when windeployqt
#   partially fails.  The source path is config-aware (Debug vs Release).
#
# Only runs on Windows; on Linux/macOS the OS dynamic linker handles this.
if(WIN32)
    # Qt6_DIR = …/vcpkg_installed/x64-windows/share/Qt6
    # Go up 2 levels to reach …/vcpkg_installed/x64-windows
    set(_qt_arch_root "${Qt6_DIR}/../..")
    cmake_path(NORMAL_PATH _qt_arch_root)

    # -- Step 1: windeployqt (non-fatal) ------------------------------------
    set(_wdqt "${_qt_arch_root}/tools/Qt6/bin/windeployqt6.exe")
    cmake_path(NORMAL_PATH _wdqt)
    if(EXISTS "${_wdqt}")
        add_custom_command(TARGET mycad_app POST_BUILD
            COMMAND powershell -NoProfile -NonInteractive -Command
                    "& '${_wdqt}' '--no-compiler-runtime' '--no-translations' '$<TARGET_FILE:mycad_app>'; exit 0"
            COMMENT "Deploying Qt runtime for mycad_app (non-fatal)..."
            VERBATIM
        )
    else()
        message(STATUS
            "windeployqt6 not found at ${_wdqt} — run it manually after build "
            "to deploy Qt platform plugins.")
    endif()

    # -- Step 2: guaranteed platforms plugin copy ---------------------------
    # vcpkg layout:  debug → …/debug/Qt6/plugins/platforms/qwindowsd.dll
    #                release → …/Qt6/plugins/platforms/qwindows.dll
    set(_vcpkg_root "${_qt_arch_root}")
    cmake_path(NORMAL_PATH _vcpkg_root)

    set(_plat_debug   "${_vcpkg_root}/debug/Qt6/plugins/platforms/qwindowsd.dll")
    set(_plat_release "${_vcpkg_root}/Qt6/plugins/platforms/qwindows.dll")

    # Use generator expression to pick the right DLL at build time.
    add_custom_command(TARGET mycad_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:mycad_app>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<IF:$<CONFIG:Debug>,${_plat_debug},${_plat_release}>"
                "$<TARGET_FILE_DIR:mycad_app>/platforms/$<IF:$<CONFIG:Debug>,qwindowsd.dll,qwindows.dll>"
        COMMENT "Copying Qt platforms plugin..."
        VERBATIM
    )
endif()
