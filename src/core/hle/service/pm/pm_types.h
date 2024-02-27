// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/result.h"
#include "core/hle/service/cmif_types.h"

namespace Service::PM {

constexpr Result ResultProcessNotFound{ErrorModule::PM, 1};
[[maybe_unused]] constexpr Result ResultAlreadyStarted{ErrorModule::PM, 2};
[[maybe_unused]] constexpr Result ResultNotTerminated{ErrorModule::PM, 3};
[[maybe_unused]] constexpr Result ResultDebugHookInUse{ErrorModule::PM, 4};
[[maybe_unused]] constexpr Result ResultApplicationRunning{ErrorModule::PM, 5};
[[maybe_unused]] constexpr Result ResultInvalidSize{ErrorModule::PM, 6};

constexpr u64 NO_PROCESS_FOUND_PID{0};

enum class BootMode {
    Normal = 0,
    Maintenance = 1,
    SafeMode = 2,
};

struct ProgramLocation {
    u64 program_id;
    u8 storage_id;
};
static_assert(sizeof(ProgramLocation) == 0x10, "ProgramLocation has an invalid size");

struct OverrideStatus {
    u64 keys_held;
    u64 flags;
};
static_assert(sizeof(OverrideStatus) == 0x10, "OverrideStatus has an invalid size");

using ProcessList = std::list<Kernel::KScopedAutoObject<Kernel::KProcess>>;

template <typename F>
static inline Kernel::KScopedAutoObject<Kernel::KProcess> SearchProcessList(
    ProcessList& process_list, F&& predicate) {
    const auto iter = std::find_if(process_list.begin(), process_list.end(), predicate);

    if (iter == process_list.end()) {
        return nullptr;
    }

    return iter->GetPointerUnsafe();
}

static inline Result GetApplicationPidGeneric(Out<ProcessId> out_process_id,
                                              ProcessList& process_list) {
    auto process = SearchProcessList(process_list, [](auto& p) { return p->IsApplication(); });

    *out_process_id =
        process.IsNull() ? ProcessId(NO_PROCESS_FOUND_PID) : ProcessId(process->GetProcessId());

    R_SUCCEED();
}

} // namespace Service::PM
