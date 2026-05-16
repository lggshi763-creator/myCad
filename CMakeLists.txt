cmake_minimum_required(VERSION 3.25)

project(myCad
    VERSION 0.0.1
    DESCRIPTION "Open-source parametric CAD"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# CMake 4.x feature-table workaround for MSVC 19.50+ (Visual Studio 2026).
#
# CMake's built-in Compiler/MSVC-CXX.cmake feature tables only cover versions
# up to ~19.44 (VS 2022 17.14). When a vcpkg package (e.g. EnTT) exports a
# transitive compile-feature requirement (cxx_std_17), CMake's Generate step
# fails with "No known features for CXX compiler MSVC 19.50" even though the
# compiler obviously supports it. Seeding CMAKE_CXX_COMPILE_FEATURES with the
# meta-feature names silences the lookup without changing compiler flags (the
# actual /std:c++23 flag comes from CMAKE_CXX_STANDARD above).
# ---------------------------------------------------------------------------
if(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.50")
    set(CMAKE_CXX_COMPILE_FEATURES
        cxx_std_98 cxx_std_11 cxx_std_14 cxx_std_17 cxx_std_20 cxx_std_23
        cxx_alias_templates cxx_alignas cxx_alignof cxx_attributes
        cxx_auto_type cxx_constexpr cxx_decltype cxx_defaulted_functions
        cxx_delegating_constructors cxx_deleted_functions
        cxx_final cxx_generic_lambdas cxx_inheriting_constructors
        cxx_lambdas cxx_long_long_type cxx_noexcept cxx_nullptr
        cxx_override cxx_range_for cxx_raw_string_literals
        cxx_right_angle_brackets cxx_rvalue_references
        cxx_sizeof_member cxx_static_assert cxx_strong_enums
        cxx_trailing_return_types cxx_uniform_initialization
        cxx_user_literals cxx_variadic_macros cxx_variadic_templates
    )
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif()

option(MYCAD_ENABLE_TESTING    "Build unit tests (Catch2)"     ON)
option(MYCAD_ENABLE_BENCHMARKS "Build benchmarks (gbench)"     OFF)
option(MYCAD_ENABLE_ASAN       "Enable AddressSanitizer"       OFF)
option(MYCAD_ENABLE_UBSAN      "Enable UndefinedBehaviorSan"   OFF)
option(MYCAD_WARNINGS_AS_ERRORS "Treat warnings as errors"     OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_library(mycad_compile_options INTERFACE)
add_library(mycad::compile_options ALIAS mycad_compile_options)

if(MSVC)
    target_compile_options(mycad_compile_options INTERFACE
        /utf-8
        /W4
        /permissive-
        /Zc:__cplusplus
        /Zc:preprocessor
        /EHsc
        /MP
    )
    target_compile_definitions(mycad_compile_options INTERFACE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MYCAD_WARNINGS_AS_ERRORS)
        target_compile_options(mycad_compile_options INTERFACE /WX)
    endif()
else()
    target_compile_options(mycad_compile_options INTERFACE
        -Wall -Wextra -Wpedantic
        -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual
        -Wdouble-promotion -Wformat=2
    )
    if(MYCAD_WARNINGS_AS_ERRORS)
        target_compile_options(mycad_compile_options INTERFACE -Werror)
    endif()
endif()

if(MYCAD_ENABLE_ASAN)
    if(MSVC)
        target_compile_options(mycad_compile_options INTERFACE /fsanitize=address)
    else()
        target_compile_options(mycad_compile_options INTERFACE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(mycad_compile_options INTERFACE -fsanitize=address)
    endif()
endif()

if(MYCAD_ENABLE_UBSAN)
    if(NOT MSVC)
        target_compile_options(mycad_compile_options INTERFACE -fsanitize=undefined -fno-omit-frame-pointer)
        target_link_options(mycad_compile_options INTERFACE -fsanitize=undefined)
    else()
        message(WARNING "UBSan is not supported on MSVC; skipping.")
    endif()
endif()

find_package(fmt           CONFIG REQUIRED)
find_package(spdlog        CONFIG REQUIRED)
find_package(EnTT          CONFIG REQUIRED)
find_package(Eigen3        CONFIG REQUIRED)
find_package(tl-expected   CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tomlplusplus  CONFIG REQUIRED)
find_package(zstd          CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(flatbuffers   CONFIG REQUIRED)
find_package(OpenCASCADE   CONFIG REQUIRED)

find_package(Qt6 6.5 CONFIG REQUIRED COMPONENTS
    Core Gui Widgets OpenGL OpenGLWidgets LinguistTools
)

add_subdirectory(src)

if(MYCAD_ENABLE_TESTING)
    enable_testing()
    find_package(Catch2 3 CONFIG REQUIRED)
    include(CTest)
    include(Catch)
    add_subdirectory(tests)
endif()

if(MYCAD_ENABLE_BENCHMARKS)
    find_package(benchmark CONFIG REQUIRED)
    add_subdirectory(benchmarks)
endif()

# ---------------------------------------------------------------------------
# Documentation (Doxygen + Sphinx + Breathe)
#
# Optional. Detects tools at configure time; defines `docs` target only if
# everything's available. Run with:
#     cmake --build <build-dir> --target docs
# Output:
#     <source>/build/docs/doxygen/{html,xml}    (Doxygen)
#     <source>/build/docs/sphinx                (Sphinx, consumes XML)
# ---------------------------------------------------------------------------
find_package(Doxygen QUIET)
find_package(Python3 COMPONENTS Interpreter QUIET)

set(MYCAD_DOCS_AVAILABLE OFF)
set(MYCAD_DOCS_REASON    "")
if(NOT DOXYGEN_FOUND)
    set(MYCAD_DOCS_REASON "Doxygen not found")
elseif(NOT Python3_FOUND)
    set(MYCAD_DOCS_REASON "Python3 interpreter not found")
else()
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import sphinx; import breathe"
        RESULT_VARIABLE _mycad_sphinx_check
        OUTPUT_QUIET ERROR_QUIET
    )
    if(_mycad_sphinx_check EQUAL 0)
        set(MYCAD_DOCS_AVAILABLE ON)
    else()
        set(MYCAD_DOCS_REASON
            "python -m sphinx / breathe import failed (pip install sphinx breathe)")
    endif()
endif()

if(MYCAD_DOCS_AVAILABLE)
    set(_mycad_doxygen_out "${PROJECT_SOURCE_DIR}/build/docs/doxygen")
    set(_mycad_sphinx_out  "${PROJECT_SOURCE_DIR}/build/docs/sphinx")
    set(_mycad_sphinx_src  "${PROJECT_SOURCE_DIR}/docs/api-reference")

    add_custom_target(docs-doxygen
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_mycad_doxygen_out}"
        COMMAND "${DOXYGEN_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/Doxyfile"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "[docs] Doxygen -> ${_mycad_doxygen_out}/{html,xml}"
        VERBATIM
    )

    add_custom_target(docs-sphinx
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_mycad_sphinx_out}"
        COMMAND "${CMAKE_COMMAND}" -E env
                "MYCAD_DOXYGEN_XML=${_mycad_doxygen_out}/xml"
                "${Python3_EXECUTABLE}" -m sphinx
                    -b html --keep-going
                    "${_mycad_sphinx_src}" "${_mycad_sphinx_out}"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        DEPENDS docs-doxygen
        COMMENT "[docs] Sphinx -> ${_mycad_sphinx_out}"
        VERBATIM
    )

    add_custom_target(docs DEPENDS docs-sphinx)

    message(STATUS "Docs target: enabled (target name 'docs')")
else()
    message(STATUS "Docs target: disabled (${MYCAD_DOCS_REASON})")
endif()

message(STATUS "myCad ${PROJECT_VERSION}  C++${CMAKE_CXX_STANDARD}  Build=${CMAKE_BUILD_TYPE}")
message(STATUS "  Tests=${MYCAD_ENABLE_TESTING}  Benchmarks=${MYCAD_ENABLE_BENCHMARKS}")
message(STATUS "  ASan=${MYCAD_ENABLE_ASAN}  UBSan=${MYCAD_ENABLE_UBSAN}")
message(STATUS "  Docs=${MYCAD_DOCS_AVAILABLE}")
