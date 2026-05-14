#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>

/// @file Aggregate root identity (16-byte UUID placeholder, ADR-0013 §7).

namespace mycad::domain {

/// @brief Opaque 16-byte aggregate identifier (UUID layout).
/// @thread-safe
struct AggregateId {
    std::array<std::uint8_t, 16> bytes{};

    auto operator<=>(const AggregateId&) const = default;

    /// @brief Returns a zero-filled placeholder AggregateId.
    /// @noexcept-ok
    [[nodiscard]] static AggregateId generate() noexcept {
        return AggregateId{};
    }
};

/// @brief Returns the standard "8-4-4-4-12" hex string.
/// @throws std::bad_alloc
[[nodiscard]] std::string to_string(AggregateId id);

}  // namespace mycad::domain

template <>
struct std::hash<mycad::domain::AggregateId> {
    std::size_t operator()(const mycad::domain::AggregateId& id) const noexcept {
        std::size_t h = 14695981039346656037ULL;
        for (auto b : id.bytes) {
            h ^= static_cast<std::size_t>(b);
            h *= 1099511628211ULL;
        }
        return h;
    }
};
