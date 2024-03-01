// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/arm/debug.h"
#include "core/core_timing.h"
#include "core/hle/service/cmif_serialization.h"
#include "core/hle/service/dmnt/cheat_process_manager.h"
#include "core/hle/service/dmnt/cheat_virtual_machine.h"
#include "core/hle/service/dmnt/dmnt_results.h"
#include "core/hle/service/hid/hid_server.h"
#include "core/hle/service/sm/sm.h"
#include "hid_core/resource_manager.h"
#include "hid_core/resources/npad/npad.h"

namespace Service::DMNT {
constexpr auto CHEAT_ENGINE_NS = std::chrono::nanoseconds{1000000000 / 12};

CheatProcessManager::CheatProcessManager(Core::System& system_)
    : system{system_}, service_context{system_, "dmnt:cht"}, core_timing{system_.CoreTiming()} {
    update_event = Core::Timing::CreateEvent("CheatEngine::FrameCallback",
                                             [this](s64 time, std::chrono::nanoseconds ns_late)
                                                 -> std::optional<std::chrono::nanoseconds> {
                                                 FrameCallback(ns_late);
                                                 return std::nullopt;
                                             });

    for (size_t i = 0; i < MaxCheatCount; i++) {
        ResetCheatEntry(i);
    }

    cheat_process_event = service_context.CreateEvent("CheatProcessManager::ProcessEvent");
    unsafe_break_event = service_context.CreateEvent("CheatProcessManager::ProcessEvent");
}

CheatProcessManager::~CheatProcessManager() {
    service_context.CloseEvent(cheat_process_event);
    service_context.CloseEvent(unsafe_break_event);
    core_timing.UnscheduleEvent(update_event);
}

void CheatProcessManager::SetVirtualMachine(std::unique_ptr<CheatVirtualMachine> vm) {
    cheat_vm = std::move(vm);
    SetNeedsReloadVm(true);
}

bool CheatProcessManager::HasActiveCheatProcess() {
    // Note: This function *MUST* be called only with the cheat lock held.
    bool has_cheat_process =
        cheat_process_debug_handle != InvalidHandle &&
        system.ApplicationProcess()->GetProcessId() == cheat_process_metadata.process_id;

    if (!has_cheat_process) {
        CloseActiveCheatProcess();
    }

    return has_cheat_process;
}

void CheatProcessManager::CloseActiveCheatProcess() {
    if (cheat_process_debug_handle != InvalidHandle) {
        // We don't need to do any unsafe breaking.
        broken_unsafe = false;
        unsafe_break_event->Signal();
        core_timing.UnscheduleEvent(update_event);

        // Close resources.
        cheat_process_debug_handle = InvalidHandle;

        // Save cheat toggles.
        if (always_save_cheat_toggles || should_save_cheat_toggles) {
            // TODO: save cheat toggles
            should_save_cheat_toggles = false;
        }

        // Clear metadata.
        cheat_process_metadata = {};

        // Clear cheat list.
        ResetAllCheatEntries();

        // Clear frozen addresses.
        {
            auto it = frozen_addresses_map.begin();
            while (it != frozen_addresses_map.end()) {
                it = frozen_addresses_map.erase(it);
            }
        }

        // Signal to our fans.
        cheat_process_event->Signal();
    }
}

Result CheatProcessManager::EnsureCheatProcess() {
    R_UNLESS(HasActiveCheatProcess(), ResultCheatNotAttached);
    R_SUCCEED();
}

void CheatProcessManager::SetNeedsReloadVm(bool reload) {
    needs_reload_vm = reload;
}

void CheatProcessManager::ResetCheatEntry(size_t i) {
    if (i < MaxCheatCount) {
        cheat_entries[i] = {};
        cheat_entries[i].cheat_id = static_cast<u32>(i);

        SetNeedsReloadVm(true);
    }
}

void CheatProcessManager::ResetAllCheatEntries() {
    for (size_t i = 0; i < MaxCheatCount; i++) {
        ResetCheatEntry(i);
    }

    cheat_vm->ResetStaticRegisters();
}

CheatEntry* CheatProcessManager::GetCheatEntryById(size_t i) {
    if (i < MaxCheatCount) {
        return cheat_entries.data() + i;
    }

    return nullptr;
}

CheatEntry* CheatProcessManager::GetCheatEntryByReadableName(const char* readable_name) {
    // Check all non-master cheats for match.
    for (size_t i = 1; i < MaxCheatCount; i++) {
        if (std::strncmp(cheat_entries[i].definition.readable_name.data(), readable_name,
                         sizeof(cheat_entries[i].definition.readable_name)) == 0) {
            return cheat_entries.data() + i;
        }
    }

    return nullptr;
}

CheatEntry* CheatProcessManager::GetFreeCheatEntry() {
    // Check all non-master cheats for availability.
    for (size_t i = 1; i < MaxCheatCount; i++) {
        if (cheat_entries[i].definition.num_opcodes == 0) {
            return cheat_entries.data() + i;
        }
    }

    return nullptr;
}

bool CheatProcessManager::HasCheatProcess() {
    std::scoped_lock lk(cheat_lock);
    return HasActiveCheatProcess();
}

Kernel::KReadableEvent& CheatProcessManager::GetCheatProcessEvent() const {
    return cheat_process_event->GetReadableEvent();
}

Result CheatProcessManager::AttachToApplicationProcess(const std::array<u8, 0x20>& build_id,
                                                       VAddr main_region_begin,
                                                       u64 main_region_size) {
    std::scoped_lock lk(cheat_lock);

    // Close the active process, if needed.
    {
        if (this->HasActiveCheatProcess()) {
            // When forcing attach, we're done.
            this->CloseActiveCheatProcess();
        }
    }

    // Get the application process's ID.
    cheat_process_metadata.process_id = system.ApplicationProcess()->GetProcessId();

    // Get process handle, use it to learn memory extents.
    {
        const auto& page_table = system.ApplicationProcess()->GetPageTable();
        cheat_process_metadata.program_id = system.GetApplicationProcessProgramID();
        cheat_process_metadata.heap_extents = {
            .base = GetInteger(page_table.GetHeapRegionStart()),
            .size = page_table.GetHeapRegionSize(),
        };
        cheat_process_metadata.aslr_extents = {
            .base = GetInteger(page_table.GetAliasCodeRegionStart()),
            .size = page_table.GetAliasCodeRegionSize(),
        };
        cheat_process_metadata.alias_extents = {
            .base = GetInteger(page_table.GetAliasRegionStart()),
            .size = page_table.GetAliasRegionSize(),
        };
    }

    // Get module information from loader.
    {
        cheat_process_metadata.main_nso_extents = {
            .base = main_region_begin,
            .size = main_region_size,
        };
        cheat_process_metadata.main_nso_build_id = build_id;
    }

    // Set our debug handle.
    cheat_process_debug_handle = cheat_process_metadata.process_id;

    // Reset broken state.
    broken_unsafe = false;
    unsafe_break_event->Signal();

    // start the process.
    core_timing.ScheduleLoopingEvent(CHEAT_ENGINE_NS, CHEAT_ENGINE_NS, update_event);

    // Signal to our fans.
    cheat_process_event->Signal();

    R_SUCCEED();
}

Result CheatProcessManager::GetCheatProcessMetadata(CheatProcessMetadata& out_metadata) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    out_metadata = cheat_process_metadata;
    R_SUCCEED();
}

Result CheatProcessManager::ForceOpenCheatProcess() {
    // R_RETURN(AttachToApplicationProcess(false));
    R_SUCCEED();
}

Result CheatProcessManager::PauseCheatProcess() {
    std::scoped_lock lk(cheat_lock);

    R_TRY(EnsureCheatProcess());
    R_RETURN(PauseCheatProcessUnsafe());
}

Result CheatProcessManager::PauseCheatProcessUnsafe() {
    broken_unsafe = true;
    unsafe_break_event->Clear();
    if (system.ApplicationProcess()->IsSuspended()) {
        R_SUCCEED();
    }
    R_RETURN(system.ApplicationProcess()->SetActivity(Kernel::Svc::ProcessActivity::Paused));
}

Result CheatProcessManager::ResumeCheatProcess() {
    std::scoped_lock lk(cheat_lock);

    R_TRY(EnsureCheatProcess());
    R_RETURN(ResumeCheatProcessUnsafe());
}

Result CheatProcessManager::ResumeCheatProcessUnsafe() {
    broken_unsafe = true;
    unsafe_break_event->Clear();
    if (!system.ApplicationProcess()->IsSuspended()) {
        R_SUCCEED();
    }
    system.ApplicationProcess()->SetActivity(Kernel::Svc::ProcessActivity::Runnable);
    R_SUCCEED();
}

Result CheatProcessManager::ForceCloseCheatProcess() {
    CloseActiveCheatProcess();
    R_SUCCEED();
}

Result CheatProcessManager::GetCheatProcessMappingCount(u64& out_count) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(this->EnsureCheatProcess());

    // TODO: Call svc::QueryDebugProcessMemory

    out_count = 0;
    R_SUCCEED();
}

Result CheatProcessManager::GetCheatProcessMappings(
    u64& out_count, u64 offset, std::span<Kernel::Svc::MemoryInfo> out_mappings) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(this->EnsureCheatProcess());

    // TODO: Call svc::QueryDebugProcessMemory

    out_count = 0;
    R_SUCCEED();
}

Result CheatProcessManager::ReadCheatProcessMemory(u64 process_address, u64 size,
                                                   std::span<u8> out_data) {
    std::scoped_lock lk(cheat_lock);

    R_TRY(EnsureCheatProcess());
    R_RETURN(ReadCheatProcessMemoryUnsafe(process_address, &out_data, size));
}

Result CheatProcessManager::ReadCheatProcessMemoryUnsafe(u64 process_address, void* out_data,
                                                         size_t size) {
    // Return zero on invalid address
    if (!system.ApplicationMemory().IsValidVirtualAddress(process_address)) {
        std::memset(out_data, 0, size);
        R_SUCCEED();
    }

    system.ApplicationMemory().ReadBlock(process_address, out_data, size);
    R_SUCCEED();
}

Result CheatProcessManager::WriteCheatProcessMemory(u64 process_address, u64 size,
                                                    std::span<const u8> data) {
    std::scoped_lock lk(cheat_lock);

    R_TRY(EnsureCheatProcess());
    R_RETURN(WriteCheatProcessMemoryUnsafe(process_address, &data, size));
}

Result CheatProcessManager::WriteCheatProcessMemoryUnsafe(u64 process_address, const void* data,
                                                          size_t size) {
    // Skip invalid memory write address
    if (!system.ApplicationMemory().IsValidVirtualAddress(process_address)) {
        R_SUCCEED();
    }

    if (system.ApplicationMemory().WriteBlock(process_address, data, size)) {
        Core::InvalidateInstructionCacheRange(system.ApplicationProcess(), process_address, size);
    }

    R_SUCCEED();
}

Result CheatProcessManager::QueryCheatProcessMemory(Out<Kernel::Svc::MemoryInfo> mapping,
                                                    u64 address) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(this->EnsureCheatProcess());

    // TODO: Call svc::QueryDebugProcessMemory
    R_SUCCEED();
}

Result CheatProcessManager::GetCheatCount(u64& out_count) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    out_count = std::count_if(cheat_entries.begin(), cheat_entries.end(),
                              [](const auto& entry) { return entry.definition.num_opcodes != 0; });
    R_SUCCEED();
}

Result CheatProcessManager::GetCheats(u64& out_count, u64 offset,
                                      std::span<CheatEntry> out_cheats) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    size_t count = 0, total_count = 0;
    for (size_t i = 0; i < MaxCheatCount && count < out_cheats.size(); i++) {
        if (cheat_entries[i].definition.num_opcodes) {
            total_count++;
            if (total_count > offset) {
                out_cheats[count++] = cheat_entries[i];
            }
        }
    }

    out_count = count;
    R_SUCCEED();
}

Result CheatProcessManager::GetCheatById(CheatEntry* out_cheat, u32 cheat_id) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    const CheatEntry* entry = GetCheatEntryById(cheat_id);
    R_UNLESS(entry != nullptr, ResultCheatUnknownId);
    R_UNLESS(entry->definition.num_opcodes != 0, ResultCheatUnknownId);

    *out_cheat = *entry;
    R_SUCCEED();
}

Result CheatProcessManager::ToggleCheat(u32 cheat_id) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    CheatEntry* entry = GetCheatEntryById(cheat_id);
    R_UNLESS(entry != nullptr, ResultCheatUnknownId);
    R_UNLESS(entry->definition.num_opcodes != 0, ResultCheatUnknownId);

    R_UNLESS(cheat_id != 0, ResultCheatCannotDisable);

    entry->enabled = !entry->enabled;

    // Trigger a VM reload.
    SetNeedsReloadVm(true);

    R_SUCCEED();
}

Result CheatProcessManager::AddCheat(u32& out_cheat_id, bool enabled,
                                     const CheatDefinition& cheat_definition) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    R_UNLESS(cheat_definition.num_opcodes != 0, ResultCheatInvalid);
    R_UNLESS(cheat_definition.num_opcodes <= cheat_definition.opcodes.size(), ResultCheatInvalid);

    CheatEntry* new_entry = GetFreeCheatEntry();
    R_UNLESS(new_entry != nullptr, ResultCheatOutOfResource);

    new_entry->enabled = enabled;
    new_entry->definition = cheat_definition;

    // Trigger a VM reload.
    SetNeedsReloadVm(true);

    // Set output id.
    out_cheat_id = new_entry->cheat_id;

    R_SUCCEED();
}

Result CheatProcessManager::RemoveCheat(u32 cheat_id) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());
    R_UNLESS(cheat_id < MaxCheatCount, ResultCheatUnknownId);

    ResetCheatEntry(cheat_id);
    SetNeedsReloadVm(true);
    R_SUCCEED();
}

Result CheatProcessManager::ReadStaticRegister(u64& out_value, u64 register_index) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());
    R_UNLESS(register_index < CheatVirtualMachine::NumStaticRegisters, ResultCheatInvalid);

    out_value = cheat_vm->GetStaticRegister(register_index);
    R_SUCCEED();
}

Result CheatProcessManager::WriteStaticRegister(u64 register_index, u64 value) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());
    R_UNLESS(register_index < CheatVirtualMachine::NumStaticRegisters, ResultCheatInvalid);

    cheat_vm->SetStaticRegister(register_index, value);
    R_SUCCEED();
}

Result CheatProcessManager::ResetStaticRegisters() {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    cheat_vm->ResetStaticRegisters();
    R_SUCCEED();
}

Result CheatProcessManager::SetMasterCheat(const CheatDefinition& cheat_definition) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    R_UNLESS(cheat_definition.num_opcodes != 0, ResultCheatInvalid);
    R_UNLESS(cheat_definition.num_opcodes <= cheat_definition.opcodes.size(), ResultCheatInvalid);

    cheat_entries[0] = {
        .enabled = true,
        .definition = cheat_definition,
    };

    // Trigger a VM reload.
    SetNeedsReloadVm(true);

    R_SUCCEED();
}

Result CheatProcessManager::GetFrozenAddressCount(u64& out_count) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    out_count = std::distance(frozen_addresses_map.begin(), frozen_addresses_map.end());
    R_SUCCEED();
}

Result CheatProcessManager::GetFrozenAddresses(u64& out_count, u64 offset,
                                               std::span<FrozenAddressEntry> out_frozen_address) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    u64 total_count = 0, written_count = 0;
    for (const auto& [address, value] : frozen_addresses_map) {
        if (written_count >= out_frozen_address.size()) {
            break;
        }

        if (offset <= total_count) {
            out_frozen_address[written_count].address = address;
            out_frozen_address[written_count].value = value;
            written_count++;
        }
        total_count++;
    }

    out_count = written_count;
    R_SUCCEED();
}

Result CheatProcessManager::GetFrozenAddress(FrozenAddressEntry& out_frozen_address_entry,
                                             u64 address) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    const auto it = frozen_addresses_map.find(address);
    R_UNLESS(it != frozen_addresses_map.end(), ResultFrozenAddressNotFound);

    out_frozen_address_entry = {
        .address = it->first,
        .value = it->second,
    };
    R_SUCCEED();
}

Result CheatProcessManager::EnableFrozenAddress(u64& out_value, u64 address, u64 width) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    const auto it = frozen_addresses_map.find(address);
    R_UNLESS(it == frozen_addresses_map.end(), ResultFrozenAddressAlreadyExists);

    FrozenAddressValue value{};
    value.width = static_cast<u8>(width);
    R_TRY(ReadCheatProcessMemoryUnsafe(address, &value.value, width));

    frozen_addresses_map.insert({address, value});
    out_value = value.value;
    R_SUCCEED();
}

Result CheatProcessManager::DisableFrozenAddress(u64 address) {
    std::scoped_lock lk(cheat_lock);
    R_TRY(EnsureCheatProcess());

    const auto it = frozen_addresses_map.find(address);
    R_UNLESS(it != frozen_addresses_map.end(), ResultFrozenAddressNotFound);

    frozen_addresses_map.erase(it);

    R_SUCCEED();
}

u64 CheatProcessManager::HidKeysDown() const {
    const auto hid = system.ServiceManager().GetService<Service::HID::IHidServer>("hid");
    if (hid == nullptr) {
        LOG_WARNING(CheatEngine, "Attempted to read input state, but hid is not initialized!");
        return 0;
    }

    const auto applet_resource = hid->GetResourceManager();
    if (applet_resource == nullptr || applet_resource->GetNpad() == nullptr) {
        LOG_WARNING(CheatEngine,
                    "Attempted to read input state, but applet resource is not initialized!");
        return 0;
    }

    const auto press_state = applet_resource->GetNpad()->GetAndResetPressState();
    return static_cast<u64>(press_state & Core::HID::NpadButton::All);
}

void CheatProcessManager::DebugLog(u8 id, u64 value) const {
    LOG_INFO(CheatEngine, "Cheat triggered DebugLog: ID '{:01X}' Value '{:016X}'", id, value);
}

void CheatProcessManager::CommandLog(std::string_view data) const {
    LOG_DEBUG(CheatEngine, "[DmntCheatVm]: {}",
              data.back() == '\n' ? data.substr(0, data.size() - 1) : data);
}

void CheatProcessManager::FrameCallback(std::chrono::nanoseconds ns_late) {
    std::scoped_lock lk(cheat_lock);

    if (cheat_vm == nullptr) {
        return;
    }

    if (needs_reload_vm) {
        cheat_vm->LoadProgram(cheat_entries);
        needs_reload_vm = false;
    }

    if (cheat_vm->GetProgramSize() == 0) {
        return;
    }

    cheat_vm->Execute(cheat_process_metadata);
}

} // namespace Service::DMNT
