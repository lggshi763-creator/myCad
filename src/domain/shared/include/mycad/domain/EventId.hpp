#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>

/// @file Domain event identity (16-byte UUID placeholder, ADR-0013 §7).

namespace mycad::domain {

/// @brief Opaque 16-byte event identifier (UUID layout).
///
/// generate() returns all-zero in Phase 0; a real UUID source is wired in Phase 1.
/// @thread-safe
struct EventId {
    std::array<std::uint8_t, 16> bytes{};

    auto operator<=>(const EventId&) const = default;

    /// @brief Returns a zero-filled placeholder EventId.
    /// @noexcept-ok
    [[nodiscard]] static EventId generate() noexcept {
        return EventId{};
    }
};

/// @brief Returns the standard "8-4-4-4-12" hex string, e.g. "00000000-0000-...".
/// @throws std::bad_alloc
[[nodiscard]] std::string to_string(EventId id);

}  // namespace mycad::domain

template <>
struct std::hash<mycad::domain::EventId> {
    std::size_t operator()(const mycad::domain::EventId& id) const noexcept {
        // FNV-1a over 16 bytes
        std::size_t h = 14695981039346656037ULL;
        for (auto b : id.bytes) {
            h ^= static_cast<std::size_t>(b);
            h *= 1099511628211ULL;
        }
        return h;
    }
};
