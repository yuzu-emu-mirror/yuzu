// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <map>
#include <span>

#include "core/hle/service/cmif_types.h"
#include "core/hle/service/dmnt/dmnt_types.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

#include "common/intrusive_red_black_tree.h"

namespace Core {
class System;
}

namespace Kernel {
class KEvent;
class KReadableEvent;
} // namespace Kernel

namespace Kernel::Svc {
struct MemoryInfo;
}

namespace Service::DMNT {
class CheatVirtualMachine;

class CheatProcessManager final {
public:
    static constexpr size_t MaxCheatCount = 0x80;
    static constexpr size_t MaxFrozenAddressCount = 0x80;

    CheatProcessManager(Core::System& system_);
    ~CheatProcessManager();

    void SetVirtualMachine(std::unique_ptr<CheatVirtualMachine> vm);

    bool HasCheatProcess();
    Kernel::KReadableEvent& GetCheatProcessEvent() const;
    Result GetCheatProcessMetadata(CheatProcessMetadata& out_metadata);
    Result AttachToApplicationProcess(const std::array<u8, 0x20>& build_id, VAddr main_region_begin,
                                      u64 main_region_size);
    Result ForceOpenCheatProcess();
    Result PauseCheatProcess();
    Result PauseCheatProcessUnsafe();
    Result ResumeCheatProcess();
    Result ResumeCheatProcessUnsafe();
    Result ForceCloseCheatProcess();

    Result GetCheatProcessMappingCount(u64& out_count);
    Result GetCheatProcessMappings(u64& out_count, u64 offset,
                                   std::span<Kernel::Svc::MemoryInfo> out_mappings);
    Result ReadCheatProcessMemory(u64 process_address, u64 size, std::span<u8> out_data);
    Result ReadCheatProcessMemoryUnsafe(u64 process_address, void* out_data, size_t size);
    Result WriteCheatProcessMemory(u64 process_address, u64 size, std::span<const u8> data);
    Result WriteCheatProcessMemoryUnsafe(u64 process_address, const void* data, size_t size);

    Result QueryCheatProcessMemory(Out<Kernel::Svc::MemoryInfo> mapping, u64 address);
    Result GetCheatCount(u64& out_count);
    Result GetCheats(u64& out_count, u64 offset, std::span<CheatEntry> out_cheats);
    Result GetCheatById(CheatEntry* out_cheat, u32 cheat_id);
    Result ToggleCheat(u32 cheat_id);

    Result AddCheat(u32& out_cheat_id, bool enabled, const CheatDefinition& cheat_definition);
    Result RemoveCheat(u32 cheat_id);
    Result ReadStaticRegister(u64& out_value, u64 register_index);
    Result WriteStaticRegister(u64 register_index, u64 value);
    Result ResetStaticRegisters();
    Result SetMasterCheat(const CheatDefinition& cheat_definition);
    Result GetFrozenAddressCount(u64& out_count);
    Result GetFrozenAddresses(u64& out_count, u64 offset,
                              std::span<FrozenAddressEntry> out_frozen_address);
    Result GetFrozenAddress(FrozenAddressEntry& out_frozen_address_entry, u64 address);
    Result EnableFrozenAddress(u64& out_value, u64 address, u64 width);
    Result DisableFrozenAddress(u64 address);

    u64 HidKeysDown() const;
    void DebugLog(u8 id, u64 value) const;
    void CommandLog(std::string_view data) const;

private:
    bool HasActiveCheatProcess();
    void CloseActiveCheatProcess();
    Result EnsureCheatProcess();
    void SetNeedsReloadVm(bool reload);
    void ResetCheatEntry(size_t i);
    void ResetAllCheatEntries();
    CheatEntry* GetCheatEntryById(size_t i);
    CheatEntry* GetCheatEntryByReadableName(const char* readable_name);
    CheatEntry* GetFreeCheatEntry();

    void FrameCallback(std::chrono::nanoseconds ns_late);

    static constexpr u64 InvalidHandle = 0;

    mutable std::mutex cheat_lock;
    Kernel::KEvent* unsafe_break_event;

    Kernel::KEvent* cheat_process_event;
    u64 cheat_process_debug_handle = InvalidHandle;
    CheatProcessMetadata cheat_process_metadata = {};

    bool broken_unsafe = false;
    bool needs_reload_vm = false;
    std::unique_ptr<CheatVirtualMachine> cheat_vm;

    bool enable_cheats_by_default = true;
    bool always_save_cheat_toggles = false;
    bool should_save_cheat_toggles = false;
    std::array<CheatEntry, MaxCheatCount> cheat_entries = {};
    // TODO: Replace with IntrusiveRedBlackTree
    std::map<u64, FrozenAddressValue> frozen_addresses_map = {};

    Core::System& system;
    KernelHelpers::ServiceContext service_context;
    std::shared_ptr<Core::Timing::EventType> update_event;
    Core::Timing::CoreTiming& core_timing;
};

} // namespace Service::DMNT
