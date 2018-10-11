// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
#include <memory>

#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/process.h"
#include "core/hle/service/ldr/ldr.h"
#include "core/hle/service/service.h"

namespace Service::LDR {

class DebugMonitor final : public ServiceFramework<DebugMonitor> {
public:
    explicit DebugMonitor() : ServiceFramework{"ldr:dmnt"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "AddProcessToDebugLaunchQueue"},
            {1, nullptr, "ClearDebugLaunchQueue"},
            {2, nullptr, "GetNsoInfos"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class ProcessManager final : public ServiceFramework<ProcessManager> {
public:
    explicit ProcessManager() : ServiceFramework{"ldr:pm"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "CreateProcess"},
            {1, nullptr, "GetProgramInfo"},
            {2, nullptr, "RegisterTitle"},
            {3, nullptr, "UnregisterTitle"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class Shell final : public ServiceFramework<Shell> {
public:
    explicit Shell() : ServiceFramework{"ldr:shel"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "AddProcessToLaunchQueue"},
            {1, nullptr, "ClearLaunchQueue"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class RelocatableObject final : public ServiceFramework<RelocatableObject> {
public:
    explicit RelocatableObject() : ServiceFramework{"ldr:ro"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &RelocatableObject::LoadNro, "LoadNro"},
            {1, nullptr, "UnloadNro"},
            {2, &RelocatableObject::LoadNrr, "LoadNrr"},
            {3, nullptr, "UnloadNrr"},
            {4, &RelocatableObject::Initialize, "Initialize"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

    void LoadNrr(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        rp.Skip(2, false);
        auto address = rp.Pop<VAddr>();
        auto size = rp.Pop<u64>();

        auto process = Core::CurrentProcess();
        auto& vm_manager = process->VMManager();

        // Find a region we can use to mirror the NRR memory.
        auto map_address = vm_manager.FindFreeRegion(size);

        ASSERT(map_address.Succeeded());

        auto result = process->MirrorMemory(*map_address, address, size,
                                            Kernel::MemoryState::ModuleCodeStatic);
        ASSERT(result == RESULT_SUCCESS);

        // TODO(Subv): Reprotect the source memory to make it inaccessible.

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void LoadNro(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        rp.Skip(2, false);
        auto nro_address = rp.Pop<VAddr>();
        auto nro_size = rp.Pop<u64>();
        auto bss_address = rp.Pop<VAddr>();
        auto bss_size = rp.Pop<u64>();

        // TODO(Subv): Verify the NRO with the currently-loaded NRR.

        NroHeader nro_header;
        Memory::ReadBlock(nro_address, &nro_header, sizeof(nro_header));

        auto process = Core::CurrentProcess();

        auto& vm_manager = process->VMManager();
        auto map_address = vm_manager.FindFreeRegion(nro_size + bss_size);

        ASSERT(map_address.Succeeded());

        auto result = process->MirrorMemory(*map_address, nro_address, nro_size,
                                            Kernel::MemoryState::ModuleCodeStatic);
        ASSERT(result == RESULT_SUCCESS);

        if (bss_size > 0) {
            result = process->MirrorMemory(*map_address + nro_size, bss_address, bss_size,
                                           Kernel::MemoryState::ModuleCodeStatic);
            ASSERT(result == RESULT_SUCCESS);
        }

        vm_manager.ReprotectRange(*map_address, nro_header.text_size,
                                  Kernel::VMAPermission::ReadExecute);
        vm_manager.ReprotectRange(*map_address + nro_header.ro_offset, nro_header.ro_size,
                                  Kernel::VMAPermission::Read);
        vm_manager.ReprotectRange(*map_address + nro_header.rw_offset,
                                  nro_header.rw_size + bss_size, Kernel::VMAPermission::ReadWrite);

        Core::System::GetInstance().ArmInterface(0).ClearInstructionCache();
        Core::System::GetInstance().ArmInterface(1).ClearInstructionCache();
        Core::System::GetInstance().ArmInterface(2).ClearInstructionCache();
        Core::System::GetInstance().ArmInterface(3).ClearInstructionCache();

        // TODO(Subv): Reprotect the source memory to make it inaccessible.

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push(nro_address);

        LOG_WARNING(Service, "called");
    }

    void Initialize(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

private:
    struct NroHeader {
        u32_le entrypoint_insn;
        u32_le mod_offset;
        INSERT_PADDING_WORDS(2);
        u32_le magic;
        INSERT_PADDING_WORDS(1);
        u32_le nro_size;
        INSERT_PADDING_WORDS(1);
        u32_le text_offset;
        u32_le text_size;
        u32_le ro_offset;
        u32_le ro_size;
        u32_le rw_offset;
        u32_le rw_size;
        u32_le bss_size;
        INSERT_PADDING_WORDS(1);
        std::array<u8, 0x20> build_id;
        INSERT_PADDING_BYTES(0x20);
    };
    static_assert(sizeof(NroHeader) == 128, "NroHeader has invalid size.");
};

void InstallInterfaces(SM::ServiceManager& sm) {
    std::make_shared<DebugMonitor>()->InstallAsService(sm);
    std::make_shared<ProcessManager>()->InstallAsService(sm);
    std::make_shared<Shell>()->InstallAsService(sm);
    std::make_shared<RelocatableObject>()->InstallAsService(sm);
}

} // namespace Service::LDR
