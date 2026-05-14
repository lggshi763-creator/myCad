#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>

/// @file Sketch aggregate identity (16-byte UUID placeholder, ADR-0013 §7).

namespace mycad::domain {

/// @brief Opaque 16-byte sketch identifier (UUID layout).
/// @thread-safe
struct SketchId {
    std::array<std::uint8_t, 16> bytes{};

    auto operator<=>(const SketchId&) const = default;

    /// @brief Returns a zero-filled placeholder SketchId.
    /// @noexcept-ok
    [[nodiscard]] static SketchId generate() noexcept {
        return SketchId{};
    }
};

/// @brief Returns the standard "8-4-4-4-12" hex string.
/// @throws std::bad_alloc
[[nodiscard]] std::string to_string(SketchId id);

}  // namespace mycad::domain

template <>
struct std::hash<mycad::domain::SketchId> {
    std::size_t operator()(const mycad::domain::SketchId& id) const noexcept {
        std::size_t h = 14695981039346656037ULL;
        for (auto b : id.bytes) {
            h ^= static_cast<std::size_t>(b);
            h *= 1099511628211ULL;
        }
        return h;
    }
};
