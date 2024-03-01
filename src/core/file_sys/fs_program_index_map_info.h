// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_funcs.h"

namespace FileSys {

struct ProgramIndexMapInfo {
    u64 program_id;
    u64 base_program_id;
    u8 program_index;
    INSERT_PADDING_BYTES(0xF);
};
static_assert(std::is_trivially_copyable_v<ProgramIndexMapInfo>,
              "Data type must be trivially copyable.");
static_assert(sizeof(ProgramIndexMapInfo) == 0x20, "ProgramIndexMapInfo has invalid size.");

} // namespace FileSys
