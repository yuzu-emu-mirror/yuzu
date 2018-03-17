// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "video_core/engines/maxwell_3d.h"

namespace Tegra {
namespace Engines {

const std::unordered_map<u32, Maxwell3D::MethodInfo> Maxwell3D::method_handlers = {
    {0xE24, {"SetShader", 5, &Maxwell3D::SetShader}},
};

Maxwell3D::Maxwell3D(MemoryManager& memory_manager) : memory_manager(memory_manager) {}

void Maxwell3D::CallMethod(u32 method, const std::vector<u32>& parameters) {
    // TODO(Subv): Write an interpreter for the macros uploaded via registers 0x45 and 0x47
    auto itr = method_handlers.find(method);
    if (itr == method_handlers.end()) {
        LOG_ERROR(HW_GPU, "Unhandled method call %08X", method);
        return;
    }

    ASSERT(itr->second.arguments == parameters.size());
    (this->*itr->second.handler)(parameters);
}

void Maxwell3D::WriteReg(u32 method, u32 value) {
    ASSERT_MSG(method < Regs::NUM_REGS,
               "Invalid Maxwell3D register, increase the size of the Regs structure");

    regs.reg_array[method] = value;

#define MAXWELL3D_REG_INDEX(field_name) (offsetof(Regs, field_name) / sizeof(u32))

    switch (method) {
    case MAXWELL3D_REG_INDEX(code_address.code_address_high):
    case MAXWELL3D_REG_INDEX(code_address.code_address_low): {
        // Note: For some reason games (like Puyo Puyo Tetris) seem to write 0 to the CODE_ADDRESS
        // register, we do not currently know if that's intended or a bug, so we assert it lest
        // stuff breaks in other places (like the shader address calculation).
        ASSERT_MSG(regs.code_address.CodeAddress() == 0, "Unexpected CODE_ADDRESS register value.");
        break;
    }
    case MAXWELL3D_REG_INDEX(draw.vertex_end_gl): {
        DrawArrays();
        break;
    }
    case MAXWELL3D_REG_INDEX(query.query_get): {
        ProcessQueryGet();
        break;
    }
    default:
        break;
    }

#undef MAXWELL3D_REG_INDEX
}

void Maxwell3D::ProcessQueryGet() {
    GPUVAddr sequence_address = regs.query.QueryAddress();
    // Since the sequence address is given as a GPU VAddr, we have to convert it to an application
    // VAddr before writing.
    VAddr address = memory_manager.PhysicalToVirtualAddress(sequence_address);

    switch (regs.query.query_get.mode) {
    case Regs::QueryMode::Write: {
        // Write the current query sequence to the sequence address.
        u32 sequence = regs.query.query_sequence;
        Memory::Write32(address, sequence);
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Query mode %u not implemented", regs.query.query_get.mode.Value());
    }
}

void Maxwell3D::DrawArrays() {
    LOG_WARNING(HW_GPU, "Game requested a DrawArrays, ignoring");
}

void Maxwell3D::SetShader(const std::vector<u32>& parameters) {
    /**
     * Parameters description:
     * [0] = Shader Program.
     * [1] = Unknown.
     * [2] = Offset to the start of the shader, after the 0x30 bytes header.
     * [3] = Shader Type.
     * [4] = Const Buffer Address >> 8.
     */
    auto shader_program = static_cast<Regs::ShaderProgram>(parameters[0]);
    // TODO(Subv): This address is probably an offset from the CODE_ADDRESS register.
    GPUVAddr address = parameters[2];
    auto shader_type = static_cast<Regs::ShaderType>(parameters[3]);
    GPUVAddr cb_address = parameters[4] << 8;

    auto& shader = state.shaders[static_cast<size_t>(shader_program)];
    shader.program = shader_program;
    shader.type = shader_type;
    shader.address = address;
    shader.cb_address = cb_address;
}

} // namespace Engines
} // namespace Tegra
