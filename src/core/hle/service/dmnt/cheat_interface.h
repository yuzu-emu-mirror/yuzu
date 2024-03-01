// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/hle/service/cmif_types.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

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
struct CheatDefinition;
struct CheatEntry;
struct CheatProcessMetadata;
struct FrozenAddressEntry;
class CheatProcessManager;

class ICheatInterface final : public ServiceFramework<ICheatInterface> {
public:
    explicit ICheatInterface(Core::System& system_, CheatProcessManager& manager);
    ~ICheatInterface() override;

private:
    Result HasCheatProcess(Out<bool> out_has_cheat);
    Result GetCheatProcessEvent(OutCopyHandle<Kernel::KReadableEvent> out_event);
    Result GetCheatProcessMetadata(Out<CheatProcessMetadata> out_metadata);
    Result ForceOpenCheatProcess();
    Result PauseCheatProcess();
    Result ResumeCheatProcess();
    Result ForceCloseCheatProcess();

    Result GetCheatProcessMappingCount(Out<u64> out_count);
    Result GetCheatProcessMappings(
        Out<u64> out_count, u64 offset,
        OutArray<Kernel::Svc::MemoryInfo, BufferAttr_HipcMapAlias> out_mappings);
    Result ReadCheatProcessMemory(u64 address, u64 size,
                                  OutBuffer<BufferAttr_HipcMapAlias> out_buffer);
    Result WriteCheatProcessMemory(u64 address, u64 size, InBuffer<BufferAttr_HipcMapAlias> buffer);

    Result QueryCheatProcessMemory(Out<Kernel::Svc::MemoryInfo> out_mapping, u64 address);
    Result GetCheatCount(Out<u64> out_count);
    Result GetCheats(Out<u64> out_count, u64 offset,
                     OutArray<CheatEntry, BufferAttr_HipcMapAlias> out_cheats);
    Result GetCheatById(OutLargeData<CheatEntry, BufferAttr_HipcMapAlias> out_cheat, u32 cheat_id);
    Result ToggleCheat(u32 cheat_id);

    Result AddCheat(Out<u32> out_cheat_id, bool enabled,
                    InLargeData<CheatDefinition, BufferAttr_HipcMapAlias> cheat_definition);
    Result RemoveCheat(u32 cheat_id);
    Result ReadStaticRegister(Out<u64> out_value, u8 register_index);
    Result WriteStaticRegister(u8 register_index, u64 value);
    Result ResetStaticRegisters();
    Result SetMasterCheat(InLargeData<CheatDefinition, BufferAttr_HipcMapAlias> cheat_definition);
    Result GetFrozenAddressCount(Out<u64> out_count);
    Result GetFrozenAddresses(
        Out<u64> out_count, u64 offset,
        OutArray<FrozenAddressEntry, BufferAttr_HipcMapAlias> out_frozen_address);
    Result GetFrozenAddress(Out<FrozenAddressEntry> out_frozen_address_entry, u64 address);
    Result EnableFrozenAddress(Out<u64> out_value, u64 address, u64 width);
    Result DisableFrozenAddress(u64 address);

private:
    void InitializeCheatManager();

    Result ReadCheatProcessMemoryUnsafe(u64 process_addr, std::span<u8> out_data, size_t size);
    Result WriteCheatProcessMemoryUnsafe(u64 process_addr, std::span<const u8> data, size_t size);

    Result PauseCheatProcessUnsafe();
    Result ResumeCheatProcessUnsafe();

    CheatProcessManager& cheat_process_manager;
};

} // namespace Service::DMNT
