#pragma once

#include <cstdint>
#include <functional>
#include <string>

/// @file Aggregate version counter for optimistic concurrency (ADR-0013 §7).

namespace mycad::domain {

/// @brief Monotonically increasing aggregate version (starts at 0).
/// @thread-safe
struct Version {
    std::uint64_t value{0};

    auto operator<=>(const Version&) const = default;

    /// @brief Returns the next version (value + 1).
    /// @noexcept-ok
    [[nodiscard]] constexpr Version next() const noexcept {
        return Version{value + 1};
    }
};

/// @brief Returns "v<N>", e.g. "v0", "v42".
/// @throws std::bad_alloc
[[nodiscard]] std::string to_string(Version v);

}  // namespace mycad::domain

template <>
struct std::hash<mycad::domain::Version> {
    std::size_t operator()(const mycad::domain::Version& v) const noexcept {
        return std::hash<std::uint64_t>{}(v.value);
    }
};
