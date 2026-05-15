#pragma once

/// @file Controls mesh quality for BRep tessellation.

namespace mycad::domain {

/// @brief Quality parameters passed to IGeometryConstructionPort::tessellate().
///
/// Smaller deflection values produce finer meshes at higher vertex counts.
///
/// @si-units{millimeter,radian}
struct TessellationParams {
    /// @brief Maximum chord-height deviation from the true surface (mm).
    ///
    /// Typical values: 0.01 (high quality) … 0.5 (fast preview). Default 0.1.
    double linearDeflection{0.1};

    /// @brief Maximum angular deviation between adjacent triangles (radians).
    ///
    /// Default 0.5 rad ≈ 28.6°.
    double angularDeflection{0.5};

    /// @brief When true, linearDeflection is interpreted as a fraction of
    ///        the bounding-box diagonal rather than an absolute mm value.
    bool relative{false};
};

}  // namespace mycad::domain
