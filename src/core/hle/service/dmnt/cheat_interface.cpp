// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/cmif_serialization.h"
#include "core/hle/service/dmnt/cheat_interface.h"
#include "core/hle/service/dmnt/cheat_process_manager.h"
#include "core/hle/service/dmnt/dmnt_results.h"
#include "core/hle/service/dmnt/dmnt_types.h"

namespace Service::DMNT {

ICheatInterface::ICheatInterface(Core::System& system_, CheatProcessManager& manager)
    : ServiceFramework{system_, "dmnt:cht"}, cheat_process_manager{manager} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {65000, C<&ICheatInterface::HasCheatProcess>, "HasCheatProcess"},
        {65001, C<&ICheatInterface::GetCheatProcessEvent>, "GetCheatProcessEvent"},
        {65002, C<&ICheatInterface::GetCheatProcessMetadata>, "GetCheatProcessMetadata"},
        {65003, C<&ICheatInterface::ForceOpenCheatProcess>, "ForceOpenCheatProcess"},
        {65004, C<&ICheatInterface::PauseCheatProcess>, "PauseCheatProcess"},
        {65005, C<&ICheatInterface::ResumeCheatProcess>, "ResumeCheatProcess"},
        {65006, C<&ICheatInterface::ForceCloseCheatProcess>, "ForceCloseCheatProcess"},
        {65100, C<&ICheatInterface::GetCheatProcessMappingCount>, "GetCheatProcessMappingCount"},
        {65101, C<&ICheatInterface::GetCheatProcessMappings>, "GetCheatProcessMappings"},
        {65102, C<&ICheatInterface::ReadCheatProcessMemory>, "ReadCheatProcessMemory"},
        {65103, C<&ICheatInterface::WriteCheatProcessMemory>, "WriteCheatProcessMemory"},
        {65104, C<&ICheatInterface::QueryCheatProcessMemory>, "QueryCheatProcessMemory"},
        {65200, C<&ICheatInterface::GetCheatCount>, "GetCheatCount"},
        {65201, C<&ICheatInterface::GetCheats>, "GetCheats"},
        {65202, C<&ICheatInterface::GetCheatById>, "GetCheatById"},
        {65203, C<&ICheatInterface::ToggleCheat>, "ToggleCheat"},
        {65204, C<&ICheatInterface::AddCheat>, "AddCheat"},
        {65205, C<&ICheatInterface::RemoveCheat>, "RemoveCheat"},
        {65206, C<&ICheatInterface::ReadStaticRegister>, "ReadStaticRegister"},
        {65207, C<&ICheatInterface::WriteStaticRegister>, "WriteStaticRegister"},
        {65208, C<&ICheatInterface::ResetStaticRegisters>, "ResetStaticRegisters"},
        {65209, C<&ICheatInterface::SetMasterCheat>, "SetMasterCheat"},
        {65300, C<&ICheatInterface::GetFrozenAddressCount>, "GetFrozenAddressCount"},
        {65301, C<&ICheatInterface::GetFrozenAddresses>, "GetFrozenAddresses"},
        {65302, C<&ICheatInterface::GetFrozenAddress>, "GetFrozenAddress"},
        {65303, C<&ICheatInterface::EnableFrozenAddress>, "EnableFrozenAddress"},
        {65304, C<&ICheatInterface::DisableFrozenAddress>, "DisableFrozenAddress"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

ICheatInterface::~ICheatInterface() = default;

Result ICheatInterface::HasCheatProcess(Out<bool> out_has_cheat) {
    LOG_INFO(CheatEngine, "called");
    *out_has_cheat = cheat_process_manager.HasCheatProcess();
    R_SUCCEED();
}

Result ICheatInterface::GetCheatProcessEvent(OutCopyHandle<Kernel::KReadableEvent> out_event) {
    LOG_INFO(CheatEngine, "called");
    *out_event = &cheat_process_manager.GetCheatProcessEvent();
    R_SUCCEED();
}

Result ICheatInterface::GetCheatProcessMetadata(Out<CheatProcessMetadata> out_metadata) {
    LOG_INFO(CheatEngine, "called");
    R_RETURN(cheat_process_manager.GetCheatProcessMetadata(*out_metadata));
}

Result ICheatInterface::ForceOpenCheatProcess() {
    LOG_INFO(CheatEngine, "called");
    R_UNLESS(R_SUCCEEDED(cheat_process_manager.ForceOpenCheatProcess()), ResultCheatNotAttached);
    R_SUCCEED();
}

Result ICheatInterface::PauseCheatProcess() {
    LOG_INFO(CheatEngine, "called");
    R_RETURN(cheat_process_manager.PauseCheatProcess());
}

Result ICheatInterface::ResumeCheatProcess() {
    LOG_INFO(CheatEngine, "called");
    R_RETURN(cheat_process_manager.ResumeCheatProcess());
}

Result ICheatInterface::ForceCloseCheatProcess() {
    LOG_WARNING(CheatEngine, "(STUBBED) called");
    R_RETURN(cheat_process_manager.ForceCloseCheatProcess());
}

Result ICheatInterface::GetCheatProcessMappingCount(Out<u64> out_count) {
    LOG_WARNING(CheatEngine, "(STUBBED) called");
    R_RETURN(cheat_process_manager.GetCheatProcessMappingCount(*out_count));
}

Result ICheatInterface::GetCheatProcessMappings(
    Out<u64> out_count, u64 offset,
    OutArray<Kernel::Svc::MemoryInfo, BufferAttr_HipcMapAlias> out_mappings) {
    LOG_INFO(CheatEngine, "called, offset={}", offset);
    R_UNLESS(!out_mappings.empty(), ResultCheatNullBuffer);
    R_RETURN(cheat_process_manager.GetCheatProcessMappings(*out_count, offset, out_mappings));
}

Result ICheatInterface::ReadCheatProcessMemory(u64 address, u64 size,
                                               OutBuffer<BufferAttr_HipcMapAlias> out_buffer) {
    LOG_DEBUG(CheatEngine, "called, address={}, size={}", address, size);
    R_UNLESS(!out_buffer.empty(), ResultCheatNullBuffer);
    R_RETURN(cheat_process_manager.ReadCheatProcessMemory(address, size, out_buffer));
}

Result ICheatInterface::WriteCheatProcessMemory(u64 address, u64 size,
                                                InBuffer<BufferAttr_HipcMapAlias> buffer) {
    LOG_DEBUG(CheatEngine, "called, address={}, size={}", address, size);
    R_UNLESS(!buffer.empty(), ResultCheatNullBuffer);
    R_RETURN(cheat_process_manager.WriteCheatProcessMemory(address, size, buffer));
}

Result ICheatInterface::QueryCheatProcessMemory(Out<Kernel::Svc::MemoryInfo> out_mapping,
                                                u64 address) {
    LOG_WARNING(CheatEngine, "(STUBBED) called, address={}", address);
    R_RETURN(cheat_process_manager.QueryCheatProcessMemory(out_mapping, address));
}

Result ICheatInterface::GetCheatCount(Out<u64> out_count) {
    LOG_INFO(CheatEngine, "called");
    R_RETURN(cheat_process_manager.GetCheatCount(*out_count));
}

Result ICheatInterface::GetCheats(Out<u64> out_count, u64 offset,
                                  OutArray<CheatEntry, BufferAttr_HipcMapAlias> out_cheats) {
    LOG_INFO(CheatEngine, "called, offset={}", offset);
    R_UNLESS(!out_cheats.empty(), ResultCheatNullBuffer);
    R_RETURN(cheat_process_manager.GetCheats(*out_count, offset, out_cheats));
}

Result ICheatInterface::GetCheatById(OutLargeData<CheatEntry, BufferAttr_HipcMapAlias> out_cheat,
                                     u32 cheat_id) {
    LOG_INFO(CheatEngine, "called, cheat_id={}", cheat_id);
    R_RETURN(cheat_process_manager.GetCheatById(out_cheat, cheat_id));
}

Result ICheatInterface::ToggleCheat(u32 cheat_id) {
    LOG_INFO(CheatEngine, "called, cheat_id={}", cheat_id);
    R_RETURN(cheat_process_manager.ToggleCheat(cheat_id));
}

Result ICheatInterface::AddCheat(
    Out<u32> out_cheat_id, bool is_enabled,
    InLargeData<CheatDefinition, BufferAttr_HipcMapAlias> cheat_definition) {
    LOG_INFO(CheatEngine, "called, is_enabled={}", is_enabled);
    R_RETURN(cheat_process_manager.AddCheat(*out_cheat_id, is_enabled, *cheat_definition));
}

Result ICheatInterface::RemoveCheat(u32 cheat_id) {
    LOG_INFO(CheatEngine, "called, cheat_id={}", cheat_id);
    R_RETURN(cheat_process_manager.RemoveCheat(cheat_id));
}

Result ICheatInterface::ReadStaticRegister(Out<u64> out_value, u8 register_index) {
    LOG_DEBUG(CheatEngine, "called, register_index={}", register_index);
    R_RETURN(cheat_process_manager.ReadStaticRegister(*out_value, register_index));
}

Result ICheatInterface::WriteStaticRegister(u8 register_index, u64 value) {
    LOG_DEBUG(CheatEngine, "called, register_index={, value={}", register_index, value);
    R_RETURN(cheat_process_manager.WriteStaticRegister(register_index, value));
}

Result ICheatInterface::ResetStaticRegisters() {
    LOG_INFO(CheatEngine, "called");
    R_RETURN(cheat_process_manager.ResetStaticRegisters());
}

Result ICheatInterface::SetMasterCheat(
    InLargeData<CheatDefinition, BufferAttr_HipcMapAlias> cheat_definition) {
    LOG_INFO(CheatEngine, "called, name={}, num_opcodes={}", cheat_definition->readable_name.data(),
             cheat_definition->num_opcodes);
    R_RETURN(cheat_process_manager.SetMasterCheat(*cheat_definition));
}

Result ICheatInterface::GetFrozenAddressCount(Out<u64> out_count) {
    LOG_INFO(CheatEngine, "called");
    R_RETURN(cheat_process_manager.GetFrozenAddressCount(*out_count));
}

Result ICheatInterface::GetFrozenAddresses(
    Out<u64> out_count, u64 offset,
    OutArray<FrozenAddressEntry, BufferAttr_HipcMapAlias> out_frozen_address) {
    LOG_INFO(CheatEngine, "called, offset={}", offset);
    R_UNLESS(!out_frozen_address.empty(), ResultCheatNullBuffer);
    R_RETURN(cheat_process_manager.GetFrozenAddresses(*out_count, offset, out_frozen_address));
}

Result ICheatInterface::GetFrozenAddress(Out<FrozenAddressEntry> out_frozen_address_entry,
                                         u64 address) {
    LOG_INFO(CheatEngine, "called, address={}", address);
    R_RETURN(cheat_process_manager.GetFrozenAddress(*out_frozen_address_entry, address));
}

Result ICheatInterface::EnableFrozenAddress(Out<u64> out_value, u64 address, u64 width) {
    LOG_INFO(CheatEngine, "called, address={}, width={}", address, width);
    R_UNLESS(width > 0, ResultFrozenAddressInvalidWidth);
    R_UNLESS(width <= sizeof(u64), ResultFrozenAddressInvalidWidth);
    R_UNLESS((width & (width - 1)) == 0, ResultFrozenAddressInvalidWidth);
    R_RETURN(cheat_process_manager.EnableFrozenAddress(*out_value, address, width));
}

Result ICheatInterface::DisableFrozenAddress(u64 address) {
    LOG_INFO(CheatEngine, "called, address={}", address);
    R_RETURN(cheat_process_manager.DisableFrozenAddress(address));
}

void ICheatInterface::InitializeCheatManager() {
    LOG_INFO(CheatEngine, "called");
}

Result ICheatInterface::ReadCheatProcessMemoryUnsafe(u64 process_addr, std::span<u8> out_data,
                                                     size_t size) {
    LOG_DEBUG(CheatEngine, "called, process_addr={}, size={}", process_addr, size);
    R_RETURN(cheat_process_manager.ReadCheatProcessMemoryUnsafe(process_addr, &out_data, size));
}

Result ICheatInterface::WriteCheatProcessMemoryUnsafe(u64 process_addr, std::span<const u8> data,
                                                      size_t size) {
    LOG_DEBUG(CheatEngine, "called, process_addr={}, size={}", process_addr, size);
    R_RETURN(cheat_process_manager.WriteCheatProcessMemoryUnsafe(process_addr, &data, size));
}

Result ICheatInterface::PauseCheatProcessUnsafe() {
    LOG_INFO(CheatEngine, "called");
    R_RETURN(cheat_process_manager.PauseCheatProcessUnsafe());
}

Result ICheatInterface::ResumeCheatProcessUnsafe() {
    LOG_INFO(CheatEngine, "called");
    R_RETURN(cheat_process_manager.ResumeCheatProcessUnsafe());
}

} // namespace Service::DMNT
