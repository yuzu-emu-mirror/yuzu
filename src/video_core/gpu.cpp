// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "video_core/engines/fermi_2d.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/maxwell_compute.h"
#include "video_core/gpu.h"

namespace Tegra {

GPU::GPU() {
    memory_manager = std::make_unique<MemoryManager>();
    maxwell_3d = std::make_unique<Engines::Maxwell3D>(*memory_manager);
    fermi_2d = std::make_unique<Engines::Fermi2D>();
    maxwell_compute = std::make_unique<Engines::MaxwellCompute>();
}

GPU::~GPU() = default;

const Tegra::Engines::Maxwell3D& GPU::Get3DEngine() const {
    return *maxwell_3d;
}

} // namespace Tegra
