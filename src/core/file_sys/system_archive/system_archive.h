// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/file_sys/vfs_types.h"

namespace FileSys::SystemArchive {

constexpr u64 SYSTEM_ARCHIVE_BASE_TITLE_ID = 0x0100000000000800;
constexpr std::size_t SYSTEM_ARCHIVE_COUNT = 0x28;

VirtualFile SynthesizeSystemArchive(u64 title_id);

} // namespace FileSys::SystemArchive
