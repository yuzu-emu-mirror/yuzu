// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

namespace Service::DMNT {

struct MemoryRegionExtents {
    u64 base{};
    u64 size{};
};
static_assert(sizeof(MemoryRegionExtents) == 0x10, "MemoryRegionExtents is an invalid size");

struct CheatProcessMetadata {
    u64 process_id{};
    u64 program_id{};
    MemoryRegionExtents main_nso_extents{};
    MemoryRegionExtents heap_extents{};
    MemoryRegionExtents alias_extents{};
    MemoryRegionExtents aslr_extents{};
    std::array<u8, 0x20> main_nso_build_id{};
};
static_assert(sizeof(CheatProcessMetadata) == 0x70, "CheatProcessMetadata is an invalid size");

struct CheatDefinition {
    std::array<char, 0x40> readable_name;
    u32 num_opcodes;
    std::array<u32, 0x100> opcodes;
};
static_assert(sizeof(CheatDefinition) == 0x444, "CheatDefinition is an invalid size");

struct CheatEntry {
    bool enabled;
    u32 cheat_id;
    CheatDefinition definition;
};
static_assert(sizeof(CheatEntry) == 0x44C, "CheatEntry is an invalid size");
static_assert(std::is_trivial_v<CheatEntry>, "CheatEntry type must be trivially copyable.");

struct FrozenAddressValue {
    u64 value;
    u8 width;
};
static_assert(sizeof(FrozenAddressValue) == 0x10, "FrozenAddressValue is an invalid size");

struct FrozenAddressEntry {
    u64 address;
    FrozenAddressValue value;
};
static_assert(sizeof(FrozenAddressEntry) == 0x18, "FrozenAddressEntry is an invalid size");

} // namespace Service::DMNT
