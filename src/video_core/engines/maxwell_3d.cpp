// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "video_core/engines/maxwell_3d.h"

namespace Tegra {
namespace Engines {

const std::unordered_map<u32, Maxwell3D::MethodInfo> Maxwell3D::method_handlers = {
    {0xE24, {"PrepareShader", 5, &Maxwell3D::PrepareShader}},
};

Maxwell3D::Maxwell3D(MemoryManager& memory_manager) : memory_manager(memory_manager) {}

void Maxwell3D::CallMethod(u32 method, const std::vector<u32>& parameters) {
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

void Maxwell3D::PrepareShader(const std::vector<u32>& parameters) {}

} // namespace Engines
} // namespace Tegra
