// Copyright 2016 The University of North Carolina at Chapel Hill
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Please send all BUG REPORTS to <pavel@cs.unc.edu>.
// <http://gamma.cs.unc.edu/FasTC/>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

#include "common/common_types.h"

#include "video_core/textures/astc.h"

namespace {

/// Count the number of bits set in a number.
constexpr u32 Popcnt(u32 n) {
    u32 c = 0;
    for (; n; c++) {
        n &= n - 1;
    }
    return c;
}

} // Anonymous namespace

class InputBitStream {
public:
    explicit InputBitStream(const u8* ptr, std::size_t start_offset = 0)
        : m_CurByte(ptr), m_NextBit(start_offset % 8) {}

    std::size_t GetBitsRead() const {
        return m_BitsRead;
    }

    u32 ReadBit() {
        u32 bit = *m_CurByte >> m_NextBit++;
        while (m_NextBit >= 8) {
            m_NextBit -= 8;
            m_CurByte++;
        }

        m_BitsRead++;
        return bit & 1;
    }

    u32 ReadBits(std::size_t nBits) {
        u32 ret = 0;
        for (std::size_t i = 0; i < nBits; ++i) {
            ret |= (ReadBit() & 1) << i;
        }
        return ret;
    }

    template <std::size_t nBits>
    u32 ReadBits() {
        u32 ret = 0;
        for (std::size_t i = 0; i < nBits; ++i) {
            ret |= (ReadBit() & 1) << i;
        }
        return ret;
    }

private:
    const u8* m_CurByte;
    std::size_t m_NextBit = 0;
    std::size_t m_BitsRead = 0;
};

class OutputBitStream {
public:
    explicit OutputBitStream(u8* ptr, s32 nBits = 0, s32 start_offset = 0)
        : m_NumBits(nBits), m_CurByte(ptr), m_NextBit(start_offset % 8) {}

    ~OutputBitStream() = default;

    s32 GetBitsWritten() const {
        return m_BitsWritten;
    }

    void WriteBitsR(u32 val, u32 nBits) {
        for (u32 i = 0; i < nBits; i++) {
            WriteBit((val >> (nBits - i - 1)) & 1);
        }
    }

    void WriteBits(u32 val, u32 nBits) {
        for (u32 i = 0; i < nBits; i++) {
            WriteBit((val >> i) & 1);
        }
    }

private:
    void WriteBit(s32 b) {

        if (done)
            return;

        const u32 mask = 1 << m_NextBit++;

        // clear the bit
        *m_CurByte &= static_cast<u8>(~mask);

        // Write the bit, if necessary
        if (b)
            *m_CurByte |= static_cast<u8>(mask);

        // Next byte?
        if (m_NextBit >= 8) {
            m_CurByte += 1;
            m_NextBit = 0;
        }

        done = done || ++m_BitsWritten >= m_NumBits;
    }

    s32 m_BitsWritten = 0;
    const s32 m_NumBits;
    u8* m_CurByte;
    s32 m_NextBit = 0;

    bool done = false;
};

template <typename IntType>
class Bits {
public:
    explicit Bits(const IntType& v) : m_Bits(v) {}

    Bits(const Bits&) = delete;
    Bits& operator=(const Bits&) = delete;

    u8 operator[](u32 bitPos) const {
        return static_cast<u8>((m_Bits >> bitPos) & 1);
    }

    IntType operator()(u32 start, u32 end) const {
        if (start == end) {
            return (*this)[start];
        } else if (start > end) {
            u32 t = start;
            start = end;
            end = t;
        }

        u64 mask = (1 << (end - start + 1)) - 1;
        return (m_Bits >> start) & static_cast<IntType>(mask);
    }

private:
    const IntType& m_Bits;
};

enum class IntegerEncoding { JustBits, Qus32, Trit };

struct IntegerEncodedValue {
    constexpr IntegerEncodedValue() = default;

    constexpr IntegerEncodedValue(IntegerEncoding encoding_, u32 num_bits_)
        : encoding{encoding_}, num_bits{num_bits_} {}

    constexpr bool MatchesEncoding(const IntegerEncodedValue& other) const {
        return encoding == other.encoding && num_bits == other.num_bits;
    }

    // Returns the number of bits required to encode nVals values.
    u32 GetBitLength(u32 nVals) const {
        u32 totalBits = num_bits * nVals;
        if (encoding == IntegerEncoding::Trit) {
            totalBits += (nVals * 8 + 4) / 5;
        } else if (encoding == IntegerEncoding::Qus32) {
            totalBits += (nVals * 7 + 2) / 3;
        }
        return totalBits;
    }

    IntegerEncoding encoding{};
    u32 num_bits = 0;
    u32 bit_value = 0;
    union {
        u32 qus32_value = 0;
        u32 trit_value;
    };
};

static void DecodeTritBlock(InputBitStream& bits, std::vector<IntegerEncodedValue>& result,
                            u32 nBitsPerValue) {
    // Implement the algorithm in section C.2.12
    u32 m[5];
    u32 t[5];
    u32 T;

    // Read the trit encoded block according to
    // table C.2.14
    m[0] = bits.ReadBits(nBitsPerValue);
    T = bits.ReadBits<2>();
    m[1] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBits<2>() << 2;
    m[2] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBit() << 4;
    m[3] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBits<2>() << 5;
    m[4] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBit() << 7;

    u32 C = 0;

    Bits<u32> Tb(T);
    if (Tb(2, 4) == 7) {
        C = (Tb(5, 7) << 2) | Tb(0, 1);
        t[4] = t[3] = 2;
    } else {
        C = Tb(0, 4);
        if (Tb(5, 6) == 3) {
            t[4] = 2;
            t[3] = Tb[7];
        } else {
            t[4] = Tb[7];
            t[3] = Tb(5, 6);
        }
    }

    Bits<u32> Cb(C);
    if (Cb(0, 1) == 3) {
        t[2] = 2;
        t[1] = Cb[4];
        t[0] = (Cb[3] << 1) | (Cb[2] & ~Cb[3]);
    } else if (Cb(2, 3) == 3) {
        t[2] = 2;
        t[1] = 2;
        t[0] = Cb(0, 1);
    } else {
        t[2] = Cb[4];
        t[1] = Cb(2, 3);
        t[0] = (Cb[1] << 1) | (Cb[0] & ~Cb[1]);
    }

    for (std::size_t i = 0; i < 5; ++i) {
        IntegerEncodedValue& val = result.emplace_back(IntegerEncoding::Trit, nBitsPerValue);
        val.bit_value = m[i];
        val.trit_value = t[i];
    }
}

static void DecodeQus32Block(InputBitStream& bits, std::vector<IntegerEncodedValue>& result,
                             u32 nBitsPerValue) {
    // Implement the algorithm in section C.2.12
    u32 m[3];
    u32 q[3];
    u32 Q;

    // Read the trit encoded block according to
    // table C.2.15
    m[0] = bits.ReadBits(nBitsPerValue);
    Q = bits.ReadBits<3>();
    m[1] = bits.ReadBits(nBitsPerValue);
    Q |= bits.ReadBits<2>() << 3;
    m[2] = bits.ReadBits(nBitsPerValue);
    Q |= bits.ReadBits<2>() << 5;

    Bits<u32> Qb(Q);
    if (Qb(1, 2) == 3 && Qb(5, 6) == 0) {
        q[0] = q[1] = 4;
        q[2] = (Qb[0] << 2) | ((Qb[4] & ~Qb[0]) << 1) | (Qb[3] & ~Qb[0]);
    } else {
        u32 C = 0;
        if (Qb(1, 2) == 3) {
            q[2] = 4;
            C = (Qb(3, 4) << 3) | ((~Qb(5, 6) & 3) << 1) | Qb[0];
        } else {
            q[2] = Qb(5, 6);
            C = Qb(0, 4);
        }

        Bits<u32> Cb(C);
        if (Cb(0, 2) == 5) {
            q[1] = 4;
            q[0] = Cb(3, 4);
        } else {
            q[1] = Cb(3, 4);
            q[0] = Cb(0, 2);
        }
    }

    for (std::size_t i = 0; i < 3; ++i) {
        IntegerEncodedValue& val = result.emplace_back(IntegerEncoding::Qus32, nBitsPerValue);
        val.bit_value = m[i];
        val.qus32_value = q[i];
    }
}

// Returns a new instance of this struct that corresponds to the
// can take no more than maxval values
static constexpr IntegerEncodedValue CreateEncoding(u32 maxVal) {
    while (maxVal > 0) {
        u32 check = maxVal + 1;

        // Is maxVal a power of two?
        if (!(check & (check - 1))) {
            return IntegerEncodedValue(IntegerEncoding::JustBits, Popcnt(maxVal));
        }

        // Is maxVal of the type 3*2^n - 1?
        if ((check % 3 == 0) && !((check / 3) & ((check / 3) - 1))) {
            return IntegerEncodedValue(IntegerEncoding::Trit, Popcnt(check / 3 - 1));
        }

        // Is maxVal of the type 5*2^n - 1?
        if ((check % 5 == 0) && !((check / 5) & ((check / 5) - 1))) {
            return IntegerEncodedValue(IntegerEncoding::Qus32, Popcnt(check / 5 - 1));
        }

        // Apparently it can't be represented with a bounded s32eger sequence...
        // just iterate.
        maxVal--;
    }
    return IntegerEncodedValue(IntegerEncoding::JustBits, 0);
}

static constexpr std::array<IntegerEncodedValue, 256> MakeEncodedValues() {
    std::array<IntegerEncodedValue, 256> encodings{};
    for (std::size_t i = 0; i < encodings.size(); ++i) {
        encodings[i] = CreateEncoding(static_cast<u32>(i));
    }
    return encodings;
}

static constexpr std::array EncodingsValues = MakeEncodedValues();

// Fills result with the values that are encoded in the given
// bitstream. We must know beforehand what the maximum possible
// value is, and how many values we're decoding.
static void DecodeIntegerSequence(std::vector<IntegerEncodedValue>& result, InputBitStream& bits,
                                  u32 maxRange, u32 nValues) {
    // Determine encoding parameters
    IntegerEncodedValue val = EncodingsValues[maxRange];

    // Start decoding
    u32 nValsDecoded = 0;
    while (nValsDecoded < nValues) {
        switch (val.encoding) {
        case IntegerEncoding::Qus32:
            DecodeQus32Block(bits, result, val.num_bits);
            nValsDecoded += 3;
            break;

        case IntegerEncoding::Trit:
            DecodeTritBlock(bits, result, val.num_bits);
            nValsDecoded += 5;
            break;

        case IntegerEncoding::JustBits:
            val.bit_value = bits.ReadBits(val.num_bits);
            result.push_back(val);
            nValsDecoded++;
            break;
        }
    }
}

namespace ASTCC {

struct TexelWeightParams {
    u32 m_Width = 0;
    u32 m_Height = 0;
    bool m_bDualPlane = false;
    u32 m_MaxWeight = 0;
    bool m_bError = false;
    bool m_bVoidExtentLDR = false;
    bool m_bVoidExtentHDR = false;

    u32 GetPackedBitSize() const {
        // How many indices do we have?
        u32 nIdxs = m_Height * m_Width;
        if (m_bDualPlane) {
            nIdxs *= 2;
        }

        return EncodingsValues[m_MaxWeight].GetBitLength(nIdxs);
    }

    u32 GetNumWeightValues() const {
        u32 ret = m_Width * m_Height;
        if (m_bDualPlane) {
            ret *= 2;
        }
        return ret;
    }
};

static TexelWeightParams DecodeBlockInfo(InputBitStream& strm) {
    TexelWeightParams params;

    // Read the entire block mode all at once
    u16 modeBits = static_cast<u16>(strm.ReadBits<11>());

    // Does this match the void extent block mode?
    if ((modeBits & 0x01FF) == 0x1FC) {
        if (modeBits & 0x200) {
            params.m_bVoidExtentHDR = true;
        } else {
            params.m_bVoidExtentLDR = true;
        }

        // Next two bits must be one.
        if (!(modeBits & 0x400) || !strm.ReadBit()) {
            params.m_bError = true;
        }

        return params;
    }

    // First check if the last four bits are zero
    if ((modeBits & 0xF) == 0) {
        params.m_bError = true;
        return params;
    }

    // If the last two bits are zero, then if bits
    // [6-8] are all ones, this is also reserved.
    if ((modeBits & 0x3) == 0 && (modeBits & 0x1C0) == 0x1C0) {
        params.m_bError = true;
        return params;
    }

    // Otherwise, there is no error... Figure out the layout
    // of the block mode. Layout is determined by a number
    // between 0 and 9 corresponding to table C.2.8 of the
    // ASTC spec.
    u32 layout = 0;

    if ((modeBits & 0x1) || (modeBits & 0x2)) {
        // layout is in [0-4]
        if (modeBits & 0x8) {
            // layout is in [2-4]
            if (modeBits & 0x4) {
                // layout is in [3-4]
                if (modeBits & 0x100) {
                    layout = 4;
                } else {
                    layout = 3;
                }
            } else {
                layout = 2;
            }
        } else {
            // layout is in [0-1]
            if (modeBits & 0x4) {
                layout = 1;
            } else {
                layout = 0;
            }
        }
    } else {
        // layout is in [5-9]
        if (modeBits & 0x100) {
            // layout is in [7-9]
            if (modeBits & 0x80) {
                // layout is in [7-8]
                assert((modeBits & 0x40) == 0U);
                if (modeBits & 0x20) {
                    layout = 8;
                } else {
                    layout = 7;
                }
            } else {
                layout = 9;
            }
        } else {
            // layout is in [5-6]
            if (modeBits & 0x80) {
                layout = 6;
            } else {
                layout = 5;
            }
        }
    }

    assert(layout < 10);

    // Determine R
    u32 R = !!(modeBits & 0x10);
    if (layout < 5) {
        R |= (modeBits & 0x3) << 1;
    } else {
        R |= (modeBits & 0xC) >> 1;
    }
    assert(2 <= R && R <= 7);

    // Determine width & height
    switch (layout) {
    case 0: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 7) & 0x3;
        params.m_Width = B + 4;
        params.m_Height = A + 2;
        break;
    }

    case 1: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 7) & 0x3;
        params.m_Width = B + 8;
        params.m_Height = A + 2;
        break;
    }

    case 2: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 7) & 0x3;
        params.m_Width = A + 2;
        params.m_Height = B + 8;
        break;
    }

    case 3: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 7) & 0x1;
        params.m_Width = A + 2;
        params.m_Height = B + 6;
        break;
    }

    case 4: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 7) & 0x1;
        params.m_Width = B + 2;
        params.m_Height = A + 2;
        break;
    }

    case 5: {
        u32 A = (modeBits >> 5) & 0x3;
        params.m_Width = 12;
        params.m_Height = A + 2;
        break;
    }

    case 6: {
        u32 A = (modeBits >> 5) & 0x3;
        params.m_Width = A + 2;
        params.m_Height = 12;
        break;
    }

    case 7: {
        params.m_Width = 6;
        params.m_Height = 10;
        break;
    }

    case 8: {
        params.m_Width = 10;
        params.m_Height = 6;
        break;
    }

    case 9: {
        u32 A = (modeBits >> 5) & 0x3;
        u32 B = (modeBits >> 9) & 0x3;
        params.m_Width = A + 6;
        params.m_Height = B + 6;
        break;
    }

    default:
        assert(!"Don't know this layout...");
        params.m_bError = true;
        break;
    }

    // Determine whether or not we're using dual planes
    // and/or high precision layouts.
    bool D = (layout != 9) && (modeBits & 0x400);
    bool H = (layout != 9) && (modeBits & 0x200);

    if (H) {
        const u32 maxWeights[6] = {9, 11, 15, 19, 23, 31};
        params.m_MaxWeight = maxWeights[R - 2];
    } else {
        const u32 maxWeights[6] = {1, 2, 3, 4, 5, 7};
        params.m_MaxWeight = maxWeights[R - 2];
    }

    params.m_bDualPlane = D;

    return params;
}

static void FillVoidExtentLDR(InputBitStream& strm, u32* const outBuf, u32 blockWidth,
                              u32 blockHeight) {
    // Don't actually care about the void extent, just read the bits...
    for (s32 i = 0; i < 4; ++i) {
        strm.ReadBits<13>();
    }

    // Decode the RGBA components and renormalize them to the range [0, 255]
    u16 r = static_cast<u16>(strm.ReadBits<16>());
    u16 g = static_cast<u16>(strm.ReadBits<16>());
    u16 b = static_cast<u16>(strm.ReadBits<16>());
    u16 a = static_cast<u16>(strm.ReadBits<16>());

    u32 rgba = (r >> 8) | (g & 0xFF00) | (static_cast<u32>(b) & 0xFF00) << 8 |
               (static_cast<u32>(a) & 0xFF00) << 16;

    for (u32 j = 0; j < blockHeight; j++) {
        for (u32 i = 0; i < blockWidth; i++) {
            outBuf[j * blockWidth + i] = rgba;
        }
    }
}

static void FillError(u32* outBuf, u32 blockWidth, u32 blockHeight) {
    for (u32 j = 0; j < blockHeight; j++) {
        for (u32 i = 0; i < blockWidth; i++) {
            outBuf[j * blockWidth + i] = 0xFFFF00FF;
        }
    }
}

// Replicates low numBits such that [(toBit - 1):(toBit - 1 - fromBit)]
// is the same as [(numBits - 1):0] and repeats all the way down.
template <typename IntType>
static IntType Replicate(IntType val, u32 numBits, u32 toBit) {
    if (numBits == 0)
        return 0;
    if (toBit == 0)
        return 0;
    IntType v = val & static_cast<IntType>((1 << numBits) - 1);
    IntType res = v;
    u32 reslen = numBits;
    while (reslen < toBit) {
        u32 comp = 0;
        if (numBits > toBit - reslen) {
            u32 newshift = toBit - reslen;
            comp = numBits - newshift;
            numBits = newshift;
        }
        res = static_cast<IntType>(res << numBits);
        res = static_cast<IntType>(res | (v >> comp));
        reslen += numBits;
    }
    return res;
}

class Pixel {
protected:
    using ChannelType = s16;
    u8 m_BitDepth[4] = {8, 8, 8, 8};
    s16 color[4] = {};

public:
    Pixel() = default;
    Pixel(u32 a, u32 r, u32 g, u32 b, u32 bitDepth = 8)
        : m_BitDepth{u8(bitDepth), u8(bitDepth), u8(bitDepth), u8(bitDepth)},
          color{static_cast<ChannelType>(a), static_cast<ChannelType>(r),
                static_cast<ChannelType>(g), static_cast<ChannelType>(b)} {}

    // Changes the depth of each pixel. This scales the values to
    // the appropriate bit depth by either truncating the least
    // significant bits when going from larger to smaller bit depth
    // or by repeating the most significant bits when going from
    // smaller to larger bit depths.
    void ChangeBitDepth(const u8 (&depth)[4]) {
        for (u32 i = 0; i < 4; i++) {
            Component(i) = ChangeBitDepth(Component(i), m_BitDepth[i], depth[i]);
            m_BitDepth[i] = depth[i];
        }
    }

    template <typename IntType>
    static float ConvertChannelToFloat(IntType channel, u8 bitDepth) {
        float denominator = static_cast<float>((1 << bitDepth) - 1);
        return static_cast<float>(channel) / denominator;
    }

    // Changes the bit depth of a single component. See the comment
    // above for how we do this.
    static ChannelType ChangeBitDepth(Pixel::ChannelType val, u8 oldDepth, u8 newDepth) {
        assert(newDepth <= 8);
        assert(oldDepth <= 8);

        if (oldDepth == newDepth) {
            // Do nothing
            return val;
        } else if (oldDepth == 0 && newDepth != 0) {
            return static_cast<ChannelType>((1 << newDepth) - 1);
        } else if (newDepth > oldDepth) {
            return Replicate(val, oldDepth, newDepth);
        } else {
            // oldDepth > newDepth
            if (newDepth == 0) {
                return 0xFF;
            } else {
                u8 bitsWasted = static_cast<u8>(oldDepth - newDepth);
                u16 v = static_cast<u16>(val);
                v = static_cast<u16>((v + (1 << (bitsWasted - 1))) >> bitsWasted);
                v = ::std::min<u16>(::std::max<u16>(0, v), static_cast<u16>((1 << newDepth) - 1));
                return static_cast<u8>(v);
            }
        }

        assert(!"We shouldn't get here.");
        return 0;
    }

    const ChannelType& A() const {
        return color[0];
    }
    ChannelType& A() {
        return color[0];
    }
    const ChannelType& R() const {
        return color[1];
    }
    ChannelType& R() {
        return color[1];
    }
    const ChannelType& G() const {
        return color[2];
    }
    ChannelType& G() {
        return color[2];
    }
    const ChannelType& B() const {
        return color[3];
    }
    ChannelType& B() {
        return color[3];
    }
    const ChannelType& Component(u32 idx) const {
        return color[idx];
    }
    ChannelType& Component(u32 idx) {
        return color[idx];
    }

    void GetBitDepth(u8 (&outDepth)[4]) const {
        for (s32 i = 0; i < 4; i++) {
            outDepth[i] = m_BitDepth[i];
        }
    }

    // Take all of the components, transform them to their 8-bit variants,
    // and then pack each channel s32o an R8G8B8A8 32-bit s32eger. We assume
    // that the architecture is little-endian, so the alpha channel will end
    // up in the most-significant byte.
    u32 Pack() const {
        Pixel eightBit(*this);
        const u8 eightBitDepth[4] = {8, 8, 8, 8};
        eightBit.ChangeBitDepth(eightBitDepth);

        u32 r = 0;
        r |= eightBit.A();
        r <<= 8;
        r |= eightBit.B();
        r <<= 8;
        r |= eightBit.G();
        r <<= 8;
        r |= eightBit.R();
        return r;
    }

    // Clamps the pixel to the range [0,255]
    void ClampByte() {
        for (u32 i = 0; i < 4; i++) {
            color[i] = (color[i] < 0) ? 0 : ((color[i] > 255) ? 255 : color[i]);
        }
    }

    void MakeOpaque() {
        A() = 255;
    }
};

static void DecodeColorValues(u32* out, u8* data, const u32* modes, const u32 nPartitions,
                              const u32 nBitsForColorData) {
    // First figure out how many color values we have
    u32 nValues = 0;
    for (u32 i = 0; i < nPartitions; i++) {
        nValues += ((modes[i] >> 2) + 1) << 1;
    }

    // Then based on the number of values and the remaining number of bits,
    // figure out the max value for each of them...
    u32 range = 256;
    while (--range > 0) {
        IntegerEncodedValue val = EncodingsValues[range];
        u32 bitLength = val.GetBitLength(nValues);
        if (bitLength <= nBitsForColorData) {
            // Find the smallest possible range that matches the given encoding
            while (--range > 0) {
                IntegerEncodedValue newval = EncodingsValues[range];
                if (!newval.MatchesEncoding(val)) {
                    break;
                }
            }

            // Return to last matching range.
            range++;
            break;
        }
    }

    // We now have enough to decode our s32eger sequence.
    std::vector<IntegerEncodedValue> decodedColorValues;
    decodedColorValues.reserve(32);

    InputBitStream colorStream(data);
    DecodeIntegerSequence(decodedColorValues, colorStream, range, nValues);

    // Once we have the decoded values, we need to dequantize them to the 0-255 range
    // This procedure is outlined in ASTC spec C.2.13
    u32 outIdx = 0;
    for (auto itr = decodedColorValues.begin(); itr != decodedColorValues.end(); ++itr) {
        // Have we already decoded all that we need?
        if (outIdx >= nValues) {
            break;
        }

        const IntegerEncodedValue& val = *itr;
        u32 bitlen = val.num_bits;
        u32 bitval = val.bit_value;

        assert(bitlen >= 1);

        u32 A = 0, B = 0, C = 0, D = 0;
        // A is just the lsb replicated 9 times.
        A = Replicate(bitval & 1, 1, 9);

        switch (val.encoding) {
        // Replicate bits
        case IntegerEncoding::JustBits:
            out[outIdx++] = Replicate(bitval, bitlen, 8);
            break;

        // Use algorithm in C.2.13
        case IntegerEncoding::Trit: {

            D = val.trit_value;

            switch (bitlen) {
            case 1: {
                C = 204;
            } break;

            case 2: {
                C = 93;
                // B = b000b0bb0
                u32 b = (bitval >> 1) & 1;
                B = (b << 8) | (b << 4) | (b << 2) | (b << 1);
            } break;

            case 3: {
                C = 44;
                // B = cb000cbcb
                u32 cb = (bitval >> 1) & 3;
                B = (cb << 7) | (cb << 2) | cb;
            } break;

            case 4: {
                C = 22;
                // B = dcb000dcb
                u32 dcb = (bitval >> 1) & 7;
                B = (dcb << 6) | dcb;
            } break;

            case 5: {
                C = 11;
                // B = edcb000ed
                u32 edcb = (bitval >> 1) & 0xF;
                B = (edcb << 5) | (edcb >> 2);
            } break;

            case 6: {
                C = 5;
                // B = fedcb000f
                u32 fedcb = (bitval >> 1) & 0x1F;
                B = (fedcb << 4) | (fedcb >> 4);
            } break;

            default:
                assert(!"Unsupported trit encoding for color values!");
                break;
            } // switch(bitlen)
        }     // case IntegerEncoding::Trit
        break;

        case IntegerEncoding::Qus32: {

            D = val.qus32_value;

            switch (bitlen) {
            case 1: {
                C = 113;
            } break;

            case 2: {
                C = 54;
                // B = b0000bb00
                u32 b = (bitval >> 1) & 1;
                B = (b << 8) | (b << 3) | (b << 2);
            } break;

            case 3: {
                C = 26;
                // B = cb0000cbc
                u32 cb = (bitval >> 1) & 3;
                B = (cb << 7) | (cb << 1) | (cb >> 1);
            } break;

            case 4: {
                C = 13;
                // B = dcb0000dc
                u32 dcb = (bitval >> 1) & 7;
                B = (dcb << 6) | (dcb >> 1);
            } break;

            case 5: {
                C = 6;
                // B = edcb0000e
                u32 edcb = (bitval >> 1) & 0xF;
                B = (edcb << 5) | (edcb >> 3);
            } break;

            default:
                assert(!"Unsupported qus32 encoding for color values!");
                break;
            } // switch(bitlen)
        }     // case IntegerEncoding::Qus32
        break;
        } // switch(val.encoding)

        if (val.encoding != IntegerEncoding::JustBits) {
            u32 T = D * C + B;
            T ^= A;
            T = (A & 0x80) | (T >> 2);
            out[outIdx++] = T;
        }
    }

    // Make sure that each of our values is in the proper range...
    for (u32 i = 0; i < nValues; i++) {
        assert(out[i] <= 255);
    }
}

static u32 UnquantizeTexelWeight(const IntegerEncodedValue& val) {
    u32 bitval = val.bit_value;
    u32 bitlen = val.num_bits;

    u32 A = Replicate(bitval & 1, 1, 7);
    u32 B = 0, C = 0, D = 0;

    u32 result = 0;
    switch (val.encoding) {
    case IntegerEncoding::JustBits:
        result = Replicate(bitval, bitlen, 6);
        break;

    case IntegerEncoding::Trit: {
        D = val.trit_value;
        assert(D < 3);

        switch (bitlen) {
        case 0: {
            u32 results[3] = {0, 32, 63};
            result = results[D];
        } break;

        case 1: {
            C = 50;
        } break;

        case 2: {
            C = 23;
            u32 b = (bitval >> 1) & 1;
            B = (b << 6) | (b << 2) | b;
        } break;

        case 3: {
            C = 11;
            u32 cb = (bitval >> 1) & 3;
            B = (cb << 5) | cb;
        } break;

        default:
            assert(!"Invalid trit encoding for texel weight");
            break;
        }
    } break;

    case IntegerEncoding::Qus32: {
        D = val.qus32_value;
        assert(D < 5);

        switch (bitlen) {
        case 0: {
            u32 results[5] = {0, 16, 32, 47, 63};
            result = results[D];
        } break;

        case 1: {
            C = 28;
        } break;

        case 2: {
            C = 13;
            u32 b = (bitval >> 1) & 1;
            B = (b << 6) | (b << 1);
        } break;

        default:
            assert(!"Invalid qus32 encoding for texel weight");
            break;
        }
    } break;
    }

    if (val.encoding != IntegerEncoding::JustBits && bitlen > 0) {
        // Decode the value...
        result = D * C + B;
        result ^= A;
        result = (A & 0x20) | (result >> 2);
    }

    assert(result < 64);

    // Change from [0,63] to [0,64]
    if (result > 32) {
        result += 1;
    }

    return result;
}

static void UnquantizeTexelWeights(u32 out[2][144], const std::vector<IntegerEncodedValue>& weights,
                                   const TexelWeightParams& params, const u32 blockWidth,
                                   const u32 blockHeight) {
    u32 weightIdx = 0;
    u32 unquantized[2][144];

    for (auto itr = weights.begin(); itr != weights.end(); ++itr) {
        unquantized[0][weightIdx] = UnquantizeTexelWeight(*itr);

        if (params.m_bDualPlane) {
            ++itr;
            unquantized[1][weightIdx] = UnquantizeTexelWeight(*itr);
            if (itr == weights.end()) {
                break;
            }
        }

        if (++weightIdx >= (params.m_Width * params.m_Height))
            break;
    }

    // Do infill if necessary (Section C.2.18) ...
    u32 Ds = (1024 + (blockWidth / 2)) / (blockWidth - 1);
    u32 Dt = (1024 + (blockHeight / 2)) / (blockHeight - 1);

    const u32 kPlaneScale = params.m_bDualPlane ? 2U : 1U;
    for (u32 plane = 0; plane < kPlaneScale; plane++)
        for (u32 t = 0; t < blockHeight; t++)
            for (u32 s = 0; s < blockWidth; s++) {
                u32 cs = Ds * s;
                u32 ct = Dt * t;

                u32 gs = (cs * (params.m_Width - 1) + 32) >> 6;
                u32 gt = (ct * (params.m_Height - 1) + 32) >> 6;

                u32 js = gs >> 4;
                u32 fs = gs & 0xF;

                u32 jt = gt >> 4;
                u32 ft = gt & 0x0F;

                u32 w11 = (fs * ft + 8) >> 4;
                u32 w10 = ft - w11;
                u32 w01 = fs - w11;
                u32 w00 = 16 - fs - ft + w11;

                u32 v0 = js + jt * params.m_Width;

#define FIND_TEXEL(tidx, bidx)                                                                     \
    u32 p##bidx = 0;                                                                               \
    do {                                                                                           \
        if ((tidx) < (params.m_Width * params.m_Height)) {                                         \
            p##bidx = unquantized[plane][(tidx)];                                                  \
        }                                                                                          \
    } while (0)

                FIND_TEXEL(v0, 00);
                FIND_TEXEL(v0 + 1, 01);
                FIND_TEXEL(v0 + params.m_Width, 10);
                FIND_TEXEL(v0 + params.m_Width + 1, 11);

#undef FIND_TEXEL

                out[plane][t * blockWidth + s] =
                    (p00 * w00 + p01 * w01 + p10 * w10 + p11 * w11 + 8) >> 4;
            }
}

// Transfers a bit as described in C.2.14
static inline void BitTransferSigned(s32& a, s32& b) {
    b >>= 1;
    b |= a & 0x80;
    a >>= 1;
    a &= 0x3F;
    if (a & 0x20)
        a -= 0x40;
}

// Adds more precision to the blue channel as described
// in C.2.14
static inline Pixel BlueContract(s32 a, s32 r, s32 g, s32 b) {
    return Pixel(static_cast<s16>(a), static_cast<s16>((r + b) >> 1),
                 static_cast<s16>((g + b) >> 1), static_cast<s16>(b));
}

// Partition selection functions as specified in
// C.2.21
static inline u32 hash52(u32 p) {
    p ^= p >> 15;
    p -= p << 17;
    p += p << 7;
    p += p << 4;
    p ^= p >> 5;
    p += p << 16;
    p ^= p >> 7;
    p ^= p >> 3;
    p ^= p << 6;
    p ^= p >> 17;
    return p;
}

static u32 SelectPartition(s32 seed, s32 x, s32 y, s32 z, s32 partitionCount, s32 smallBlock) {
    if (1 == partitionCount)
        return 0;

    if (smallBlock) {
        x <<= 1;
        y <<= 1;
        z <<= 1;
    }

    seed += (partitionCount - 1) * 1024;

    u32 rnum = hash52(static_cast<u32>(seed));
    u8 seed1 = static_cast<u8>(rnum & 0xF);
    u8 seed2 = static_cast<u8>((rnum >> 4) & 0xF);
    u8 seed3 = static_cast<u8>((rnum >> 8) & 0xF);
    u8 seed4 = static_cast<u8>((rnum >> 12) & 0xF);
    u8 seed5 = static_cast<u8>((rnum >> 16) & 0xF);
    u8 seed6 = static_cast<u8>((rnum >> 20) & 0xF);
    u8 seed7 = static_cast<u8>((rnum >> 24) & 0xF);
    u8 seed8 = static_cast<u8>((rnum >> 28) & 0xF);
    u8 seed9 = static_cast<u8>((rnum >> 18) & 0xF);
    u8 seed10 = static_cast<u8>((rnum >> 22) & 0xF);
    u8 seed11 = static_cast<u8>((rnum >> 26) & 0xF);
    u8 seed12 = static_cast<u8>(((rnum >> 30) | (rnum << 2)) & 0xF);

    seed1 = static_cast<u8>(seed1 * seed1);
    seed2 = static_cast<u8>(seed2 * seed2);
    seed3 = static_cast<u8>(seed3 * seed3);
    seed4 = static_cast<u8>(seed4 * seed4);
    seed5 = static_cast<u8>(seed5 * seed5);
    seed6 = static_cast<u8>(seed6 * seed6);
    seed7 = static_cast<u8>(seed7 * seed7);
    seed8 = static_cast<u8>(seed8 * seed8);
    seed9 = static_cast<u8>(seed9 * seed9);
    seed10 = static_cast<u8>(seed10 * seed10);
    seed11 = static_cast<u8>(seed11 * seed11);
    seed12 = static_cast<u8>(seed12 * seed12);

    s32 sh1, sh2, sh3;
    if (seed & 1) {
        sh1 = (seed & 2) ? 4 : 5;
        sh2 = (partitionCount == 3) ? 6 : 5;
    } else {
        sh1 = (partitionCount == 3) ? 6 : 5;
        sh2 = (seed & 2) ? 4 : 5;
    }
    sh3 = (seed & 0x10) ? sh1 : sh2;

    seed1 = static_cast<u8>(seed1 >> sh1);
    seed2 = static_cast<u8>(seed2 >> sh2);
    seed3 = static_cast<u8>(seed3 >> sh1);
    seed4 = static_cast<u8>(seed4 >> sh2);
    seed5 = static_cast<u8>(seed5 >> sh1);
    seed6 = static_cast<u8>(seed6 >> sh2);
    seed7 = static_cast<u8>(seed7 >> sh1);
    seed8 = static_cast<u8>(seed8 >> sh2);
    seed9 = static_cast<u8>(seed9 >> sh3);
    seed10 = static_cast<u8>(seed10 >> sh3);
    seed11 = static_cast<u8>(seed11 >> sh3);
    seed12 = static_cast<u8>(seed12 >> sh3);

    s32 a = seed1 * x + seed2 * y + seed11 * z + (rnum >> 14);
    s32 b = seed3 * x + seed4 * y + seed12 * z + (rnum >> 10);
    s32 c = seed5 * x + seed6 * y + seed9 * z + (rnum >> 6);
    s32 d = seed7 * x + seed8 * y + seed10 * z + (rnum >> 2);

    a &= 0x3F;
    b &= 0x3F;
    c &= 0x3F;
    d &= 0x3F;

    if (partitionCount < 4)
        d = 0;
    if (partitionCount < 3)
        c = 0;

    if (a >= b && a >= c && a >= d)
        return 0;
    else if (b >= c && b >= d)
        return 1;
    else if (c >= d)
        return 2;
    return 3;
}

static inline u32 Select2DPartition(s32 seed, s32 x, s32 y, s32 partitionCount, s32 smallBlock) {
    return SelectPartition(seed, x, y, 0, partitionCount, smallBlock);
}

// Section C.2.14
static void ComputeEndpos32s(Pixel& ep1, Pixel& ep2, const u32*& colorValues,
                             u32 colorEndpos32Mode) {
#define READ_UINT_VALUES(N)                                                                        \
    u32 v[N];                                                                                      \
    for (u32 i = 0; i < N; i++) {                                                                  \
        v[i] = *(colorValues++);                                                                   \
    }

#define READ_INT_VALUES(N)                                                                         \
    s32 v[N];                                                                                      \
    for (u32 i = 0; i < N; i++) {                                                                  \
        v[i] = static_cast<s32>(*(colorValues++));                                                 \
    }

    switch (colorEndpos32Mode) {
    case 0: {
        READ_UINT_VALUES(2)
        ep1 = Pixel(0xFF, v[0], v[0], v[0]);
        ep2 = Pixel(0xFF, v[1], v[1], v[1]);
    } break;

    case 1: {
        READ_UINT_VALUES(2)
        u32 L0 = (v[0] >> 2) | (v[1] & 0xC0);
        u32 L1 = std::max(L0 + (v[1] & 0x3F), 0xFFU);
        ep1 = Pixel(0xFF, L0, L0, L0);
        ep2 = Pixel(0xFF, L1, L1, L1);
    } break;

    case 4: {
        READ_UINT_VALUES(4)
        ep1 = Pixel(v[2], v[0], v[0], v[0]);
        ep2 = Pixel(v[3], v[1], v[1], v[1]);
    } break;

    case 5: {
        READ_INT_VALUES(4)
        BitTransferSigned(v[1], v[0]);
        BitTransferSigned(v[3], v[2]);
        ep1 = Pixel(v[2], v[0], v[0], v[0]);
        ep2 = Pixel(v[2] + v[3], v[0] + v[1], v[0] + v[1], v[0] + v[1]);
        ep1.ClampByte();
        ep2.ClampByte();
    } break;

    case 6: {
        READ_UINT_VALUES(4)
        ep1 = Pixel(0xFF, v[0] * v[3] >> 8, v[1] * v[3] >> 8, v[2] * v[3] >> 8);
        ep2 = Pixel(0xFF, v[0], v[1], v[2]);
    } break;

    case 8: {
        READ_UINT_VALUES(6)
        if (v[1] + v[3] + v[5] >= v[0] + v[2] + v[4]) {
            ep1 = Pixel(0xFF, v[0], v[2], v[4]);
            ep2 = Pixel(0xFF, v[1], v[3], v[5]);
        } else {
            ep1 = BlueContract(0xFF, v[1], v[3], v[5]);
            ep2 = BlueContract(0xFF, v[0], v[2], v[4]);
        }
    } break;

    case 9: {
        READ_INT_VALUES(6)
        BitTransferSigned(v[1], v[0]);
        BitTransferSigned(v[3], v[2]);
        BitTransferSigned(v[5], v[4]);
        if (v[1] + v[3] + v[5] >= 0) {
            ep1 = Pixel(0xFF, v[0], v[2], v[4]);
            ep2 = Pixel(0xFF, v[0] + v[1], v[2] + v[3], v[4] + v[5]);
        } else {
            ep1 = BlueContract(0xFF, v[0] + v[1], v[2] + v[3], v[4] + v[5]);
            ep2 = BlueContract(0xFF, v[0], v[2], v[4]);
        }
        ep1.ClampByte();
        ep2.ClampByte();
    } break;

    case 10: {
        READ_UINT_VALUES(6)
        ep1 = Pixel(v[4], v[0] * v[3] >> 8, v[1] * v[3] >> 8, v[2] * v[3] >> 8);
        ep2 = Pixel(v[5], v[0], v[1], v[2]);
    } break;

    case 12: {
        READ_UINT_VALUES(8)
        if (v[1] + v[3] + v[5] >= v[0] + v[2] + v[4]) {
            ep1 = Pixel(v[6], v[0], v[2], v[4]);
            ep2 = Pixel(v[7], v[1], v[3], v[5]);
        } else {
            ep1 = BlueContract(v[7], v[1], v[3], v[5]);
            ep2 = BlueContract(v[6], v[0], v[2], v[4]);
        }
    } break;

    case 13: {
        READ_INT_VALUES(8)
        BitTransferSigned(v[1], v[0]);
        BitTransferSigned(v[3], v[2]);
        BitTransferSigned(v[5], v[4]);
        BitTransferSigned(v[7], v[6]);
        if (v[1] + v[3] + v[5] >= 0) {
            ep1 = Pixel(v[6], v[0], v[2], v[4]);
            ep2 = Pixel(v[7] + v[6], v[0] + v[1], v[2] + v[3], v[4] + v[5]);
        } else {
            ep1 = BlueContract(v[6] + v[7], v[0] + v[1], v[2] + v[3], v[4] + v[5]);
            ep2 = BlueContract(v[6], v[0], v[2], v[4]);
        }
        ep1.ClampByte();
        ep2.ClampByte();
    } break;

    default:
        assert(!"Unsupported color endpos32 mode (is it HDR?)");
        break;
    }

#undef READ_UINT_VALUES
#undef READ_INT_VALUES
}

static void DecompressBlock(const u8 inBuf[16], const u32 blockWidth, const u32 blockHeight,
                            u32* outBuf) {
    InputBitStream strm(inBuf);
    TexelWeightParams weightParams = DecodeBlockInfo(strm);

    // Was there an error?
    if (weightParams.m_bError) {
        assert(!"Invalid block mode");
        FillError(outBuf, blockWidth, blockHeight);
        return;
    }

    if (weightParams.m_bVoidExtentLDR) {
        FillVoidExtentLDR(strm, outBuf, blockWidth, blockHeight);
        return;
    }

    if (weightParams.m_bVoidExtentHDR) {
        assert(!"HDR void extent blocks are unsupported!");
        FillError(outBuf, blockWidth, blockHeight);
        return;
    }

    if (weightParams.m_Width > blockWidth) {
        assert(!"Texel weight grid width should be smaller than block width");
        FillError(outBuf, blockWidth, blockHeight);
        return;
    }

    if (weightParams.m_Height > blockHeight) {
        assert(!"Texel weight grid height should be smaller than block height");
        FillError(outBuf, blockWidth, blockHeight);
        return;
    }

    // Read num partitions
    u32 nPartitions = strm.ReadBits<2>() + 1;
    assert(nPartitions <= 4);

    if (nPartitions == 4 && weightParams.m_bDualPlane) {
        assert(!"Dual plane mode is incompatible with four partition blocks");
        FillError(outBuf, blockWidth, blockHeight);
        return;
    }

    // Based on the number of partitions, read the color endpos32 mode for
    // each partition.

    // Determine partitions, partition index, and color endpos32 modes
    s32 planeIdx = -1;
    u32 partitionIndex;
    u32 colorEndpos32Mode[4] = {0, 0, 0, 0};

    // Define color data.
    u8 colorEndpos32Data[16];
    memset(colorEndpos32Data, 0, sizeof(colorEndpos32Data));
    OutputBitStream colorEndpos32Stream(colorEndpos32Data, 16 * 8, 0);

    // Read extra config data...
    u32 baseCEM = 0;
    if (nPartitions == 1) {
        colorEndpos32Mode[0] = strm.ReadBits<4>();
        partitionIndex = 0;
    } else {
        partitionIndex = strm.ReadBits<10>();
        baseCEM = strm.ReadBits<6>();
    }
    u32 baseMode = (baseCEM & 3);

    // Remaining bits are color endpos32 data...
    u32 nWeightBits = weightParams.GetPackedBitSize();
    s32 remainingBits = 128 - nWeightBits - static_cast<s32>(strm.GetBitsRead());

    // Consider extra bits prior to texel data...
    u32 extraCEMbits = 0;
    if (baseMode) {
        switch (nPartitions) {
        case 2:
            extraCEMbits += 2;
            break;
        case 3:
            extraCEMbits += 5;
            break;
        case 4:
            extraCEMbits += 8;
            break;
        default:
            assert(false);
            break;
        }
    }
    remainingBits -= extraCEMbits;

    // Do we have a dual plane situation?
    u32 planeSelectorBits = 0;
    if (weightParams.m_bDualPlane) {
        planeSelectorBits = 2;
    }
    remainingBits -= planeSelectorBits;

    // Read color data...
    u32 colorDataBits = remainingBits;
    while (remainingBits > 0) {
        u32 nb = std::min(remainingBits, 8);
        u32 b = strm.ReadBits(nb);
        colorEndpos32Stream.WriteBits(b, nb);
        remainingBits -= 8;
    }

    // Read the plane selection bits
    planeIdx = strm.ReadBits(planeSelectorBits);

    // Read the rest of the CEM
    if (baseMode) {
        u32 extraCEM = strm.ReadBits(extraCEMbits);
        u32 CEM = (extraCEM << 6) | baseCEM;
        CEM >>= 2;

        bool C[4] = {0};
        for (u32 i = 0; i < nPartitions; i++) {
            C[i] = CEM & 1;
            CEM >>= 1;
        }

        u8 M[4] = {0};
        for (u32 i = 0; i < nPartitions; i++) {
            M[i] = CEM & 3;
            CEM >>= 2;
            assert(M[i] <= 3);
        }

        for (u32 i = 0; i < nPartitions; i++) {
            colorEndpos32Mode[i] = baseMode;
            if (!(C[i]))
                colorEndpos32Mode[i] -= 1;
            colorEndpos32Mode[i] <<= 2;
            colorEndpos32Mode[i] |= M[i];
        }
    } else if (nPartitions > 1) {
        u32 CEM = baseCEM >> 2;
        for (u32 i = 0; i < nPartitions; i++) {
            colorEndpos32Mode[i] = CEM;
        }
    }

    // Make sure everything up till here is sane.
    for (u32 i = 0; i < nPartitions; i++) {
        assert(colorEndpos32Mode[i] < 16);
    }
    assert(strm.GetBitsRead() + weightParams.GetPackedBitSize() == 128);

    // Decode both color data and texel weight data
    u32 colorValues[32]; // Four values, two endpos32s, four maximum paritions
    DecodeColorValues(colorValues, colorEndpos32Data, colorEndpos32Mode, nPartitions,
                      colorDataBits);

    Pixel endpos32s[4][2];
    const u32* colorValuesPtr = colorValues;
    for (u32 i = 0; i < nPartitions; i++) {
        ComputeEndpos32s(endpos32s[i][0], endpos32s[i][1], colorValuesPtr, colorEndpos32Mode[i]);
    }

    // Read the texel weight data..
    u8 texelWeightData[16];
    memcpy(texelWeightData, inBuf, sizeof(texelWeightData));

    // Reverse everything
    for (u32 i = 0; i < 8; i++) {
// Taken from http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
#define REVERSE_BYTE(b) (((b)*0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32
        u8 a = static_cast<u8>(REVERSE_BYTE(texelWeightData[i]));
        u8 b = static_cast<u8>(REVERSE_BYTE(texelWeightData[15 - i]));
#undef REVERSE_BYTE

        texelWeightData[i] = b;
        texelWeightData[15 - i] = a;
    }

    // Make sure that higher non-texel bits are set to zero
    const u32 clearByteStart = (weightParams.GetPackedBitSize() >> 3) + 1;
    texelWeightData[clearByteStart - 1] =
        texelWeightData[clearByteStart - 1] &
        static_cast<u8>((1 << (weightParams.GetPackedBitSize() % 8)) - 1);
    memset(texelWeightData + clearByteStart, 0, 16 - clearByteStart);

    std::vector<IntegerEncodedValue> texelWeightValues;
    texelWeightValues.reserve(64);

    InputBitStream weightStream(texelWeightData);

    DecodeIntegerSequence(texelWeightValues, weightStream, weightParams.m_MaxWeight,
                          weightParams.GetNumWeightValues());

    // Blocks can be at most 12x12, so we can have as many as 144 weights
    u32 weights[2][144];
    UnquantizeTexelWeights(weights, texelWeightValues, weightParams, blockWidth, blockHeight);

    // Now that we have endpos32s and weights, we can s32erpolate and generate
    // the proper decoding...
    for (u32 j = 0; j < blockHeight; j++)
        for (u32 i = 0; i < blockWidth; i++) {
            u32 partition = Select2DPartition(partitionIndex, i, j, nPartitions,
                                              (blockHeight * blockWidth) < 32);
            assert(partition < nPartitions);

            Pixel p;
            for (u32 c = 0; c < 4; c++) {
                u32 C0 = endpos32s[partition][0].Component(c);
                C0 = Replicate(C0, 8, 16);
                u32 C1 = endpos32s[partition][1].Component(c);
                C1 = Replicate(C1, 8, 16);

                u32 plane = 0;
                if (weightParams.m_bDualPlane && (((planeIdx + 1) & 3) == c)) {
                    plane = 1;
                }

                u32 weight = weights[plane][j * blockWidth + i];
                u32 C = (C0 * (64 - weight) + C1 * weight + 32) / 64;
                if (C == 65535) {
                    p.Component(c) = 255;
                } else {
                    double Cf = static_cast<double>(C);
                    p.Component(c) = static_cast<u16>(255.0 * (Cf / 65536.0) + 0.5);
                }
            }

            outBuf[j * blockWidth + i] = p.Pack();
        }
}

} // namespace ASTCC

namespace Tegra::Texture::ASTC {

std::vector<u8> Decompress(const u8* data, u32 width, u32 height, u32 depth, u32 block_width,
                           u32 block_height) {
    u32 blockIdx = 0;
    std::size_t depth_offset = 0;
    std::vector<u8> outData(height * width * depth * 4);
    for (u32 k = 0; k < depth; k++) {
        for (u32 j = 0; j < height; j += block_height) {
            for (u32 i = 0; i < width; i += block_width) {

                const u8* blockPtr = data + blockIdx * 16;

                // Blocks can be at most 12x12
                u32 uncompData[144];
                ASTCC::DecompressBlock(blockPtr, block_width, block_height, uncompData);

                u32 decompWidth = std::min(block_width, width - i);
                u32 decompHeight = std::min(block_height, height - j);

                u8* outRow = depth_offset + outData.data() + (j * width + i) * 4;
                for (u32 jj = 0; jj < decompHeight; jj++) {
                    memcpy(outRow + jj * width * 4, uncompData + jj * block_width, decompWidth * 4);
                }

                blockIdx++;
            }
        }
        depth_offset += height * width * 4;
    }

    return outData;
}

} // namespace Tegra::Texture::ASTC
