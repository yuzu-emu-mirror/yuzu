// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
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
            {0, &IStorage::Read, "Read"}, {1, nullptr, "Write"},   {2, nullptr, "Flush"},
            {3, nullptr, "SetSize"},      {4, nullptr, "GetSize"},
        };
        RegisterHandlers(functions);
    }

private:
    std::unique_ptr<FileSys::StorageBackend> backend;

    void Read(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const s64 offset = rp.Pop<s64>();
        const s64 length = rp.Pop<s64>();

        LOG_DEBUG(Service_FS, "called, offset=0x%llx, length=0x%llx", offset, length);

        // Error checking
        if (length < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidLength));
            return;
        }
        if (offset < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidOffset));
            return;
        }

        // Read the data from the Storage backend
        std::vector<u8> output(length);
        ResultVal<size_t> res = backend->Read(offset, length, output.data());
        if (res.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(res.Code());
            return;
        }

        // Write the data to memory
        ctx.WriteBuffer(output);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }
};

class IFile final : public ServiceFramework<IFile> {
public:
    explicit IFile(std::unique_ptr<FileSys::StorageBackend>&& backend)
        : ServiceFramework("IFile"), backend(std::move(backend)) {
        static const FunctionInfo functions[] = {
            {0, &IFile::Read, "Read"}, {1, &IFile::Write, "Write"}, {2, nullptr, "Flush"},
            {3, nullptr, "SetSize"},   {4, nullptr, "GetSize"},
        };
        RegisterHandlers(functions);
    }

private:
    std::unique_ptr<FileSys::StorageBackend> backend;

    void Read(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 unk = rp.Pop<u64>();
        const s64 offset = rp.Pop<s64>();
        const s64 length = rp.Pop<s64>();

        LOG_DEBUG(Service_FS, "called, offset=0x%llx, length=0x%llx", offset, length);

        // Error checking
        if (length < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidLength));
            return;
        }
        if (offset < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidOffset));
            return;
        }

        // Read the data from the Storage backend
        std::vector<u8> output(length);
        ResultVal<size_t> res = backend->Read(offset, length, output.data());
        if (res.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(res.Code());
            return;
        }

        // Write the data to memory
        ctx.WriteBuffer(output);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push(static_cast<u64>(*res));
    }

    void Write(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 unk = rp.Pop<u64>();
        const s64 offset = rp.Pop<s64>();
        const s64 length = rp.Pop<s64>();

        LOG_DEBUG(Service_FS, "called, offset=0x%llx, length=0x%llx", offset, length);

        // Error checking
        if (length < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidLength));
            return;
        }
        if (offset < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidOffset));
            return;
        }

        // Write the data to the Storage backend
        std::vector<u8> data = ctx.ReadBuffer();
        ResultVal<size_t> res = backend->Write(offset, length, true, data.data());
        if (res.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(res.Code());
            return;
        }

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }
};

class IFileSystem final : public ServiceFramework<IFileSystem> {
public:
    explicit IFileSystem(std::unique_ptr<FileSys::FileSystemBackend>&& backend)
        : ServiceFramework("IFileSystem"), backend(std::move(backend)) {
        static const FunctionInfo functions[] = {
            {0, &IFileSystem::CreateFile, "CreateFile"},
            {7, &IFileSystem::GetEntryType, "GetEntryType"},
            {8, &IFileSystem::OpenFile, "OpenFile"},
            {10, &IFileSystem::Commit, "Commit"},
        };
        RegisterHandlers(functions);
    }

    void CreateFile(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        auto file_buffer = ctx.ReadBuffer();
        auto end = std::find(file_buffer.begin(), file_buffer.end(), '\0');

        std::string name(file_buffer.begin(), end);

        u64 mode = rp.Pop<u64>();
        u32 size = rp.Pop<u32>();

        LOG_DEBUG(Service_FS, "called file %s mode 0x%" PRIX64 " size 0x%08X", name.c_str(), mode,
                  size);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(backend->CreateFile(name, size));
    }

    void OpenFile(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        auto file_buffer = ctx.ReadBuffer();
        auto end = std::find(file_buffer.begin(), file_buffer.end(), '\0');

        std::string name(file_buffer.begin(), end);

        auto mode = static_cast<FileSys::Mode>(rp.Pop<u32>());

        LOG_DEBUG(Service_FS, "called file %s mode %u", name.c_str(), static_cast<u32>(mode));

        auto result = backend->OpenFile(name, mode);
        if (result.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result.Code());
            return;
        }

        auto file = std::move(result.Unwrap());

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IFile>(std::move(file));
    }

    void GetEntryType(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        auto file_buffer = ctx.ReadBuffer();
        auto end = std::find(file_buffer.begin(), file_buffer.end(), '\0');

        std::string name(file_buffer.begin(), end);

        LOG_DEBUG(Service_FS, "called file %s", name.c_str());

        auto result = backend->GetEntryType(name);
        if (result.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result.Code());
            return;
        }

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(static_cast<u32>(*result));
    }

    void Commit(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_FS, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

private:
    std::unique_ptr<FileSys::FileSystemBackend> backend;
};

FSP_SRV::FSP_SRV() : ServiceFramework("fsp-srv") {
    static const FunctionInfo functions[] = {
        {1, &FSP_SRV::Initalize, "Initalize"},
        {18, &FSP_SRV::MountSdCard, "MountSdCard"},
        {22, &FSP_SRV::CreateSaveData, "CreateSaveData"},
        {51, &FSP_SRV::MountSaveData, "MountSaveData"},
        {200, &FSP_SRV::OpenDataStorageByCurrentProcess, "OpenDataStorageByCurrentProcess"},
        {202, nullptr, "OpenDataStorageByDataId"},
        {203, &FSP_SRV::OpenRomStorage, "OpenRomStorage"},
        {1005, &FSP_SRV::GetGlobalAccessLogMode, "GetGlobalAccessLogMode"},
    };
    RegisterHandlers(functions);
}

void FSP_SRV::TryLoadRomFS() {
    if (romfs) {
        return;
    }
    FileSys::Path unused;
    auto res = OpenFileSystem(Type::RomFS, unused);
    if (res.Succeeded()) {
        romfs = std::move(res.Unwrap());
    }
}

void FSP_SRV::Initalize(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_FS, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void FSP_SRV::MountSdCard(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_FS, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void FSP_SRV::CreateSaveData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    auto save_struct = rp.PopRaw<std::array<u8, 0x40>>();
    auto save_create_struct = rp.PopRaw<std::array<u8, 0x40>>();
    u128 uid = rp.PopRaw<u128>();

    LOG_WARNING(Service_FS, "(STUBBED) called uid = %016" PRIX64 "%016" PRIX64, uid[1], uid[0]);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void FSP_SRV::MountSaveData(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_FS, "(STUBBED) called");

    FileSys::Path unused;
    auto filesystem = OpenFileSystem(Type::SaveData, unused).Unwrap();

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IFileSystem>(std::move(filesystem));
}

void FSP_SRV::GetGlobalAccessLogMode(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_FS, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(5);
}

void FSP_SRV::OpenDataStorageByCurrentProcess(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_FS, "called");

    TryLoadRomFS();
    if (!romfs) {
        // TODO (bunnei): Find the right error code to use here
        LOG_CRITICAL(Service_FS, "no file system interface available!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultCode(-1));
        return;
    }

    // Attempt to open a StorageBackend interface to the RomFS
    auto storage = romfs->OpenFile({}, {});
    if (storage.Failed()) {
        LOG_CRITICAL(Service_FS, "no storage interface available!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(storage.Code());
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IStorage>(std::move(storage.Unwrap()));
}

void FSP_SRV::OpenRomStorage(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_FS, "(STUBBED) called, using OpenDataStorageByCurrentProcess");
    OpenDataStorageByCurrentProcess(ctx);
}

} // namespace FileSystem
} // namespace Service
