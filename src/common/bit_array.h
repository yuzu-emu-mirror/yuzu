// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <climits>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <type_traits>

#include "common/alignment.h"
#include "common/assert.h"

namespace Common {

/// Abstracts bits packed in an integer (or array of integers)
template <typename T, std::size_t total_bits = sizeof(T) * CHAR_BIT>
class BitArray {
public:
    BitArray() = default;

    /// Initializes a bit array
    constexpr BitArray(std::initializer_list<bool> list) {
        std::size_t i = 0;
        for (const bool element : list) {
            TurnBit(i++, element);
        }
    }

    /// Initializes a bit array setting all bits on or off depending on the argument
    constexpr explicit BitArray(bool defaults) {
        if (defaults) {
            TurnOnAll();
        }
    }

    /// Gets a bit value
    constexpr bool operator[](std::size_t index) const {
        return ((GetTypeValue(index) >> GetSubOffset(index)) & 1) != 0;
    }

    /// Gets a bit value testing for out of bound reads
    bool at(std::size_t index) const {
        ASSERT(index < size());
        return operator[](index);
    }

    /// Turn on a bit
    void TurnOn(std::size_t index) {
        GetTypeValue(index) |= 1 << GetSubOffset(index);
    }

    /// Turn off a bit
    void TurnOff(std::size_t index) {
        GetTypeValue(index) &= ~static_cast<T>(1 << GetSubOffset(index));
    }

    /// Set a bit value
    void TurnBit(std::size_t index, bool value) {
        if (value) {
            TurnOn(index);
        } else {
            TurnOff(index);
        }
    }

    /// Turns on all bits
    void TurnOnAll() {
        std::memset(raw.data(), std::numeric_limits<u8>::max(), sizeof(raw));
    }

    /// Turns off all bits
    void TurnOffAll() {
        std::memset(raw.data(), 0, sizeof(raw));
    }

    /// Returns the size in bits of the array
    constexpr std::size_t size() const {
        return total_bits;
    }

    /// Returns the capacity in bits of the array
    constexpr std::size_t capacity() const {
        return capacity_bits;
    }

private:
    static constexpr std::size_t type_bits = sizeof(T) * CHAR_BIT;
    static constexpr std::size_t capacity_bits = AlignUp(total_bits, type_bits);

    static constexpr std::size_t GetSubIndex(std::size_t index) {
        return index / type_bits;
    }

    static constexpr std::size_t GetSubOffset(std::size_t index) {
        return index % type_bits;
    }

    constexpr T GetTypeValue(std::size_t index) const {
        return raw[GetSubIndex(index)];
    }

    T& GetTypeValue(std::size_t index) {
        return raw[GetSubIndex(index)];
    }

    // Raw values of the array
    std::array<T, capacity_bits / type_bits> raw{};

    static_assert(std::is_unsigned_v<T>, "T must be an unsigned value.");
};

} // namespace Common
