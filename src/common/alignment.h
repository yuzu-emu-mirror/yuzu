// This file is under the public domain.

#pragma once

#include <cstddef>
#include <type_traits>

namespace Common {

template <typename T>
[[nodiscard]] constexpr T AlignUp(T value, std::size_t size) noexcept {
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned value.");
    return static_cast<T>(value + (size - value % size) % size);
}

template <typename T>
[[nodiscard]] constexpr T AlignDown(T value, std::size_t size) noexcept {
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned value.");
    return static_cast<T>(value - value % size);
}

template <typename T>
[[nodiscard]] constexpr bool Is4KBAligned(T value) noexcept {
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned value.");
    return (value & 0xFFF) == 0;
}

template <typename T>
[[nodiscard]] constexpr bool IsWordAligned(T value) noexcept {
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned value.");
    return (value & 0b11) == 0;
}

} // namespace Common
