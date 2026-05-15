#pragma once

#include <expected>
#include <string>

/// @file Geometry operation error type and monadic result alias (C++23 std::expected).
///
/// Uses std::expected (C++23). Domain layer requires cxx_std_23 in CMakeLists.

namespace mycad::domain {

/// @brief Classifies geometry operation failure modes.
enum class GeomErrorKind {
    InvalidInput,        ///< Caller-supplied parameters are out of range or inconsistent.
    AlgorithmFailed,     ///< Underlying algorithm (e.g. OCCT) reported a failure.
    DegenerateGeometry,  ///< Input produces zero-area / zero-volume / zero-length output.
    BooleanFailure,      ///< Boolean operation could not be completed.
    NotImplemented,      ///< Adapter method not yet implemented in the current sprint.
    OutOfMemory,         ///< Memory allocation failed.
    Unknown,             ///< Catch-all for unexpected failures.
};

/// @brief Carries geometry operation error details.
struct GeomError {
    GeomErrorKind kind{GeomErrorKind::Unknown};
    std::string message;  ///< Human-readable description (English, no newlines).
};

/// @brief Monadic return type for geometry operations.
///
/// OK path holds T; error path holds GeomError.
///
/// Example:
/// @code
///   GeomResult<BRepHandle> h = adapter->makeBox(10, 10, 10);
///   if (!h) { log(h.error().message); return; }
///   adapter->tessellate(*h, {});
/// @endcode
template <typename T>
using GeomResult = std::expected<T, GeomError>;

}  // namespace mycad::domain
