// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

// This files contains code from Ryujinx
// A copy of the code can be obtained from https://github.com/Ryujinx/Ryujinx
// The sections using code from Ryujinx are marked with a link to the original version

// MIT License
//
// Copyright (c) Ryujinx Team and Contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "common/bit_field.h"
#include "common/common_types.h"
#include "shader_recompiler/frontend/maxwell/translate/impl/common_funcs.h"
#include "shader_recompiler/frontend/maxwell/translate/impl/impl.h"

namespace Shader::Maxwell {
namespace {
// https://forums.developer.nvidia.com/t/reverse-lut-for-lop3-lut/110651
// Emulate GPU's LOP3.LUT (three-input logic op with 8-bit truth table)
IR::U32 ApplyLUT(IR::IREmitter& ir, const IR::U32& a, const IR::U32& b, const IR::U32& c,
                 u64 ttbl) {
    std::optional<IR::U32> value;

    // Encode into gray code.
    u32 map = ttbl & 1;
    map |= ((ttbl >> 1) & 1) << 4;
    map |= ((ttbl >> 2) & 1) << 1;
    map |= ((ttbl >> 3) & 1) << 5;
    map |= ((ttbl >> 4) & 1) << 3;
    map |= ((ttbl >> 5) & 1) << 7;
    map |= ((ttbl >> 6) & 1) << 2;
    map |= ((ttbl >> 7) & 1) << 6;

    u32 visited = 0;
    for (u32 index = 0; index < 8 && visited != 0xff; index++) {
        if ((map & (1 << index)) == 0) {
            continue;
        }

        const auto RotateLeft4 = [](u32 value, u32 shift) {
            return ((value << shift) | (value >> (4 - shift))) & 0xf;
        };

        u32 mask = 0;
        for (u32 size = 4; size != 0; size >>= 1) {
            mask = RotateLeft4((1 << size) - 1, index & 3) << (index & 4);

            if ((map & mask) == mask) {
                break;
            }
        }

        // The mask should wrap, if we are on the high row, shift to low etc.
        const u32 mask2 = (index & 4) != 0 ? mask >> 4 : mask << 4;

        if ((map & mask2) == mask2) {
            mask |= mask2;
        }

        if ((mask & visited) == mask) {
            continue;
        }

        const bool not_a = (mask & 0x33) != 0;
        const bool not_b = (mask & 0x99) != 0;
        const bool not_c = (mask & 0x0f) != 0;

        const bool a_changes = (mask & 0xcc) != 0 && not_a;
        const bool b_changes = (mask & 0x66) != 0 && not_b;
        const bool c_changes = (mask & 0xf0) != 0 && not_c;

        std::optional<IR::U32> local_value;

        const auto And = [&](const IR::U32& source, bool inverted) {
            IR::U32 result = inverted ? ir.BitwiseNot(source) : source;
            if (local_value) {
                local_value = ir.BitwiseAnd(*local_value, result);
            } else {
                local_value = result;
            }
        };

        if (!a_changes) {
            And(a, not_a);
        }

        if (!b_changes) {
            And(b, not_b);
        }

        if (!c_changes) {
            And(c, not_c);
        }

        if (value) {
            value = ir.BitwiseOr(*value, *local_value);
        } else {
            value = local_value;
        }
        visited |= mask;
    }
    return *value;
}

IR::U32 LOP3(TranslatorVisitor& v, u64 insn, const IR::U32& op_b, const IR::U32& op_c, u64 lut) {
    union {
        u64 insn;
        BitField<0, 8, IR::Reg> dest_reg;
        BitField<8, 8, IR::Reg> src_reg;
        BitField<47, 1, u64> cc;
    } const lop3{insn};

    if (lop3.cc != 0) {
        throw NotImplementedException("LOP3 CC");
    }

    const IR::U32 op_a{v.X(lop3.src_reg)};
    const IR::U32 result{ApplyLUT(v.ir, op_a, op_b, op_c, lut)};
    v.X(lop3.dest_reg, result);
    return result;
}

u64 GetLut48(u64 insn) {
    union {
        u64 raw;
        BitField<48, 8, u64> lut;
    } const lut{insn};
    return lut.lut;
}
} // Anonymous namespace

void TranslatorVisitor::LOP3_reg(u64 insn) {
    union {
        u64 insn;
        BitField<28, 8, u64> lut;
        BitField<38, 1, u64> x;
        BitField<36, 2, PredicateOp> pred_op;
        BitField<48, 3, IR::Pred> pred;
    } const lop3{insn};

    if (lop3.x != 0) {
        throw NotImplementedException("LOP3 X");
    }
    const IR::U32 result{LOP3(*this, insn, GetReg20(insn), GetReg39(insn), lop3.lut)};
    const IR::U1 pred_result{PredicateOperation(ir, result, lop3.pred_op)};
    ir.SetPred(lop3.pred, pred_result);
}

void TranslatorVisitor::LOP3_cbuf(u64 insn) {
    LOP3(*this, insn, GetCbuf(insn), GetReg39(insn), GetLut48(insn));
}

void TranslatorVisitor::LOP3_imm(u64 insn) {
    LOP3(*this, insn, GetImm20(insn), GetReg39(insn), GetLut48(insn));
}
} // namespace Shader::Maxwell
