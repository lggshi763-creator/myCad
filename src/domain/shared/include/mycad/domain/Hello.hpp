#pragma once

#include <string>
#include <string_view>

/// @file Minimal domain example — validates CMake + Catch2 linkage (Sprint 0.1).

namespace mycad::domain {

/// @brief Returns "Hello, <name>!" (Unicode-safe).
/// @throws std::bad_alloc
/// @thread-safe
std::string greet(std::string_view name);

}  // namespace mycad::domain
