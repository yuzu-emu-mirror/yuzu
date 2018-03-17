// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <unordered_map>
#include "common/common_types.h"
#include "video_core/engines/fermi_2d.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/maxwell_compute.h"
#include "video_core/memory_manager.h"

namespace Tegra {

enum class EngineID {
    FERMI_TWOD_A = 0x902D, // 2D Engine
    MAXWELL_B = 0xB197,    // 3D Engine
    MAXWELL_COMPUTE_B = 0xB1C0,
    KEPLER_INLINE_TO_MEMORY_B = 0xA140,
    MAXWELL_DMA_COPY_A = 0xB0B5,
};

class GPU final {
public:
    GPU() {
        memory_manager = std::make_unique<MemoryManager>();
        maxwell_3d = std::make_unique<Engines::Maxwell3D>(*memory_manager);
        fermi_2d = std::make_unique<Engines::Fermi2D>();
        maxwell_compute = std::make_unique<Engines::MaxwellCompute>();
    }
    ~GPU() = default;

    /// Processes a command list stored at the specified address in GPU memory.
    void ProcessCommandList(GPUVAddr address, u32 size);

    std::unique_ptr<MemoryManager> memory_manager;

private:
    /// Writes a single register in the engine bound to the specified subchannel
    void WriteReg(u32 method, u32 subchannel, u32 value);

    /// Calls a method in the engine bound to the specified subchannel with the input parameters.
    void CallMethod(u32 method, u32 subchannel, const std::vector<u32>& parameters);

    /// Mapping of command subchannels to their bound engine ids.
    std::unordered_map<u32, EngineID> bound_engines;

    /// 3D engine
    std::unique_ptr<Engines::Maxwell3D> maxwell_3d;
    /// 2D engine
    std::unique_ptr<Engines::Fermi2D> fermi_2d;
    /// Compute engine
    std::unique_ptr<Engines::MaxwellCompute> maxwell_compute;
};

} // namespace Tegra
