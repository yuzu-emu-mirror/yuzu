// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/memory.h"
#include "video_core/engines/maxwell_dma.h"
#include "video_core/textures/decoders.h"

namespace Tegra {
namespace Engines {

MaxwellDMA::MaxwellDMA(MemoryManager& memory_manager) : memory_manager(memory_manager) {}

void MaxwellDMA::WriteReg(u32 method, u32 value) {
    ASSERT_MSG(method < Regs::NUM_REGS,
               "Invalid MaxwellDMA register, increase the size of the Regs structure");

    regs.reg_array[method] = value;

#define MAXWELLDMA_REG_INDEX(field_name)                                                           \
    (offsetof(Tegra::Engines::MaxwellDMA::Regs, field_name) / sizeof(u32))

    switch (method) {
    case MAXWELLDMA_REG_INDEX(exec): {
        HandleCopy();
        break;
    }
    }

#undef MAXWELLDMA_REG_INDEX
}

void MaxwellDMA::HandleCopy() {
    NGLOG_WARNING(HW_GPU, "Requested a DMA copy");

    const GPUVAddr source = regs.src_address.Address();
    const GPUVAddr dest = regs.dst_address.Address();

    const VAddr source_cpu = *memory_manager.GpuToCpuAddress(source);
    const VAddr dest_cpu = *memory_manager.GpuToCpuAddress(dest);

    // TODO(Subv): Perform more research and implement all features of this engine.
    ASSERT(regs.exec.enable_swizzle == 0);
    ASSERT(regs.exec.enable_2d == 1);
    ASSERT(regs.exec.query_mode == Regs::QueryMode::None);
    ASSERT(regs.exec.query_intr == Regs::QueryIntr::None);
    ASSERT(regs.exec.copy_mode == Regs::CopyMode::Unk2);
    ASSERT(regs.src_params.pos_x == 0);
    ASSERT(regs.src_params.pos_y == 0);
    ASSERT(regs.dst_params.pos_x == 0);
    ASSERT(regs.dst_params.pos_y == 0);

    if (regs.exec.is_dst_linear == regs.exec.is_src_linear) {
        Memory::CopyBlock(dest_cpu, source_cpu, regs.x_count * regs.y_count);
        return;
    }

    u8* src_buffer = Memory::GetPointer(source_cpu);
    u8* dst_buffer = Memory::GetPointer(dest_cpu);

    if (regs.exec.is_dst_linear && !regs.exec.is_src_linear) {
        // If the input is tiled and the output is linear, deswizzle the input and copy it over.
        Texture::CopySwizzledData(regs.src_params.size_x, regs.src_params.size_y, 1, 1, src_buffer,
                                  dst_buffer, true, regs.src_params.BlockHeight());
    } else {
        // If the input is linear and the output is tiled, swizzle the input and copy it over.
        Texture::CopySwizzledData(regs.dst_params.size_x, regs.dst_params.size_y, 1, 1, dst_buffer,
                                  src_buffer, false, regs.dst_params.BlockHeight());
    }
}

} // namespace Engines
} // namespace Tegra
