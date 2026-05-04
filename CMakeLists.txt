cmake_minimum_required(VERSION 3.25)

project(myCad
    VERSION 0.0.1
    DESCRIPTION "Open-source parametric CAD"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
endif()

message(STATUS "myCad ${PROJECT_VERSION}  C++${CMAKE_CXX_STANDARD}  Build=${CMAKE_BUILD_TYPE}")
message(STATUS "  Tests=${MYCAD_ENABLE_TESTING}  Benchmarks=${MYCAD_ENABLE_BENCHMARKS}")
message(STATUS "  ASan=${MYCAD_ENABLE_ASAN}  UBSan=${MYCAD_ENABLE_UBSAN}")
