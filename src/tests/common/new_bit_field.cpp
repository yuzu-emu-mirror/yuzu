// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "common/new_bit_field.h"

namespace Common::BitField {

enum class U8Enum : std::uint8_t {};
enum class U16Enum : std::uint16_t {};
enum class U32Enum : std::uint32_t {};
enum class U64Enum : std::uint64_t {};

enum class S8Enum : std::int8_t {};
enum class S16Enum : std::int16_t {};
enum class S32Enum : std::int32_t {};
enum class S64Enum : std::int64_t {};

template <size_t N>
struct NByteStruct {
    std::array<std::uint8_t, N> raw;
};

struct NonTrivialStruct {
    NonTrivialStruct() {}
};

struct NonTriviallyCopyableStruct {
    NonTriviallyCopyableStruct(const NonTriviallyCopyableStruct&) {}
};

struct NonStandardLayoutStruct {
    std::uint8_t a;

private:
    std::uint8_t b;
};

struct NonPODStruct {
    virtual void Foo() const = 0;
};

// clang-format off

// Tests that must pass.

static_assert(std::is_same_v<bool, BitFieldOutputType<bool, 1>>);

static_assert(std::is_same_v<std::uint8_t, BitFieldOutputType<std::uint8_t, 1>>);
static_assert(std::is_same_v<std::uint8_t, BitFieldOutputType<std::uint8_t, 8>>);
static_assert(std::is_same_v<std::uint8_t, BitFieldOutputType<AutoUInt, 1>>);
static_assert(std::is_same_v<std::uint8_t, BitFieldOutputType<AutoUInt, 8>>);
static_assert(std::is_same_v<std::uint16_t, BitFieldOutputType<std::uint16_t, 1>>);
static_assert(std::is_same_v<std::uint16_t, BitFieldOutputType<std::uint16_t, 16>>);
static_assert(std::is_same_v<std::uint16_t, BitFieldOutputType<AutoUInt, 9>>);
static_assert(std::is_same_v<std::uint16_t, BitFieldOutputType<AutoUInt, 16>>);
static_assert(std::is_same_v<std::uint32_t, BitFieldOutputType<std::uint32_t, 1>>);
static_assert(std::is_same_v<std::uint32_t, BitFieldOutputType<std::uint32_t, 32>>);
static_assert(std::is_same_v<std::uint32_t, BitFieldOutputType<AutoUInt, 17>>);
static_assert(std::is_same_v<std::uint32_t, BitFieldOutputType<AutoUInt, 32>>);
static_assert(std::is_same_v<std::uint64_t, BitFieldOutputType<std::uint64_t, 1>>);
static_assert(std::is_same_v<std::uint64_t, BitFieldOutputType<std::uint64_t, 64>>);
static_assert(std::is_same_v<std::uint64_t, BitFieldOutputType<AutoUInt, 33>>);
static_assert(std::is_same_v<std::uint64_t, BitFieldOutputType<AutoUInt, 64>>);

static_assert(std::is_same_v<std::int8_t, BitFieldOutputType<AutoSignExtSInt, 1>>);
static_assert(std::is_same_v<std::int8_t, BitFieldOutputType<AutoSignExtSInt, 8>>);
static_assert(std::is_same_v<std::int8_t, BitFieldOutputType<AutoZeroExtSInt, 1>>);
static_assert(std::is_same_v<std::int8_t, BitFieldOutputType<AutoZeroExtSInt, 8>>);
static_assert(std::is_same_v<std::int8_t, BitFieldOutputType<SignExtSInt<std::int8_t>, 1>>);
static_assert(std::is_same_v<std::int8_t, BitFieldOutputType<SignExtSInt<std::int8_t>, 8>>);
static_assert(std::is_same_v<std::int8_t, BitFieldOutputType<ZeroExtSInt<std::int8_t>, 1>>);
static_assert(std::is_same_v<std::int8_t, BitFieldOutputType<ZeroExtSInt<std::int8_t>, 8>>);
static_assert(std::is_same_v<std::int16_t, BitFieldOutputType<AutoSignExtSInt, 9>>);
static_assert(std::is_same_v<std::int16_t, BitFieldOutputType<AutoSignExtSInt, 16>>);
static_assert(std::is_same_v<std::int16_t, BitFieldOutputType<AutoZeroExtSInt, 9>>);
static_assert(std::is_same_v<std::int16_t, BitFieldOutputType<AutoZeroExtSInt, 16>>);
static_assert(std::is_same_v<std::int16_t, BitFieldOutputType<SignExtSInt<std::int16_t>, 9>>);
static_assert(std::is_same_v<std::int16_t, BitFieldOutputType<SignExtSInt<std::int16_t>, 16>>);
static_assert(std::is_same_v<std::int16_t, BitFieldOutputType<SignExtSInt<std::int16_t>, 9>>);
static_assert(std::is_same_v<std::int16_t, BitFieldOutputType<SignExtSInt<std::int16_t>, 16>>);
static_assert(std::is_same_v<std::int32_t, BitFieldOutputType<AutoSignExtSInt, 17>>);
static_assert(std::is_same_v<std::int32_t, BitFieldOutputType<AutoSignExtSInt, 32>>);
static_assert(std::is_same_v<std::int32_t, BitFieldOutputType<AutoZeroExtSInt, 17>>);
static_assert(std::is_same_v<std::int32_t, BitFieldOutputType<AutoZeroExtSInt, 32>>);
static_assert(std::is_same_v<std::int32_t, BitFieldOutputType<SignExtSInt<std::int32_t>, 17>>);
static_assert(std::is_same_v<std::int32_t, BitFieldOutputType<SignExtSInt<std::int32_t>, 32>>);
static_assert(std::is_same_v<std::int32_t, BitFieldOutputType<SignExtSInt<std::int32_t>, 17>>);
static_assert(std::is_same_v<std::int32_t, BitFieldOutputType<SignExtSInt<std::int32_t>, 32>>);
static_assert(std::is_same_v<std::int64_t, BitFieldOutputType<AutoSignExtSInt, 33>>);
static_assert(std::is_same_v<std::int64_t, BitFieldOutputType<AutoSignExtSInt, 64>>);
static_assert(std::is_same_v<std::int64_t, BitFieldOutputType<AutoZeroExtSInt, 33>>);
static_assert(std::is_same_v<std::int64_t, BitFieldOutputType<AutoZeroExtSInt, 64>>);
static_assert(std::is_same_v<std::int64_t, BitFieldOutputType<SignExtSInt<std::int64_t>, 33>>);
static_assert(std::is_same_v<std::int64_t, BitFieldOutputType<SignExtSInt<std::int64_t>, 64>>);
static_assert(std::is_same_v<std::int64_t, BitFieldOutputType<SignExtSInt<std::int64_t>, 33>>);
static_assert(std::is_same_v<std::int64_t, BitFieldOutputType<SignExtSInt<std::int64_t>, 64>>);

static_assert(std::is_same_v<float, BitFieldOutputType<float, 32>>);
static_assert(std::is_same_v<float, BitFieldOutputType<AutoFloat, 32>>);
static_assert(std::is_same_v<double, BitFieldOutputType<double, 64>>);
static_assert(std::is_same_v<double, BitFieldOutputType<AutoFloat, 64>>);

static_assert(std::is_same_v<U8Enum, BitFieldOutputType<U8Enum, 1>>);
static_assert(std::is_same_v<U8Enum, BitFieldOutputType<U8Enum, 8>>);
static_assert(std::is_same_v<U16Enum, BitFieldOutputType<U16Enum, 1>>);
static_assert(std::is_same_v<U16Enum, BitFieldOutputType<U16Enum, 16>>);
static_assert(std::is_same_v<U32Enum, BitFieldOutputType<U32Enum, 1>>);
static_assert(std::is_same_v<U32Enum, BitFieldOutputType<U32Enum, 32>>);
static_assert(std::is_same_v<U64Enum, BitFieldOutputType<U64Enum, 1>>);
static_assert(std::is_same_v<U64Enum, BitFieldOutputType<U64Enum, 64>>);

static_assert(std::is_same_v<S8Enum, BitFieldOutputType<SignExtSInt<S8Enum>, 1>>);
static_assert(std::is_same_v<S8Enum, BitFieldOutputType<SignExtSInt<S8Enum>, 8>>);
static_assert(std::is_same_v<S8Enum, BitFieldOutputType<ZeroExtSInt<S8Enum>, 1>>);
static_assert(std::is_same_v<S8Enum, BitFieldOutputType<ZeroExtSInt<S8Enum>, 8>>);
static_assert(std::is_same_v<S16Enum, BitFieldOutputType<SignExtSInt<S16Enum>, 1>>);
static_assert(std::is_same_v<S16Enum, BitFieldOutputType<SignExtSInt<S16Enum>, 16>>);
static_assert(std::is_same_v<S16Enum, BitFieldOutputType<ZeroExtSInt<S16Enum>, 1>>);
static_assert(std::is_same_v<S16Enum, BitFieldOutputType<ZeroExtSInt<S16Enum>, 16>>);
static_assert(std::is_same_v<S32Enum, BitFieldOutputType<SignExtSInt<S32Enum>, 1>>);
static_assert(std::is_same_v<S32Enum, BitFieldOutputType<SignExtSInt<S32Enum>, 32>>);
static_assert(std::is_same_v<S32Enum, BitFieldOutputType<ZeroExtSInt<S32Enum>, 1>>);
static_assert(std::is_same_v<S32Enum, BitFieldOutputType<ZeroExtSInt<S32Enum>, 32>>);
static_assert(std::is_same_v<S64Enum, BitFieldOutputType<SignExtSInt<S64Enum>, 1>>);
static_assert(std::is_same_v<S64Enum, BitFieldOutputType<SignExtSInt<S64Enum>, 64>>);
static_assert(std::is_same_v<S64Enum, BitFieldOutputType<ZeroExtSInt<S64Enum>, 1>>);
static_assert(std::is_same_v<S64Enum, BitFieldOutputType<ZeroExtSInt<S64Enum>, 64>>);

static_assert(std::is_same_v<NByteStruct<1>, BitFieldOutputType<NByteStruct<1>, 8>>);
static_assert(std::is_same_v<NByteStruct<2>, BitFieldOutputType<NByteStruct<2>, 16>>);
static_assert(std::is_same_v<NByteStruct<3>, BitFieldOutputType<NByteStruct<3>, 24>>);
static_assert(std::is_same_v<NByteStruct<4>, BitFieldOutputType<NByteStruct<4>, 32>>);
static_assert(std::is_same_v<NByteStruct<5>, BitFieldOutputType<NByteStruct<5>, 40>>);
static_assert(std::is_same_v<NByteStruct<6>, BitFieldOutputType<NByteStruct<6>, 48>>);
static_assert(std::is_same_v<NByteStruct<7>, BitFieldOutputType<NByteStruct<7>, 56>>);
static_assert(std::is_same_v<NByteStruct<8>, BitFieldOutputType<NByteStruct<8>, 64>>);

static_assert(TypeTraits<AutoSignExtSInt, 8>::SignExtend == true);
static_assert(TypeTraits<AutoZeroExtSInt, 8>::SignExtend == false);
static_assert(TypeTraits<SignExtSInt<std::int8_t>, 8>::SignExtend == true);
static_assert(TypeTraits<SignExtSInt<S8Enum>, 8>::SignExtend == true);
static_assert(TypeTraits<ZeroExtSInt<std::int8_t>, 8>::SignExtend == false);
static_assert(TypeTraits<ZeroExtSInt<S8Enum>, 8>::SignExtend == false);

// Tests that will fail if uncommented.

// NumBits must not be 0.

// static_assert(std::is_same_v<std::uint8_t, BitFieldOutputType<std::uint8_t, 0>>);

// For bool, NumBits must be exactly 1.

// static_assert(std::is_same_v<bool, BitFieldOutputType<bool, 8>>);

// For signed integers, use SignExtSInt<Type> or ZeroExtSInt<Type> instead.

// static_assert(std::is_same_v<std::int8_t, BitFieldOutputType<std::int8_t, 8>>);
// static_assert(std::is_same_v<std::int16_t, BitFieldOutputType<std::int16_t, 16>>);
// static_assert(std::is_same_v<std::int32_t, BitFieldOutputType<std::int32_t, 32>>);
// static_assert(std::is_same_v<std::int64_t, BitFieldOutputType<std::int64_t, 64>>);
// static_assert(std::is_same_v<S8Enum, BitFieldOutputType<S8Enum, 8>>);
// static_assert(std::is_same_v<S16Enum, BitFieldOutputType<S16Enum, 16>>);
// static_assert(std::is_same_v<S32Enum, BitFieldOutputType<S32Enum, 32>>);
// static_assert(std::is_same_v<S64Enum, BitFieldOutputType<S64Enum, 64>>);

// SignExtSInt<Type>: Type must be a signed integral.

// static_assert(std::is_same_v<std::uint8_t, BitFieldOutputType<SignExtSInt<std::uint8_t>, 8>>);
// static_assert(std::is_same_v<float, BitFieldOutputType<SignExtSInt<float>, 32>>);
// static_assert(std::is_same_v<U8Enum, BitFieldOutputType<SignExtSInt<U8Enum>, 8>>);
// static_assert(std::is_same_v<NByteStruct<1>, BitFieldOutputType<SignExtSInt<NByteStruct<1>>, 8>>);

// ZeroExtSInt<Type>: Type must be a signed integral.

// static_assert(std::is_same_v<std::uint8_t, BitFieldOutputType<ZeroExtSInt<std::uint8_t>, 8>>);
// static_assert(std::is_same_v<float, BitFieldOutputType<ZeroExtSInt<float>, 32>>);
// static_assert(std::is_same_v<U8Enum, BitFieldOutputType<ZeroExtSInt<U8Enum>, 8>>);
// static_assert(std::is_same_v<NByteStruct<1>, BitFieldOutputType<ZeroExtSInt<NByteStruct<1>>, 8>>);

// Type must be a POD type.

// static_assert(std::is_same_v<NonTrivialStruct, BitFieldOutputType<NonTrivialStruct, 8>>);
// static_assert(std::is_same_v<NonTriviallyCopyableStruct, BitFieldOutputType<NonTriviallyCopyableStruct, 8>>);
// static_assert(std::is_same_v<NonStandardLayoutStruct, BitFieldOutputType<NonStandardLayoutStruct, 16>>);
// static_assert(std::is_same_v<NonPODStruct, BitFieldOutputType<NonPODStruct, 64>>);

// Type must contain at least NumBits bits.

// static_assert(std::is_same_v<std::uint8_t, BitFieldOutputType<std::uint8_t, 9>>);
// static_assert(std::is_same_v<std::uint16_t, BitFieldOutputType<std::uint16_t, 17>>);
// static_assert(std::is_same_v<std::uint32_t, BitFieldOutputType<std::uint32_t, 33>>);
// static_assert(std::is_same_v<std::uint64_t, BitFieldOutputType<std::uint64_t, 65>>);
// static_assert(std::is_same_v<std::uint64_t, BitFieldOutputType<AutoUInt, 65>>);
// static_assert(std::is_same_v<std::int64_t, BitFieldOutputType<AutoSignExtSInt, 65>>);
// static_assert(std::is_same_v<std::int64_t, BitFieldOutputType<AutoZeroExtSInt, 65>>);

// Type must contain exactly NumBits bits.

// static_assert(std::is_same_v<float, BitFieldOutputType<float, 31>>);
// static_assert(std::is_same_v<double, BitFieldOutputType<double, 63>>);
// static_assert(std::is_same_v<NByteStruct<1>, BitFieldOutputType<NByteStruct<1>, 7>>);
// static_assert(std::is_same_v<NByteStruct<2>, BitFieldOutputType<NByteStruct<2>, 15>>);
// static_assert(std::is_same_v<NByteStruct<3>, BitFieldOutputType<NByteStruct<3>, 23>>);
// static_assert(std::is_same_v<NByteStruct<4>, BitFieldOutputType<NByteStruct<4>, 31>>);
// static_assert(std::is_same_v<NByteStruct<5>, BitFieldOutputType<NByteStruct<5>, 39>>);
// static_assert(std::is_same_v<NByteStruct<6>, BitFieldOutputType<NByteStruct<6>, 47>>);
// static_assert(std::is_same_v<NByteStruct<7>, BitFieldOutputType<NByteStruct<7>, 55>>);
// static_assert(std::is_same_v<NByteStruct<8>, BitFieldOutputType<NByteStruct<8>, 63>>);

// clang-format on

} // namespace Common::BitField
