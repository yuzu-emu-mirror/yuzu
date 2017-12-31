// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "common/microprofile.h"
#include "core/core_timing.h"
#include "core/hle/function_wrappers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/sync_object.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/lock.h"
#include "core/hle/result.h"
#include "core/hle/service/service.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Namespace SVC

using Kernel::ERR_INVALID_HANDLE;
using Kernel::Handle;
using Kernel::SharedPtr;

namespace SVC {

/// Set the process heap to a given Size. It can both extend and shrink the heap.
static ResultCode SetHeapSize(VAddr* heap_addr, u64 heap_size) {
    LOG_TRACE(Kernel_SVC, "called, heap_size=0x%llx", heap_size);
    auto& process = *Kernel::g_current_process;
    CASCADE_RESULT(*heap_addr, process.HeapAllocate(Memory::HEAP_VADDR, heap_size,
                                                    Kernel::VMAPermission::ReadWrite));
    return RESULT_SUCCESS;
}

/// Maps a memory range into a different range.
static ResultCode MapMemory(VAddr dst_addr, VAddr src_addr, u64 size) {
    LOG_TRACE(Kernel_SVC, "called, dst_addr=0x%llx, src_addr=0x%llx, size=0x%llx", dst_addr,
              src_addr, size);
    return Kernel::g_current_process->MirrorMemory(dst_addr, src_addr, size);
}

/// Unmaps a region that was previously mapped with svcMapMemory
static ResultCode UnmapMemory(VAddr dst_addr, VAddr src_addr, u64 size) {
    LOG_TRACE(Kernel_SVC, "called, dst_addr=0x%llx, src_addr=0x%llx, size=0x%llx", dst_addr,
        src_addr, size);
    return Kernel::g_current_process->UnmapMemory(dst_addr, src_addr, size);
}

/// Connect to an OS service given the port name, returns the handle to the port to out
static ResultCode ConnectToPort(Kernel::Handle* out_handle, VAddr port_name_address) {
    if (!Memory::IsValidVirtualAddress(port_name_address))
        return Kernel::ERR_NOT_FOUND;

    static constexpr std::size_t PortNameMaxLength = 11;
    // Read 1 char beyond the max allowed port name to detect names that are too long.
    std::string port_name = Memory::ReadCString(port_name_address, PortNameMaxLength + 1);
    if (port_name.size() > PortNameMaxLength)
        return Kernel::ERR_PORT_NAME_TOO_LONG;

    LOG_TRACE(Kernel_SVC, "called port_name=%s", port_name.c_str());

    auto it = Service::g_kernel_named_ports.find(port_name);
    if (it == Service::g_kernel_named_ports.end()) {
        LOG_WARNING(Kernel_SVC, "tried to connect to unknown port: %s", port_name.c_str());
        return Kernel::ERR_NOT_FOUND;
    }

    auto client_port = it->second;

    SharedPtr<Kernel::ClientSession> client_session;
    CASCADE_RESULT(client_session, client_port->Connect());

    // Return the client session
    CASCADE_RESULT(*out_handle, Kernel::g_handle_table.Create(client_session));
    return RESULT_SUCCESS;
}

/// Makes a blocking IPC call to an OS service.
static ResultCode SendSyncRequest(Kernel::Handle handle) {
    SharedPtr<Kernel::SyncObject> session = Kernel::g_handle_table.Get<Kernel::SyncObject>(handle);
    if (!session) {
        LOG_ERROR(Kernel_SVC, "called with invalid handle=0x%08X", handle);
        return ERR_INVALID_HANDLE;
    }

    LOG_TRACE(Kernel_SVC, "called handle=0x%08X(%s)", handle, session->GetName().c_str());

    Core::System::GetInstance().PrepareReschedule();

    // TODO(Subv): svcSendSyncRequest should put the caller thread to sleep while the server
    // responds and cause a reschedule.
    return session->SendSyncRequest(Kernel::GetCurrentThread());
}

/// Get the ID for the specified thread.
static ResultCode GetThreadId(u32* thread_id, Kernel::Handle thread_handle) {
    LOG_TRACE(Kernel_SVC, "called thread=0x%08X", thread_handle);

    const SharedPtr<Kernel::Thread> thread = Kernel::g_handle_table.Get<Kernel::Thread>(thread_handle);
    if (!thread) {
        return ERR_INVALID_HANDLE;
    }

    *thread_id = thread->GetThreadId();
    return RESULT_SUCCESS;
}

/// Get the ID of the specified process
static ResultCode GetProcessId(u32* process_id, Kernel::Handle process_handle) {
    LOG_TRACE(Kernel_SVC, "called process=0x%08X", process_handle);

    const SharedPtr<Kernel::Process> process =
        Kernel::g_handle_table.Get<Kernel::Process>(process_handle);
    if (!process) {
        return ERR_INVALID_HANDLE;
    }

    *process_id = process->process_id;
    return RESULT_SUCCESS;
}

/// Break program execution
static void Break(u64 unk_0, u64 unk_1, u64 unk_2) {
    LOG_CRITICAL(Debug_Emulated, "Emulated program broke execution!");
    ASSERT(false);
}

/// Used to output a message on a debug hardware unit - does nothing on a retail unit
static void OutputDebugString(VAddr address, int len) {
    std::vector<char> string(len);
    Memory::ReadBlock(address, string.data(), len);
    LOG_DEBUG(Debug_Emulated, "%.*s", len, string.data());
}

static ResultCode GetInfo(u64* result, u64 info_id, u64 handle, u64 info_sub_id) {
    LOG_TRACE(Kernel_SVC, "called, info_id=0x%X, info_sub_id=0x%X, handle=0x%08X", info_id, info_sub_id, handle);

    if (!handle) {
        switch (info_id) {
        case 0xB:
            *result = 0; // Used for PRNG seed
            return RESULT_SUCCESS;
        }
    }
    return RESULT_SUCCESS;
}

/// Gets the priority for the specified thread
static ResultCode GetThreadPriority(s32* priority, Kernel::Handle handle) {
    LOG_TRACE(Kernel_SVC, "called, handle=0x%08X", handle);
    const SharedPtr<Kernel::Thread> thread = Kernel::g_handle_table.Get<Kernel::Thread>(handle);
    *priority = thread ? thread->GetPriority() : 0;
    return RESULT_SUCCESS;
}

/// Query process memory
static ResultCode QueryProcessMemory(MemoryInfo* memory_info, PageInfo* /*page_info*/,
                                     Kernel::Handle process_handle, u64 addr) {
    using Kernel::Process;
    Kernel::SharedPtr<Process> process = Kernel::g_handle_table.Get<Process>(process_handle);
    if (!process) {
        return ERR_INVALID_HANDLE;
    }
    auto vma = process->vm_manager.FindVMA(addr);
    memory_info->attributes = 0;
    if (vma == Kernel::g_current_process->vm_manager.vma_map.end()) {
        memory_info->base_address = 0;
        memory_info->permission = static_cast<u32>(Kernel::VMAPermission::None);
        memory_info->size = 0;
        memory_info->type = static_cast<u32>(Kernel::MemoryState::Free);
    } else {
        memory_info->base_address = vma->second.base;
        memory_info->permission = static_cast<u32>(vma->second.permissions);
        memory_info->size = vma->second.size;
        memory_info->type = static_cast<u32>(vma->second.meminfo_state);
    }

    LOG_TRACE(Kernel_SVC, "called process=0x%08X addr=%llx", process_handle, addr);
    return RESULT_SUCCESS;
}

/// Query memory
static ResultCode QueryMemory(MemoryInfo* memory_info, PageInfo* page_info, VAddr addr) {
    LOG_TRACE(Kernel_SVC, "called, addr=%llx", addr);
    return QueryProcessMemory(memory_info, page_info, Kernel::CurrentProcess, addr);
}

/// Starts the thread for the provided handle
static ResultCode StartThread(Handle thread_handle) {
    LOG_TRACE(Kernel_SVC, "called thread=0x%08X", thread_handle);

    const SharedPtr<Kernel::Thread> thread =
        Kernel::g_handle_table.Get<Kernel::Thread>(thread_handle);
    if (!thread) {
        return ERR_INVALID_HANDLE;
    }

    thread->ResumeFromWait();

    return RESULT_SUCCESS;
}

/// Sleep the current thread
static void SleepThread(s64 nanoseconds) {
    LOG_TRACE(Kernel_SVC, "called nanoseconds=%lld", nanoseconds);

    // Don't attempt to yield execution if there are no available threads to run,
    // this way we avoid a useless reschedule to the idle thread.
    if (nanoseconds == 0 && !Kernel::HaveReadyThreads())
        return;

    // Sleep current thread and check for next thread to schedule
    Kernel::WaitCurrentThread_Sleep();

    // Create an event to wake the thread up after the specified nanosecond delay has passed
    Kernel::GetCurrentThread()->WakeAfterDelay(nanoseconds);

    Core::System::GetInstance().PrepareReschedule();
}

/// Signal process wide key
static ResultCode SignalProcessWideKey(VAddr address, u32 target) {
    LOG_TRACE(Kernel_SVC, "called, address=0x%llx, target=0x%08x", address, target);
    return RESULT_SUCCESS;
}

/// Close a handle
static ResultCode CloseHandle(Kernel::Handle handle) {
    LOG_TRACE(Kernel_SVC, "Closing handle 0x%08X", handle);
    return Kernel::g_handle_table.Close(handle);
}

namespace {
struct FunctionDef {
    using Func = void();

    u32 id;
    Func* func;
    const char* name;
};
} // namespace

static const FunctionDef SVC_Table[] = {
    {0x00, nullptr, "Unknown"},
    {0x01, HLE::Wrap<SetHeapSize>, "svcSetHeapSize"},
    {0x02, nullptr, "svcSetMemoryPermission"},
    {0x03, nullptr, "svcSetMemoryAttribute"},
    {0x04, HLE::Wrap<MapMemory>, "svcMapMemory"},
    {0x05, HLE::Wrap<UnmapMemory>, "svcUnmapMemory"},
    {0x06, HLE::Wrap<QueryMemory>, "svcQueryMemory"},
    {0x07, nullptr, "svcExitProcess"},
    {0x08, nullptr, "svcCreateThread"},
    {0x09, nullptr, "svcStartThread"},
    {0x09, HLE::Wrap<StartThread>, "svcStartThread"},
    {0x0A, nullptr, "svcExitThread"},
    {0x0B, HLE::Wrap<SleepThread>, "svcSleepThread"},
    {0x0C, HLE::Wrap<GetThreadPriority>, "svcGetThreadPriority"},
    {0x0D, nullptr, "svcSetThreadPriority"},
    {0x0E, nullptr, "svcGetThreadCoreMask"},
    {0x0F, nullptr, "svcSetThreadCoreMask"},
    {0x10, nullptr, "svcGetCurrentProcessorNumber"},
    {0x11, nullptr, "svcSignalEvent"},
    {0x12, nullptr, "svcClearEvent"},
    {0x13, nullptr, "svcMapSharedMemory"},
    {0x14, nullptr, "svcUnmapSharedMemory"},
    {0x15, nullptr, "svcCreateTransferMemory"},
    {0x16, HLE::Wrap<CloseHandle>, "svcCloseHandle"},
    {0x17, nullptr, "svcResetSignal"},
    {0x18, nullptr, "svcWaitSynchronization"},
    {0x19, nullptr, "svcCancelSynchronization"},
    {0x1A, nullptr, "svcLockMutex"},
    {0x1B, nullptr, "svcUnlockMutex"},
    {0x1C, nullptr, "svcWaitProcessWideKeyAtomic"},
    {0x1D, HLE::Wrap<SignalProcessWideKey>, "svcSignalProcessWideKey"},
    {0x1E, nullptr, "svcGetSystemTick"},
    {0x1F, HLE::Wrap<ConnectToPort>, "svcConnectToPort"},
    {0x20, nullptr, "svcSendSyncRequestLight"},
    {0x21, HLE::Wrap<SendSyncRequest>, "svcSendSyncRequest"},
    {0x22, nullptr, "svcSendSyncRequestWithUserBuffer"},
    {0x23, nullptr, "svcSendAsyncRequestWithUserBuffer"},
    {0x24, HLE::Wrap<GetProcessId>, "svcGetProcessId"},
    {0x25, HLE::Wrap<GetThreadId>, "svcGetThreadId"},
    {0x26, HLE::Wrap<Break>, "svcBreak"},
    {0x27, HLE::Wrap<OutputDebugString>, "svcOutputDebugString"},
    {0x28, nullptr, "svcReturnFromException"},
    {0x29, HLE::Wrap<GetInfo>, "svcGetInfo"},
    {0x2A, nullptr, "svcFlushEntireDataCache"},
    {0x2B, nullptr, "svcFlushDataCache"},
    {0x2C, nullptr, "svcMapPhysicalMemory"},
    {0x2D, nullptr, "svcUnmapPhysicalMemory"},
    {0x2E, nullptr, "Unknown"},
    {0x2F, nullptr, "svcGetLastThreadInfo"},
    {0x30, nullptr, "svcGetResourceLimitLimitValue"},
    {0x31, nullptr, "svcGetResourceLimitCurrentValue"},
    {0x32, nullptr, "svcSetThreadActivity"},
    {0x33, nullptr, "svcGetThreadContext"},
    {0x34, nullptr, "Unknown"},
    {0x35, nullptr, "Unknown"},
    {0x36, nullptr, "Unknown"},
    {0x37, nullptr, "Unknown"},
    {0x38, nullptr, "Unknown"},
    {0x39, nullptr, "Unknown"},
    {0x3A, nullptr, "Unknown"},
    {0x3B, nullptr, "Unknown"},
    {0x3C, nullptr, "svcDumpInfo"},
    {0x3D, nullptr, "Unknown"},
    {0x3E, nullptr, "Unknown"},
    {0x3F, nullptr, "Unknown"},
    {0x40, nullptr, "svcCreateSession"},
    {0x41, nullptr, "svcAcceptSession"},
    {0x42, nullptr, "svcReplyAndReceiveLight"},
    {0x43, nullptr, "svcReplyAndReceive"},
    {0x44, nullptr, "svcReplyAndReceiveWithUserBuffer"},
    {0x45, nullptr, "svcCreateEvent"},
    {0x46, nullptr, "Unknown"},
    {0x47, nullptr, "Unknown"},
    {0x48, nullptr, "Unknown"},
    {0x49, nullptr, "Unknown"},
    {0x4A, nullptr, "Unknown"},
    {0x4B, nullptr, "Unknown"},
    {0x4C, nullptr, "Unknown"},
    {0x4D, nullptr, "svcSleepSystem"},
    {0x4E, nullptr, "svcReadWriteRegister"},
    {0x4F, nullptr, "svcSetProcessActivity"},
    {0x50, nullptr, "svcCreateSharedMemory"},
    {0x51, nullptr, "svcMapTransferMemory"},
    {0x52, nullptr, "svcUnmapTransferMemory"},
    {0x53, nullptr, "svcCreateInterruptEvent"},
    {0x54, nullptr, "svcQueryPhysicalAddress"},
    {0x55, nullptr, "svcQueryIoMapping"},
    {0x56, nullptr, "svcCreateDeviceAddressSpace"},
    {0x57, nullptr, "svcAttachDeviceAddressSpace"},
    {0x58, nullptr, "svcDetachDeviceAddressSpace"},
    {0x59, nullptr, "svcMapDeviceAddressSpaceByForce"},
    {0x5A, nullptr, "svcMapDeviceAddressSpaceAligned"},
    {0x5B, nullptr, "svcMapDeviceAddressSpace"},
    {0x5C, nullptr, "svcUnmapDeviceAddressSpace"},
    {0x5D, nullptr, "svcInvalidateProcessDataCache"},
    {0x5E, nullptr, "svcStoreProcessDataCache"},
    {0x5F, nullptr, "svcFlushProcessDataCache"},
    {0x60, nullptr, "svcDebugActiveProcess"},
    {0x61, nullptr, "svcBreakDebugProcess"},
    {0x62, nullptr, "svcTerminateDebugProcess"},
    {0x63, nullptr, "svcGetDebugEvent"},
    {0x64, nullptr, "svcContinueDebugEvent"},
    {0x65, nullptr, "svcGetProcessList"},
    {0x66, nullptr, "svcGetThreadList"},
    {0x67, nullptr, "svcGetDebugThreadContext"},
    {0x68, nullptr, "svcSetDebugThreadContext"},
    {0x69, nullptr, "svcQueryDebugProcessMemory"},
    {0x6A, nullptr, "svcReadDebugProcessMemory"},
    {0x6B, nullptr, "svcWriteDebugProcessMemory"},
    {0x6C, nullptr, "svcSetHardwareBreakPoint"},
    {0x6D, nullptr, "svcGetDebugThreadParam"},
    {0x6E, nullptr, "Unknown"},
    {0x6F, nullptr, "Unknown"},
    {0x70, nullptr, "svcCreatePort"},
    {0x71, nullptr, "svcManageNamedPort"},
    {0x72, nullptr, "svcConnectToPort"},
    {0x73, nullptr, "svcSetProcessMemoryPermission"},
    {0x74, nullptr, "svcMapProcessMemory"},
    {0x75, nullptr, "svcUnmapProcessMemory"},
    {0x76, nullptr, "svcQueryProcessMemory"},
    {0x77, nullptr, "svcMapProcessCodeMemory"},
    {0x78, nullptr, "svcUnmapProcessCodeMemory"},
    {0x79, nullptr, "svcCreateProcess"},
    {0x7A, nullptr, "svcStartProcess"},
    {0x7B, nullptr, "svcTerminateProcess"},
    {0x7C, nullptr, "svcGetProcessInfo"},
    {0x7D, nullptr, "svcCreateResourceLimit"},
    {0x7E, nullptr, "svcSetResourceLimitLimitValue"},
    {0x7F, nullptr, "svcCallSecureMonitor"},
};

static const FunctionDef* GetSVCInfo(u32 func_num) {
    if (func_num >= ARRAY_SIZE(SVC_Table)) {
        LOG_ERROR(Kernel_SVC, "unknown svc=0x%02X", func_num);
        return nullptr;
    }
    return &SVC_Table[func_num];
}

MICROPROFILE_DEFINE(Kernel_SVC, "Kernel", "SVC", MP_RGB(70, 200, 70));

void CallSVC(u32 immediate) {
    MICROPROFILE_SCOPE(Kernel_SVC);

    // Lock the global kernel mutex when we enter the kernel HLE.
    std::lock_guard<std::recursive_mutex> lock(HLE::g_hle_lock);

    const FunctionDef* info = GetSVCInfo(immediate);
    if (info) {
        if (info->func) {
            info->func();
        } else {
            LOG_CRITICAL(Kernel_SVC, "unimplemented SVC function %s(..)", info->name);
        }
    } else {
        LOG_CRITICAL(Kernel_SVC, "unknown SVC function 0x%x", immediate);
    }
}

} // namespace SVC
