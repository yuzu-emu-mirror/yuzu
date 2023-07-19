// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <fmt/core.h>

#include "common/bit_cast.h"

// Forward declarations

struct AutoUInt;
struct AutoSignExtSInt;
struct AutoZeroExtSInt;
struct AutoFloat;
template <typename T>
struct SignExtSInt;
template <typename T>
struct ZeroExtSInt;

namespace Common::BitField {

template <typename T>
constexpr size_t BitSize() {
    return sizeof(T) * 8;
}

template <typename T>
concept IsPOD = std::is_trivial_v<T> && std::is_standard_layout_v<T>;

template <typename T, typename = typename std::is_enum<T>::type>
struct ToUnderlyingType {
    using type = T;
};

template <typename T>
struct ToUnderlyingType<T, std::true_type> {
    using type = std::underlying_type_t<T>;
};

// clang-format off

// Automatically deduces the smallest unsigned integer type that contains at least NumBits bits.
template <size_t NumBits>
using SmallestUIntType = std::conditional_t<NumBits <= BitSize<std::uint8_t>(), std::uint8_t,
                         std::conditional_t<NumBits <= BitSize<std::uint16_t>(), std::uint16_t,
                         std::conditional_t<NumBits <= BitSize<std::uint32_t>(), std::uint32_t,
                         std::conditional_t<NumBits <= BitSize<std::uint64_t>(), std::uint64_t, std::uint64_t>>>>;

// Automatically deduces the smallest signed integer type that contains at least NumBits bits.
template <size_t NumBits>
using SmallestSIntType = std::conditional_t<NumBits <= BitSize<std::int8_t>(), std::int8_t,
                         std::conditional_t<NumBits <= BitSize<std::int16_t>(), std::int16_t,
                         std::conditional_t<NumBits <= BitSize<std::int32_t>(), std::int32_t,
                         std::conditional_t<NumBits <= BitSize<std::int64_t>(), std::int64_t, std::int64_t>>>>;

// Automatically deduces the smallest floating point type that contains at exactly NumBits bits.
template <size_t NumBits>
using SmallestFloatType = std::conditional_t<NumBits == BitSize<float>(), float,
                          std::conditional_t<NumBits == BitSize<double>(), double, double>>;

// clang-format on

struct TagType {};

struct AutoType : public TagType {};

struct SIntType : public TagType {};

template <typename T, size_t NumBits,
          typename = typename std::is_base_of<Common::BitField::AutoType, T>::type>
struct DeduceAutoType {
    using type = void;
};

template <typename T, size_t NumBits>
struct DeduceAutoType<T, NumBits, std::true_type> {
    // clang-format off
    using type = std::conditional_t<std::is_same_v<T, AutoUInt>, Common::BitField::SmallestUIntType<NumBits>,
                 std::conditional_t<std::is_same_v<T, AutoSignExtSInt>, Common::BitField::SmallestSIntType<NumBits>,
                 std::conditional_t<std::is_same_v<T, AutoZeroExtSInt>, Common::BitField::SmallestSIntType<NumBits>,
                 std::conditional_t<std::is_same_v<T, AutoFloat>, Common::BitField::SmallestFloatType<NumBits>, void>>>>;
    // clang-format on
};

template <typename T, typename = typename std::is_base_of<Common::BitField::SIntType, T>::type>
struct DeduceSIntType {
    using type = void;
};

template <typename T>
struct DeduceSIntType<T, std::true_type> {
    using type = typename T::OutputType;
};

template <typename T, typename = typename std::is_base_of<Common::BitField::TagType, T>::type>
struct DeduceSignExtendValue {
    static constexpr bool value = false;
};

template <typename T>
struct DeduceSignExtendValue<T, std::true_type> {
    static constexpr bool value = T::SignExtend;
};

template <typename T, size_t NumBits>
struct TypeTraits {
    static_assert(NumBits != 0, "NumBits must not be 0.");
    static_assert(!std::is_same_v<T, bool> || NumBits == 1, "For bool, NumBits must be exactly 1.");
    static_assert(!std::signed_integral<typename ToUnderlyingType<T>::type>,
                  "For signed integers, use SignExtSInt<Type> or ZeroExtSInt<Type> instead.");

    using AutoType = typename DeduceAutoType<T, NumBits>::type;
    using SIntType = typename DeduceSIntType<T>::type;

    // clang-format off
    using OutputType = std::conditional_t<!std::is_void_v<AutoType>, AutoType,
                       std::conditional_t<!std::is_void_v<SIntType>, SIntType,
                                                                     T>>;
    // clang-format on

    static_assert(IsPOD<OutputType>, "Type must be a POD type.");

    using UnderlyingType = typename ToUnderlyingType<OutputType>::type;

    // For integral types, assert that OutputType contains at least NumBits bits.
    static_assert(!std::is_integral_v<UnderlyingType> || BitSize<UnderlyingType>() >= NumBits,
                  "Type must contain at least NumBits bits.");
    // For all other types, assert that OutputType contains exactly NumBits bits.
    static_assert(std::is_integral_v<UnderlyingType> || BitSize<UnderlyingType>() == NumBits,
                  "Type must contain exactly NumBits bits.");

    static constexpr bool SignExtend = DeduceSignExtendValue<T>::value;
};

template <typename To, typename From>
constexpr To AutoBitCast(const From& from) {
    if constexpr (sizeof(To) != sizeof(From)) {
        using FromUnsignedType = SmallestUIntType<BitSize<From>()>;
        using ToUnsignedType = SmallestUIntType<BitSize<To>()>;
        return BitCast<To>(static_cast<ToUnsignedType>(BitCast<FromUnsignedType>(from)));
    } else {
        return BitCast<To>(from);
    }
}

template <typename UnsignedRawType, size_t Position, size_t NumBits>
constexpr UnsignedRawType GenerateBitMask() {
    static_assert(std::unsigned_integral<UnsignedRawType>);
    if constexpr (BitSize<UnsignedRawType>() == NumBits) {
        return ~UnsignedRawType{0};
    } else {
        return ((UnsignedRawType{1} << NumBits) - 1) << Position;
    }
}

template <typename RawType, typename OutputType, size_t Position, size_t NumBits>
constexpr void InsertBits(RawType& raw, OutputType value) {
    using UnsignedType = SmallestUIntType<BitSize<RawType>()>;
    static_assert(sizeof(RawType) == sizeof(UnsignedType));
    constexpr auto Mask = GenerateBitMask<UnsignedType, Position, NumBits>();
    raw = AutoBitCast<RawType>((AutoBitCast<UnsignedType>(raw) & ~Mask) |
                               ((AutoBitCast<UnsignedType>(value) << Position) & Mask));
}

template <typename RawType, typename OutputType, size_t Position, size_t NumBits, bool SignExtend>
constexpr OutputType ExtractBits(const RawType& raw) {
    if constexpr (SignExtend) {
        using SignedType = SmallestSIntType<BitSize<RawType>()>;
        static_assert(sizeof(RawType) == sizeof(SignedType));
        constexpr auto RightShift = BitSize<RawType>() - NumBits;
        constexpr auto LeftShift = RightShift - Position;
        // C++20: Signed Integers are Two’s Complement
        // Left-shift on signed integer types produces the same results as
        // left-shift on the corresponding unsigned integer type.
        // Right-shift is an arithmetic right shift which performs sign-extension.
        return AutoBitCast<OutputType>((AutoBitCast<SignedType>(raw) << LeftShift) >> RightShift);
    } else {
        using UnsignedType = SmallestUIntType<BitSize<RawType>()>;
        static_assert(sizeof(RawType) == sizeof(UnsignedType));
        constexpr auto Mask = GenerateBitMask<UnsignedType, Position, NumBits>();
        return AutoBitCast<OutputType>((AutoBitCast<UnsignedType>(raw) & Mask) >> Position);
    }
}

template <typename RawType, typename OutputType, size_t Position, size_t NumBits, bool SignExtend>
class BitFieldHelper final {
public:
    constexpr BitFieldHelper(RawType& raw_) : raw{raw_} {}

    BitFieldHelper(const BitFieldHelper&) = delete;
    BitFieldHelper& operator=(const BitFieldHelper&) = delete;

    BitFieldHelper(BitFieldHelper&&) = delete;
    BitFieldHelper& operator=(BitFieldHelper&&) = delete;

    constexpr void Set(OutputType value) noexcept {
        return InsertBits<RawType, OutputType, Position, NumBits>(raw, value);
    }

    constexpr BitFieldHelper& operator=(OutputType value) noexcept {
        Set(value);
        return *this;
    }

    [[nodiscard]] constexpr OutputType Get() const noexcept {
        return ExtractBits<RawType, OutputType, Position, NumBits, SignExtend>(raw);
    }

    [[nodiscard]] constexpr operator OutputType() const noexcept {
        return Get();
    }

    template <typename Other>
    [[nodiscard]] friend constexpr bool operator==(const BitFieldHelper& lhs,
                                                   const Other& rhs) noexcept {
        return lhs.Get() == rhs;
    }

    template <typename Other>
    [[nodiscard]] friend constexpr bool operator==(const Other& lhs,
                                                   const BitFieldHelper& rhs) noexcept {
        return lhs == rhs.Get();
    }

    [[nodiscard]] constexpr bool operator==(const BitFieldHelper& other) const noexcept {
        return Get() == other.Get();
    }

private:
    RawType& raw;
};

template <typename RawType, typename OutputType, size_t Position, size_t NumBits, bool SignExtend>
inline auto format_as(BitFieldHelper<RawType, OutputType, Position, NumBits, SignExtend> bitfield) {
    return bitfield.Get();
}

} // namespace Common::BitField

/**
 * A type tag that automatically deduces the smallest unsigned integer type that
 * contains at least NumBits bits in the bitfield.
 * Currently supported unsigned integer types:
 * - u8 (8 bits)
 * - u16 (16 bits)
 * - u32 (32 bits)
 * - u64 (64 bits)
 */
struct AutoUInt : public Common::BitField::AutoType {
    static constexpr bool SignExtend = false;
};

/**
 * A type tag that automatically deduces the smallest signed integer type that
 * contains at least NumBits bits in the bitfield.
 * Additionally, the value is treated as a NumBits-bit 2's complement integer
 * which is sign-extended to fit in the output type, preserving its sign and value.
 * Currently supported signed integer types:
 * - s8 (8 bits)
 * - s16 (16 bits)
 * - s32 (32 bits)
 * - s64 (64 bits)
 */
struct AutoSignExtSInt : public Common::BitField::AutoType {
    static constexpr bool SignExtend = true;
};

/**
 * A type tag that automatically deduces the smallest signed integer type that
 * contains at least NumBits bits in the bitfield.
 * Unlike AutoSignExtInt, the value is zero-extended to fit in the output type,
 * effectively treating it as an unsigned integer that is bitcast to a signed integer.
 * Its sign and value are not preserved, unless the output type contains exactly NumBits bits.
 * Currently supported signed integer types:
 * - s8 (8 bits)
 * - s16 (16 bits)
 * - s32 (32 bits)
 * - s64 (64 bits)
 */
struct AutoZeroExtSInt : public Common::BitField::AutoType {
    static constexpr bool SignExtend = false;
};

/**
 * A type tag that automatically deduces the smallest floating point type that
 * contains exactly NumBits bits in the bitfield.
 * Currently supported floating point types:
 * - float (32 bits)
 * - double (64 bits)
 */
struct AutoFloat : public Common::BitField::AutoType {
    static constexpr bool SignExtend = false;
};

/**
 * A type tag that treats the value as a NumBits-bit 2's Complement Integer
 * which is sign-extended to fit in the output type, preserving its sign and value.
 * Currently supported signed integer types:
 * - s8 (8 bits)
 * - s16 (16 bits)
 * - s32 (32 bits)
 * - s64 (64 bits)
 */
template <typename T>
struct SignExtSInt : public Common::BitField::SIntType {
    static_assert(std::signed_integral<typename Common::BitField::ToUnderlyingType<T>::type>,
                  "SignExtSInt<Type>: Type must be a signed integral.");

    using OutputType = T;
    static constexpr bool SignExtend = true;
};

/**
 * A type tag that, unlike SignExtInt, zero-extends the value to fit in the output type,
 * effectively treating it as an unsigned integer that is bitcast to a signed integer.
 * Its sign and value are not preserved, unless the output type contains exactly NumBits bits.
 * Currently supported signed integer types:
 * - s8 (8 bits)
 * - s16 (16 bits)
 * - s32 (32 bits)
 * - s64 (64 bits)
 */
template <typename T>
struct ZeroExtSInt : public Common::BitField::SIntType {
    static_assert(std::signed_integral<typename Common::BitField::ToUnderlyingType<T>::type>,
                  "ZeroExtSInt<Type>: Type must be a signed integral.");

    using OutputType = T;
    static constexpr bool SignExtend = false;
};

template <typename T, size_t NumBits>
using BitFieldOutputType = typename Common::BitField::TypeTraits<T, NumBits>::OutputType;

#define YUZU_RO_BITFIELD(Position, NumBits, Type, Name)                                            \
    constexpr BitFieldOutputType<Type, NumBits> Name() const {                                     \
        using BitFieldTypeTraits = Common::BitField::TypeTraits<Type, NumBits>;                    \
        using OutputType = BitFieldTypeTraits::OutputType;                                         \
        constexpr bool SignExtend = BitFieldTypeTraits::SignExtend;                                \
        using ThisType = std::remove_cvref_t<decltype(*this)>;                                     \
        static_assert(!std::is_union_v<ThisType>,                                                  \
                      "An object containing BitFields cannot be a union type.");                   \
        static_assert(Common::BitField::IsPOD<ThisType>,                                           \
                      "An object containing BitFields must be a POD type.");                       \
        static_assert(Common::BitField::BitSize<ThisType>() <= 64,                                 \
                      "An object containing BitFields must be at most 64 bits in size.");          \
        /* A structured binding is used to decompose *this into its constituent members. */        \
        /* It also allows us to guarantee that we only have one member in *this object. */         \
        /* Bit manipulation is performed on this member, so it must support bit operators. */      \
        const auto& [yuzu_raw_value] = *this;                                                      \
        using RawType = std::remove_cvref_t<decltype(raw)>;                                        \
        static_assert(Common::BitField::IsPOD<RawType>,                                            \
                      "An object containing BitFields must be a POD type.");                       \
        static_assert(Common::BitField::BitSize<RawType>() <= 64,                                  \
                      "An object containing BitFields must be at most 64 bits in size.");          \
        static_assert(Position < Common::BitField::BitSize<RawType>(),                             \
                      "BitField is out of range.");                                                \
        static_assert(NumBits <= Common::BitField::BitSize<RawType>(),                             \
                      "BitField is out of range.");                                                \
        static_assert(Position + NumBits <= Common::BitField::BitSize<RawType>(),                  \
                      "BitField is out of range.");                                                \
        return Common::BitField::ExtractBits<RawType, OutputType, Position, NumBits, SignExtend>(  \
            yuzu_raw_value);                                                                       \
    }

#define YUZU_BITFIELD(Position, NumBits, Type, Name)                                               \
    YUZU_RO_BITFIELD(Position, NumBits, Type, Name);                                               \
    constexpr auto Name() {                                                                        \
        using BitFieldTypeTraits = Common::BitField::TypeTraits<Type, NumBits>;                    \
        using OutputType = BitFieldTypeTraits::OutputType;                                         \
        constexpr bool SignExtend = BitFieldTypeTraits::SignExtend;                                \
        using ThisType = std::remove_cvref_t<decltype(*this)>;                                     \
        static_assert(!std::is_union_v<ThisType>,                                                  \
                      "An object containing BitFields cannot be a union type.");                   \
        static_assert(Common::BitField::IsPOD<ThisType>,                                           \
                      "An object containing BitFields must be a POD type.");                       \
        static_assert(Common::BitField::BitSize<ThisType>() <= 64,                                 \
                      "An object containing BitFields must be at most 64 bits in size.");          \
        /* A structured binding is used to decompose *this into its constituent members. */        \
        /* It also allows us to guarantee that we only have one member in *this object. */         \
        /* Bit manipulation is performed on this member, so it must support bit operators. */      \
        auto& [yuzu_raw_value] = *this;                                                            \
        using RawType = std::remove_cvref_t<decltype(raw)>;                                        \
        static_assert(Common::BitField::IsPOD<RawType>,                                            \
                      "An object containing BitFields must be a POD type.");                       \
        static_assert(Common::BitField::BitSize<RawType>() <= 64,                                  \
                      "An object containing BitFields must be at most 64 bits in size.");          \
        static_assert(Position < Common::BitField::BitSize<RawType>(),                             \
                      "BitField is out of range.");                                                \
        static_assert(NumBits <= Common::BitField::BitSize<RawType>(),                             \
                      "BitField is out of range.");                                                \
        static_assert(Position + NumBits <= Common::BitField::BitSize<RawType>(),                  \
                      "BitField is out of range.");                                                \
        return Common::BitField::BitFieldHelper<RawType, OutputType, Position, NumBits,            \
                                                SignExtend>{yuzu_raw_value};                       \
    }
