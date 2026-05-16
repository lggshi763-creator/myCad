#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

#include <Eigen/Dense>

/// @file Camera — spherical orbit camera for the 3-D viewport.

namespace mycad::ui {

/// @brief Spherical orbit camera that rotates around a fixed target point.
///
/// The camera is described in spherical coordinates (azimuth, elevation, radius)
/// relative to a world-space target. viewMatrix() and projMatrix() return
/// column-major 4×4 matrices suitable for direct upload via glUniformMatrix4fv.
///
/// Coordinate convention: right-handed, Y-up, camera looks in the -Z direction.
struct Camera {
    float azimuth{0.f};                     ///< Horizontal angle (radians, 0 = +Z).
    float elevation{0.4f};                  ///< Vertical angle above horizon (radians).
    float radius{8.f};                      ///< Distance from target (world units).
    float fovY{45.f};                       ///< Vertical field-of-view (degrees).
    Eigen::Vector3f target{0.f, 0.f, 0.f};  ///< World-space look-at point.

    /// @brief Returns the eye position derived from the spherical parameters.
    /// @noexcept-ok
    [[nodiscard]] Eigen::Vector3f eye() const noexcept {
        const float cosEl = std::cos(elevation);
        return target + Eigen::Vector3f{radius * cosEl * std::sin(azimuth),
                                        radius * std::sin(elevation),
                                        radius * cosEl * std::cos(azimuth)};
    }

    /// @brief Returns a column-major 4×4 view matrix (world → camera transform).
    ///
    /// Compatible with glUniformMatrix4fv(loc, 1, GL_FALSE, m.data()).
    /// @noexcept-ok
    [[nodiscard]] Eigen::Matrix4f viewMatrix() const noexcept {
        const Eigen::Vector3f e = eye();
        // forward = eye − target (camera looks in −Z in right-handed space)
        const Eigen::Vector3f fwd = (e - target).normalized();
        const Eigen::Vector3f rgt = Eigen::Vector3f::UnitY().cross(fwd).normalized();
        const Eigen::Vector3f up = fwd.cross(rgt);

        Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
        // Row 0 — right
        m(0, 0) = rgt.x();
        m(0, 1) = rgt.y();
        m(0, 2) = rgt.z();
        m(0, 3) = -rgt.dot(e);
        // Row 1 — up
        m(1, 0) = up.x();
        m(1, 1) = up.y();
        m(1, 2) = up.z();
        m(1, 3) = -up.dot(e);
        // Row 2 — forward (+Z in camera = away from viewer)
        m(2, 0) = fwd.x();
        m(2, 1) = fwd.y();
        m(2, 2) = fwd.z();
        m(2, 3) = -fwd.dot(e);
        // Row 3 — homogeneous
        m(3, 0) = 0.f;
        m(3, 1) = 0.f;
        m(3, 2) = 0.f;
        m(3, 3) = 1.f;
        return m;
    }

    /// @brief Returns a column-major 4×4 perspective projection matrix.
    ///
    /// Maps to OpenGL clip space (NDC depth in [-1, 1]).
    ///
    /// @param aspect  Viewport width / height.
    /// @noexcept-ok
    [[nodiscard]] Eigen::Matrix4f projMatrix(float aspect) const noexcept {
        constexpr float kNear = 0.1f;
        constexpr float kFar = 1000.f;

        const float fovRad = fovY * std::numbers::pi_v<float> / 180.f;
        const float tanHalf = std::tan(fovRad / 2.f);

        Eigen::Matrix4f m = Eigen::Matrix4f::Zero();
        m(0, 0) = 1.f / (aspect * tanHalf);
        m(1, 1) = 1.f / tanHalf;
        m(2, 2) = -(kFar + kNear) / (kFar - kNear);
        m(2, 3) = -(2.f * kFar * kNear) / (kFar - kNear);
        m(3, 2) = -1.f;
        return m;
    }

    /// @brief Orbits the camera horizontally and vertically.
    ///
    /// @param dAz   Azimuth delta (radians).
    /// @param dEl   Elevation delta (radians); clamped to ±85°.
    /// @noexcept-ok
    void orbit(float dAz, float dEl) noexcept {
        constexpr float kMaxEl = 1.48353f;  // ~85°
        azimuth += dAz;
        elevation = std::clamp(elevation + dEl, -kMaxEl, kMaxEl);
    }

    /// @brief Adjusts the orbit radius multiplicatively.
    ///
    /// @param delta  Positive = zoom in, negative = zoom out.
    /// @noexcept-ok
    void zoom(float delta) noexcept {
        constexpr float kMinRadius = 0.5f;
        radius = std::max(kMinRadius, radius * (1.f - delta * 0.1f));
    }
};

}  // namespace mycad::ui
