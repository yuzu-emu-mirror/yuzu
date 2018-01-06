// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "common/microprofile.h"
#include "common/string_util.h"
#include "core/core_timing.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/mutex.h"
#include "core/hle/kernel/object_address_table.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/hle/kernel/svc.h"
#include "core/hle/kernel/svc_wrap.h"
#include "core/hle/kernel/sync_object.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/lock.h"
#include "core/hle/result.h"
#include "core/hle/service/service.h"

namespace Kernel {

/// Set the process heap to a given Size. It can both extend and shrink the heap.
static ResultCode SetHeapSize(VAddr* heap_addr, u64 heap_size) {
    LOG_TRACE(Kernel_SVC, "called, heap_size=0x%llx", heap_size);
    auto& process = *g_current_process;
    CASCADE_RESULT(*heap_addr,
                   process.HeapAllocate(Memory::HEAP_VADDR, heap_size, VMAPermission::ReadWrite));
    return RESULT_SUCCESS;
}

/// Maps a memory range into a different range.
static ResultCode MapMemory(VAddr dst_addr, VAddr src_addr, u64 size) {
    LOG_TRACE(Kernel_SVC, "called, dst_addr=0x%llx, src_addr=0x%llx, size=0x%llx", dst_addr,
              src_addr, size);
    return g_current_process->MirrorMemory(dst_addr, src_addr, size);
}

/// Unmaps a region that was previously mapped with svcMapMemory
static ResultCode UnmapMemory(VAddr dst_addr, VAddr src_addr, u64 size) {
    LOG_TRACE(Kernel_SVC, "called, dst_addr=0x%llx, src_addr=0x%llx, size=0x%llx", dst_addr,
              src_addr, size);
    return g_current_process->UnmapMemory(dst_addr, src_addr, size);
}

/// Connect to an OS service given the port name, returns the handle to the port to out
static ResultCode ConnectToPort(Handle* out_handle, VAddr port_name_address) {
    if (!Memory::IsValidVirtualAddress(port_name_address))
        return ERR_NOT_FOUND;

    static constexpr std::size_t PortNameMaxLength = 11;
    // Read 1 char beyond the max allowed port name to detect names that are too long.
    std::string port_name = Memory::ReadCString(port_name_address, PortNameMaxLength + 1);
    if (port_name.size() > PortNameMaxLength)
        return ERR_PORT_NAME_TOO_LONG;

    LOG_TRACE(Kernel_SVC, "called port_name=%s", port_name.c_str());

    auto it = Service::g_kernel_named_ports.find(port_name);
    if (it == Service::g_kernel_named_ports.end()) {
        LOG_WARNING(Kernel_SVC, "tried to connect to unknown port: %s", port_name.c_str());
        return ERR_NOT_FOUND;
    }

    auto client_port = it->second;

    SharedPtr<ClientSession> client_session;
    CASCADE_RESULT(client_session, client_port->Connect());

    // Return the client session
    CASCADE_RESULT(*out_handle, g_handle_table.Create(client_session));
    return RESULT_SUCCESS;
}

/// Makes a blocking IPC call to an OS service.
static ResultCode SendSyncRequest(Handle handle) {
    SharedPtr<SyncObject> session = g_handle_table.Get<SyncObject>(handle);
    if (!session) {
        LOG_ERROR(Kernel_SVC, "called with invalid handle=0x%08X", handle);
        return ERR_INVALID_HANDLE;
    }

    LOG_TRACE(Kernel_SVC, "called handle=0x%08X(%s)", handle, session->GetName().c_str());

    Core::System::GetInstance().PrepareReschedule();

    // TODO(Subv): svcSendSyncRequest should put the caller thread to sleep while the server
    // responds and cause a reschedule.
    return session->SendSyncRequest(GetCurrentThread());
}

/// Get the ID for the specified thread.
static ResultCode GetThreadId(u32* thread_id, Handle thread_handle) {
    LOG_TRACE(Kernel_SVC, "called thread=0x%08X", thread_handle);

    const SharedPtr<Thread> thread = g_handle_table.Get<Thread>(thread_handle);
    if (!thread) {
        return ERR_INVALID_HANDLE;
    }

    *thread_id = thread->GetThreadId();
    return RESULT_SUCCESS;
}

/// Get the ID of the specified process
static ResultCode GetProcessId(u32* process_id, Handle process_handle) {
    LOG_TRACE(Kernel_SVC, "called process=0x%08X", process_handle);

    const SharedPtr<Process> process = g_handle_table.Get<Process>(process_handle);
    if (!process) {
        return ERR_INVALID_HANDLE;
    }

    *process_id = process->process_id;
    return RESULT_SUCCESS;
}

/// Default thread wakeup callback for WaitSynchronization
static void DefaultThreadWakeupCallback(ThreadWakeupReason reason, SharedPtr<Thread> thread,
                                        SharedPtr<WaitObject> object) {
    ASSERT(thread->status == THREADSTATUS_WAIT_SYNCH_ANY);

    if (reason == ThreadWakeupReason::Timeout) {
        thread->SetWaitSynchronizationResult(RESULT_TIMEOUT);
        return;
    }

    ASSERT(reason == ThreadWakeupReason::Signal);
    thread->SetWaitSynchronizationResult(RESULT_SUCCESS);
};

/// Wait for a kernel object to synchronize, timeout after the specified nanoseconds
static ResultCode WaitSynchronization1(
    SharedPtr<WaitObject> object, Thread* thread, s64 nano_seconds = -1,
    std::function<Thread::WakeupCallback> wakeup_callback = DefaultThreadWakeupCallback) {

    if (!object) {
        return ERR_INVALID_HANDLE;
    }

    if (object->ShouldWait(thread)) {
        if (nano_seconds == 0) {
            return RESULT_TIMEOUT;
        }

        thread->wait_objects = {object};
        object->AddWaitingThread(thread);
        thread->status = THREADSTATUS_WAIT_SYNCH_ANY;

        // Create an event to wake the thread up after the specified nanosecond delay has passed
        thread->WakeAfterDelay(nano_seconds);
        thread->wakeup_callback = wakeup_callback;

        Core::System::GetInstance().PrepareReschedule();
    } else {
        object->Acquire(thread);
    }

    return RESULT_SUCCESS;
}

/// Wait for the given handles to synchronize, timeout after the specified nanoseconds
static ResultCode WaitSynchronization(VAddr handles_address, u64 handle_count, s64 nano_seconds) {
    LOG_TRACE(Kernel_SVC, "called handles_address=0x%llx, handle_count=%d, nano_seconds=%d",
              handles_address, handle_count, nano_seconds);

    if (!Memory::IsValidVirtualAddress(handles_address))
        return ERR_INVALID_POINTER;

    // Check if 'handle_count' is invalid
    if (handle_count < 0)
        return ERR_OUT_OF_RANGE;

    using ObjectPtr = SharedPtr<WaitObject>;
    std::vector<ObjectPtr> objects(handle_count);

    for (int i = 0; i < handle_count; ++i) {
        Handle handle = Memory::Read32(handles_address + i * sizeof(Handle));
        auto object = g_handle_table.Get<WaitObject>(handle);
        if (object == nullptr)
            return ERR_INVALID_HANDLE;
        objects[i] = object;
    }

    // Just implement for a single handle for now
    ASSERT(handle_count == 1);
    return WaitSynchronization1(objects[0], GetCurrentThread(), nano_seconds);
}

/// Attempts to locks a mutex, creating it if it does not already exist
static ResultCode LockMutex(Handle holding_thread_handle, VAddr mutex_addr,
                            Handle requesting_thread_handle) {
    LOG_TRACE(Kernel_SVC,
              "called holding_thread_handle=0x%08X, mutex_addr=0x%llx, "
              "requesting_current_thread_handle=0x%08X",
              holding_thread_handle, mutex_addr, requesting_thread_handle);

    SharedPtr<Thread> holding_thread = g_handle_table.Get<Thread>(holding_thread_handle);
    SharedPtr<Thread> requesting_thread = g_handle_table.Get<Thread>(requesting_thread_handle);

    ASSERT(holding_thread);
    ASSERT(requesting_thread);

    SharedPtr<Mutex> mutex = g_object_address_table.Get<Mutex>(mutex_addr);
    if (!mutex) {
        // Create a new mutex for the specified address if one does not already exist
        mutex = Mutex::Create(holding_thread, mutex_addr);
        mutex->name = Common::StringFromFormat("mutex-%llx", mutex_addr);
    }

    return WaitSynchronization1(mutex, requesting_thread.get());
}

/// Unlock a mutex
static ResultCode UnlockMutex(VAddr mutex_addr) {
    LOG_TRACE(Kernel_SVC, "called mutex_addr=0x%llx", mutex_addr);

    SharedPtr<Mutex> mutex = g_object_address_table.Get<Mutex>(mutex_addr);
    ASSERT(mutex);

    return mutex->Release(GetCurrentThread());
}

/// Break program execution
static void Break(u64 unk_0, u64 unk_1, u64 unk_2) {
    LOG_CRITICAL(Debug_Emulated, "Emulated program broke execution!");
    ASSERT(false);
}

/// Used to output a message on a debug hardware unit - does nothing on a retail unit
static void OutputDebugString(VAddr address, s32 len) {
    std::vector<char> string(len);
    Memory::ReadBlock(address, string.data(), len);
    LOG_DEBUG(Debug_Emulated, "%.*s", len, string.data());
}

/// Gets system/memory information for the current process
static ResultCode GetInfo(u64* result, u64 info_id, u64 handle, u64 info_sub_id) {
    LOG_TRACE(Kernel_SVC, "called info_id=0x%X, info_sub_id=0x%X, handle=0x%08X", info_id,
              info_sub_id, handle);

    auto& vm_manager = g_current_process->vm_manager;
    switch (static_cast<GetInfoType>(info_id)) {
    case GetInfoType::TotalMemoryUsage:
        *result = vm_manager.GetTotalMemoryUsage();
        break;
    case GetInfoType::TotalHeapUsage:
        *result = vm_manager.GetTotalHeapUsage();
        break;
    case GetInfoType::RandomEntropy:
        *result = 0;
        break;
    case GetInfoType::AddressSpaceBaseAddr:
        *result = vm_manager.GetAddressSpaceBaseAddr();
        break;
    case GetInfoType::AddressSpaceSize:
        *result = vm_manager.GetAddressSpaceSize();
        break;
    case GetInfoType::NewMapRegionBaseAddr:
        *result = vm_manager.GetNewMapRegionBaseAddr();
        break;
    case GetInfoType::NewMapRegionSize:
        *result = vm_manager.GetNewMapRegionSize();
        break;
    default:
        UNIMPLEMENTED();
    }

    return RESULT_SUCCESS;
}

/// Gets the priority for the specified thread
static ResultCode GetThreadPriority(u32* priority, Handle handle) {
    const SharedPtr<Thread> thread = g_handle_table.Get<Thread>(handle);
    if (!thread)
        return ERR_INVALID_HANDLE;

    *priority = thread->GetPriority();
    return RESULT_SUCCESS;
}

/// Sets the priority for the specified thread
static ResultCode SetThreadPriority(Handle handle, u32 priority) {
    if (priority > THREADPRIO_LOWEST) {
        return ERR_OUT_OF_RANGE;
    }

    SharedPtr<Thread> thread = g_handle_table.Get<Thread>(handle);
    if (!thread)
        return ERR_INVALID_HANDLE;

    // Note: The kernel uses the current process's resource limit instead of
    // the one from the thread owner's resource limit.
    SharedPtr<ResourceLimit>& resource_limit = g_current_process->resource_limit;
    if (resource_limit->GetMaxResourceValue(ResourceTypes::PRIORITY) > priority) {
        return ERR_NOT_AUTHORIZED;
    }

    thread->SetPriority(priority);
    thread->UpdatePriority();

    // Update the mutexes that this thread is waiting for
    for (auto& mutex : thread->pending_mutexes)
        mutex->UpdatePriority();

    Core::System::GetInstance().PrepareReschedule();
    return RESULT_SUCCESS;
}

/// Get which CPU core is executing the current thread
static u32 GetCurrentProcessorNumber() {
    LOG_WARNING(Kernel_SVC, "(STUBBED) called, defaulting to processor 0");
    return 0;
}

/// Query process memory
static ResultCode QueryProcessMemory(MemoryInfo* memory_info, PageInfo* /*page_info*/,
                                     Handle process_handle, u64 addr) {
    SharedPtr<Process> process = g_handle_table.Get<Process>(process_handle);
    if (!process) {
        return ERR_INVALID_HANDLE;
    }
    auto vma = process->vm_manager.FindVMA(addr);
    memory_info->attributes = 0;
    if (vma == g_current_process->vm_manager.vma_map.end()) {
        memory_info->base_address = 0;
        memory_info->permission = static_cast<u32>(VMAPermission::None);
        memory_info->size = 0;
        memory_info->type = static_cast<u32>(MemoryState::Free);
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
    return QueryProcessMemory(memory_info, page_info, CurrentProcess, addr);
}

/// Exits the current process
static void ExitProcess() {
    LOG_INFO(Kernel_SVC, "Process %u exiting", g_current_process->process_id);

    ASSERT_MSG(g_current_process->status == ProcessStatus::Running, "Process has already exited");

    g_current_process->status = ProcessStatus::Exited;

    // Stop all the process threads that are currently waiting for objects.
    auto& thread_list = GetThreadList();
    for (auto& thread : thread_list) {
        if (thread->owner_process != g_current_process)
            continue;

        if (thread == GetCurrentThread())
            continue;

        // TODO(Subv): When are the other running/ready threads terminated?
        ASSERT_MSG(thread->status == THREADSTATUS_WAIT_SYNCH_ANY ||
                       thread->status == THREADSTATUS_WAIT_SYNCH_ALL,
                   "Exiting processes with non-waiting threads is currently unimplemented");

        thread->Stop();
    }

    // Kill the current thread
    GetCurrentThread()->Stop();

    Core::System::GetInstance().PrepareReschedule();
}

/// Creates a new thread
static ResultCode CreateThread(Handle* out_handle, VAddr entry_point, u64 arg, VAddr stack_top,
                               u32 priority, s32 processor_id) {
    std::string name = Common::StringFromFormat("unknown-%llx", entry_point);

    if (priority > THREADPRIO_LOWEST) {
        return ERR_OUT_OF_RANGE;
    }

    SharedPtr<ResourceLimit>& resource_limit = g_current_process->resource_limit;
    if (resource_limit->GetMaxResourceValue(ResourceTypes::PRIORITY) > priority) {
        return ERR_NOT_AUTHORIZED;
    }

    if (processor_id == THREADPROCESSORID_DEFAULT) {
        // Set the target CPU to the one specified in the process' exheader.
        processor_id = g_current_process->ideal_processor;
        ASSERT(processor_id != THREADPROCESSORID_DEFAULT);
    }

    switch (processor_id) {
    case THREADPROCESSORID_0:
        break;
    case THREADPROCESSORID_ALL:
        LOG_INFO(Kernel_SVC,
                 "Newly created thread is allowed to be run in any Core, unimplemented.");
        break;
    case THREADPROCESSORID_1:
        LOG_ERROR(Kernel_SVC,
                  "Newly created thread must run in the SysCore (Core1), unimplemented.");
        break;
    default:
        // TODO(bunnei): Implement support for other processor IDs
        ASSERT_MSG(false, "Unsupported thread processor ID: %d", processor_id);
        break;
    }

    CASCADE_RESULT(SharedPtr<Thread> thread,
                   Thread::Create(name, entry_point, priority, arg, processor_id, stack_top,
                                  g_current_process));
    CASCADE_RESULT(thread->guest_handle, g_handle_table.Create(thread));
    *out_handle = thread->guest_handle;

    Core::System::GetInstance().PrepareReschedule();

    LOG_TRACE(Kernel_SVC,
              "called entrypoint=0x%08X (%s), arg=0x%08X, stacktop=0x%08X, "
              "threadpriority=0x%08X, processorid=0x%08X : created handle=0x%08X",
              entry_point, name.c_str(), arg, stack_top, priority, processor_id, *out_handle);

    return RESULT_SUCCESS;
}

/// Starts the thread for the provided handle
static ResultCode StartThread(Handle thread_handle) {
    LOG_TRACE(Kernel_SVC, "called thread=0x%08X", thread_handle);

    const SharedPtr<Thread> thread = g_handle_table.Get<Thread>(thread_handle);
    if (!thread) {
        return ERR_INVALID_HANDLE;
    }

    thread->ResumeFromWait();

    return RESULT_SUCCESS;
}

/// Called when a thread exits
static void ExitThread() {
    LOG_TRACE(Kernel_SVC, "called, pc=0x%08X", Core::CPU().GetPC());

    ExitCurrentThread();
    Core::System::GetInstance().PrepareReschedule();
}

/// Sleep the current thread
static void SleepThread(s64 nanoseconds) {
    LOG_TRACE(Kernel_SVC, "called nanoseconds=%lld", nanoseconds);

    // Don't attempt to yield execution if there are no available threads to run,
    // this way we avoid a useless reschedule to the idle thread.
    if (nanoseconds == 0 && !HaveReadyThreads())
        return;

    // Sleep current thread and check for next thread to schedule
    WaitCurrentThread_Sleep();

    // Create an event to wake the thread up after the specified nanosecond delay has passed
    GetCurrentThread()->WakeAfterDelay(nanoseconds);

    Core::System::GetInstance().PrepareReschedule();
}

/// Signal process wide key
static ResultCode SignalProcessWideKey(VAddr addr, u32 target) {
    LOG_WARNING(Kernel_SVC, "(STUBBED) called, address=0x%llx, target=0x%08x", addr, target);
    return RESULT_SUCCESS;
}

/// Close a handle
static ResultCode CloseHandle(Handle handle) {
    LOG_TRACE(Kernel_SVC, "Closing handle 0x%08X", handle);
    return g_handle_table.Close(handle);
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
    {0x01, SvcWrap<SetHeapSize>, "SetHeapSize"},
    {0x02, nullptr, "SetMemoryPermission"},
    {0x03, nullptr, "SetMemoryAttribute"},
    {0x04, SvcWrap<MapMemory>, "MapMemory"},
    {0x05, SvcWrap<UnmapMemory>, "UnmapMemory"},
    {0x06, SvcWrap<QueryMemory>, "QueryMemory"},
    {0x07, SvcWrap<ExitProcess>, "ExitProcess"},
    {0x08, SvcWrap<CreateThread>, "CreateThread"},
    {0x09, SvcWrap<StartThread>, "StartThread"},
    {0x0A, SvcWrap<ExitThread>, "ExitThread"},
    {0x0B, SvcWrap<SleepThread>, "SleepThread"},
    {0x0C, SvcWrap<GetThreadPriority>, "GetThreadPriority"},
    {0x0D, SvcWrap<SetThreadPriority>, "SetThreadPriority"},
    {0x0E, nullptr, "GetThreadCoreMask"},
    {0x0F, nullptr, "SetThreadCoreMask"},
    {0x10, SvcWrap<GetCurrentProcessorNumber>, "GetCurrentProcessorNumber"},
    {0x11, nullptr, "SignalEvent"},
    {0x12, nullptr, "ClearEvent"},
    {0x13, nullptr, "MapSharedMemory"},
    {0x14, nullptr, "UnmapSharedMemory"},
    {0x15, nullptr, "CreateTransferMemory"},
    {0x16, SvcWrap<CloseHandle>, "CloseHandle"},
    {0x17, nullptr, "ResetSignal"},
    {0x18, SvcWrap<WaitSynchronization>, "WaitSynchronization"},
    {0x19, nullptr, "CancelSynchronization"},
    {0x1A, SvcWrap<LockMutex>, "LockMutex"},
    {0x1B, SvcWrap<UnlockMutex>, "UnlockMutex"},
    {0x1C, nullptr, "WaitProcessWideKeyAtomic"},
    {0x1D, SvcWrap<SignalProcessWideKey>, "SignalProcessWideKey"},
    {0x1E, nullptr, "GetSystemTick"},
    {0x1F, SvcWrap<ConnectToPort>, "ConnectToPort"},
    {0x20, nullptr, "SendSyncRequestLight"},
    {0x21, SvcWrap<SendSyncRequest>, "SendSyncRequest"},
    {0x22, nullptr, "SendSyncRequestWithUserBuffer"},
    {0x23, nullptr, "SendAsyncRequestWithUserBuffer"},
    {0x24, SvcWrap<GetProcessId>, "GetProcessId"},
    {0x25, SvcWrap<GetThreadId>, "GetThreadId"},
    {0x26, SvcWrap<Break>, "Break"},
    {0x27, SvcWrap<OutputDebugString>, "OutputDebugString"},
    {0x28, nullptr, "ReturnFromException"},
    {0x29, SvcWrap<GetInfo>, "GetInfo"},
    {0x2A, nullptr, "FlushEntireDataCache"},
    {0x2B, nullptr, "FlushDataCache"},
    {0x2C, nullptr, "MapPhysicalMemory"},
    {0x2D, nullptr, "UnmapPhysicalMemory"},
    {0x2E, nullptr, "Unknown"},
    {0x2F, nullptr, "GetLastThreadInfo"},
    {0x30, nullptr, "GetResourceLimitLimitValue"},
    {0x31, nullptr, "GetResourceLimitCurrentValue"},
    {0x32, nullptr, "SetThreadActivity"},
    {0x33, nullptr, "GetThreadContext"},
    {0x34, nullptr, "Unknown"},
    {0x35, nullptr, "Unknown"},
    {0x36, nullptr, "Unknown"},
    {0x37, nullptr, "Unknown"},
    {0x38, nullptr, "Unknown"},
    {0x39, nullptr, "Unknown"},
    {0x3A, nullptr, "Unknown"},
    {0x3B, nullptr, "Unknown"},
    {0x3C, nullptr, "DumpInfo"},
    {0x3D, nullptr, "Unknown"},
    {0x3E, nullptr, "Unknown"},
    {0x3F, nullptr, "Unknown"},
    {0x40, nullptr, "CreateSession"},
    {0x41, nullptr, "AcceptSession"},
    {0x42, nullptr, "ReplyAndReceiveLight"},
    {0x43, nullptr, "ReplyAndReceive"},
    {0x44, nullptr, "ReplyAndReceiveWithUserBuffer"},
    {0x45, nullptr, "CreateEvent"},
    {0x46, nullptr, "Unknown"},
    {0x47, nullptr, "Unknown"},
    {0x48, nullptr, "Unknown"},
    {0x49, nullptr, "Unknown"},
    {0x4A, nullptr, "Unknown"},
    {0x4B, nullptr, "Unknown"},
    {0x4C, nullptr, "Unknown"},
    {0x4D, nullptr, "SleepSystem"},
    {0x4E, nullptr, "ReadWriteRegister"},
    {0x4F, nullptr, "SetProcessActivity"},
    {0x50, nullptr, "CreateSharedMemory"},
    {0x51, nullptr, "MapTransferMemory"},
    {0x52, nullptr, "UnmapTransferMemory"},
    {0x53, nullptr, "CreateInterruptEvent"},
    {0x54, nullptr, "QueryPhysicalAddress"},
    {0x55, nullptr, "QueryIoMapping"},
    {0x56, nullptr, "CreateDeviceAddressSpace"},
    {0x57, nullptr, "AttachDeviceAddressSpace"},
    {0x58, nullptr, "DetachDeviceAddressSpace"},
    {0x59, nullptr, "MapDeviceAddressSpaceByForce"},
    {0x5A, nullptr, "MapDeviceAddressSpaceAligned"},
    {0x5B, nullptr, "MapDeviceAddressSpace"},
    {0x5C, nullptr, "UnmapDeviceAddressSpace"},
    {0x5D, nullptr, "InvalidateProcessDataCache"},
    {0x5E, nullptr, "StoreProcessDataCache"},
    {0x5F, nullptr, "FlushProcessDataCache"},
    {0x60, nullptr, "DebugActiveProcess"},
    {0x61, nullptr, "BreakDebugProcess"},
    {0x62, nullptr, "TerminateDebugProcess"},
    {0x63, nullptr, "GetDebugEvent"},
    {0x64, nullptr, "ContinueDebugEvent"},
    {0x65, nullptr, "GetProcessList"},
    {0x66, nullptr, "GetThreadList"},
    {0x67, nullptr, "GetDebugThreadContext"},
    {0x68, nullptr, "SetDebugThreadContext"},
    {0x69, nullptr, "QueryDebugProcessMemory"},
    {0x6A, nullptr, "ReadDebugProcessMemory"},
    {0x6B, nullptr, "WriteDebugProcessMemory"},
    {0x6C, nullptr, "SetHardwareBreakPoint"},
    {0x6D, nullptr, "GetDebugThreadParam"},
    {0x6E, nullptr, "Unknown"},
    {0x6F, nullptr, "Unknown"},
    {0x70, nullptr, "CreatePort"},
    {0x71, nullptr, "ManageNamedPort"},
    {0x72, nullptr, "ConnectToPort"},
    {0x73, nullptr, "SetProcessMemoryPermission"},
    {0x74, nullptr, "MapProcessMemory"},
    {0x75, nullptr, "UnmapProcessMemory"},
    {0x76, nullptr, "QueryProcessMemory"},
    {0x77, nullptr, "MapProcessCodeMemory"},
    {0x78, nullptr, "UnmapProcessCodeMemory"},
    {0x79, nullptr, "CreateProcess"},
    {0x7A, nullptr, "StartProcess"},
    {0x7B, nullptr, "TerminateProcess"},
    {0x7C, nullptr, "GetProcessInfo"},
    {0x7D, nullptr, "CreateResourceLimit"},
    {0x7E, nullptr, "SetResourceLimitLimitValue"},
    {0x7F, nullptr, "CallSecureMonitor"},
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

} // namespace Kernel
