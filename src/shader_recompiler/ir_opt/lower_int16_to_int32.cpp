// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/ir/value.h"
#include "shader_recompiler/ir_opt/passes.h"

namespace Shader::Optimization {
namespace {
IR::Opcode Replace(IR::Opcode op) {
    switch (op) {
    case IR::Opcode::GetCbufU16:
    case IR::Opcode::GetCbufS16:
        return IR::Opcode::GetCbufU32;
    case IR::Opcode::UndefU16:
        return IR::Opcode::UndefU32;
    case IR::Opcode::LoadGlobalU16:
    case IR::Opcode::LoadGlobalS16:
        return IR::Opcode::LoadGlobal32;
    case IR::Opcode::WriteGlobalU16:
    case IR::Opcode::WriteGlobalS16:
        return IR::Opcode::WriteGlobal32;
    case IR::Opcode::LoadStorageU16:
    case IR::Opcode::LoadStorageS16:
        return IR::Opcode::LoadStorage32;
    case IR::Opcode::WriteStorageU16:
    case IR::Opcode::WriteStorageS16:
        return IR::Opcode::WriteStorage32;
    case IR::Opcode::LoadSharedU16:
    case IR::Opcode::LoadSharedS16:
        return IR::Opcode::LoadSharedU32;
    case IR::Opcode::WriteSharedU16:
        return IR::Opcode::WriteSharedU32;
    case IR::Opcode::SelectU16:
        return IR::Opcode::SelectU32;
    case IR::Opcode::BitCastU16F16:
        return IR::Opcode::BitCastU32F32;
    case IR::Opcode::BitCastF16U16:
        return IR::Opcode::BitCastF32U32;
    case IR::Opcode::ConvertS16F16:
    case IR::Opcode::ConvertS16F32:
        return IR::Opcode::ConvertS32F32;
    case IR::Opcode::ConvertS16F64:
        return IR::Opcode::ConvertS32F64;
    case IR::Opcode::ConvertU16F16:
    case IR::Opcode::ConvertU16F32:
        return IR::Opcode::ConvertU32F32;
    case IR::Opcode::ConvertU16F64:
        return IR::Opcode::ConvertU32F64;
    case IR::Opcode::ConvertF16S16:
    case IR::Opcode::ConvertF32S16:
        return IR::Opcode::ConvertF32S32;
    case IR::Opcode::ConvertF16U16:
    case IR::Opcode::ConvertF32U16:
        return IR::Opcode::ConvertF32U32;
    case IR::Opcode::ConvertF64S16:
    case IR::Opcode::ConvertF64U16:
        return IR::Opcode::ConvertF64U32;
    default:
        return op;
    }
}
} // Anonymous namespace

void LowerInt16ToInt32(IR::Program& program) {
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            inst.ReplaceOpcode(Replace(inst.GetOpcode()));
        }
    }
}

} // namespace Shader::Optimization
