// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "video_core/engines/fermi_2d.h"

namespace Tegra {
namespace Engines {

Fermi2D::Fermi2D(MemoryManager& memory_manager) : memory_manager(memory_manager) {}

void Fermi2D::WriteReg(u32 method, u32 value) {
    ASSERT_MSG(method < Regs::NUM_REGS,
               "Invalid Fermi2D register, increase the size of the Regs structure");
}

} // namespace Engines
} // namespace Tegra
