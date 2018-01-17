// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/core.h"
#include "core/file_sys/filesystem.h"
#include "core/file_sys/storage.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/filesystem/fsp_srv.h"

namespace Service {
namespace FileSystem {

class IStorage final : public ServiceFramework<IStorage> {
public:
    IStorage(std::unique_ptr<FileSys::StorageBackend>&& backend)
        : ServiceFramework("IStorage"), backend(std::move(backend)) {
        static const FunctionInfo functions[] = {
            {0, &IStorage::Read, "Read"},       {1, &IStorage::Write, "Write"},
            {2, &IStorage::Flush, "Flush"},     {3, &IStorage::SetSize, "SetSize"},
            {4, &IStorage::GetSize, "GetSize"},
        };
        RegisterHandlers(functions);
    }

private:
    std::unique_ptr<FileSys::StorageBackend> backend;

    void Read(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        u64 offset = rp.Pop<u64>();
        u64 length = rp.Pop<u64>();

        LOG_DEBUG(Service, "called, offset=0x%llx, length=0x%llx", offset, length);

        auto descriptor = ctx.BufferDescriptorB()[0];
        std::vector<u8> output(length);

        ResultVal<size_t> res = backend->Read(offset, length, output.data());
        if (res.Failed()) {
            IPC::RequestBuilder rb{ctx, 2};
            rb.Push(res.Code());
        }

        Memory::WriteBlock(descriptor.Address(), output.data(), descriptor.Size());

        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void Write(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service, "(STUBBED) called");
    }

    void Flush(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service, "(STUBBED) called");
    }

    void SetSize(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service, "(STUBBED) called");
    }

    void GetSize(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service, "(STUBBED) called");
    }
};

FSP_SRV::FSP_SRV() : ServiceFramework("fsp-srv") {
    static const FunctionInfo functions[] = {
        {1, &FSP_SRV::Initalize, "Initalize"},
        {200, &FSP_SRV::OpenDataStorageByCurrentProcess, "OpenDataStorageByCurrentProcess"},
        {203, &FSP_SRV::OpenRomStorage, "OpenRomStorage"},
        {1005, &FSP_SRV::GetGlobalAccessLogMode, "GetGlobalAccessLogMode"},
    };
    RegisterHandlers(functions);
}

void FSP_SRV::Initalize(Kernel::HLERequestContext& ctx) {
    IPC::RequestBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
    LOG_WARNING(Service, "(STUBBED) called");
}

void FSP_SRV::GetGlobalAccessLogMode(Kernel::HLERequestContext& ctx) {
    IPC::RequestBuilder rb{ctx, 4};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(5);
    LOG_WARNING(Service, "(STUBBED) called");
}

void FSP_SRV::OpenDataStorageByCurrentProcess(Kernel::HLERequestContext& ctx) {
    FileSys::Path path;
    auto filesystem = OpenFileSystem(Type::RomFS, path);
    if (filesystem.Failed()) {
        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(filesystem.Code());
        return;
    }

    auto storage = filesystem.Unwrap()->OpenFile({}, {});
    if (storage.Failed()) {
        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(storage.Code());
        return;
    }

    // TODO: What if already opened?

    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IStorage>(std::move(storage.Unwrap()));
    LOG_WARNING(Service, "(STUBBED) called");
}

void FSP_SRV::OpenRomStorage(Kernel::HLERequestContext& ctx) {
    OpenDataStorageByCurrentProcess(ctx);
}

} // namespace Filesystem
} // namespace Service
