// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include <cstring>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "common/assert.h"
#include "common/common_types.h"
#include "common/hex_util.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/directory.h"
#include "core/file_sys/errors.h"
#include "core/file_sys/mode.h"
#include "core/file_sys/nca_metadata.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/romfs.h"
#include "core/file_sys/romfs_factory.h"
#include "core/file_sys/savedata_factory.h"
#include "core/file_sys/system_archive/system_archive.h"
#include "core/file_sys/vfs.h"
#include "core/file_sys/vfs_ro_layer.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/readable_event.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/filesystem/fsp_srv.h"
#include "core/reporter.h"

namespace Service::FileSystem {

struct SizeGetter {
    std::function<u64()> get_free_size;
    std::function<u64()> get_total_size;

    static SizeGetter FromStorageId(const FileSystemController& fsc, FileSys::StorageId id) {
        return {
            [&fsc, id] { return fsc.GetFreeSpaceSize(id); },
            [&fsc, id] { return fsc.GetTotalSpaceSize(id); },
        };
    }
};

enum class FileSystemType : u8 {
    Invalid0 = 0,
    Invalid1 = 1,
    Logo = 2,
    ContentControl = 3,
    ContentManual = 4,
    ContentMeta = 5,
    ContentData = 6,
    ApplicationPackage = 7,
};

enum class SaveDataOpenMode {
    Normal,
    ReadOnly,
    System,
};

class IStorage final : public ServiceFramework<IStorage> {
public:
    explicit IStorage(FileSys::VirtualFile backend_)
        : ServiceFramework("IStorage"), backend(std::move(backend_)) {
        static const FunctionInfo functions[] = {
            {0, &IStorage::Read, "Read"},
            {1, nullptr, "Write"},
            {2, nullptr, "Flush"},
            {3, nullptr, "SetSize"},
            {4, &IStorage::GetSize, "GetSize"},
            {5, nullptr, "OperateRange"},
        };
        RegisterHandlers(functions);
    }

private:
    FileSys::VirtualFile backend;

    void Read(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const s64 offset = rp.Pop<s64>();
        const s64 length = rp.Pop<s64>();

        LOG_DEBUG(Service_FS, "called, offset=0x{:X}, length={}", offset, length);

        // Error checking
        if (length < 0) {
            LOG_ERROR(Service_FS, "Length is less than 0, length={}", length);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(FileSys::ERROR_INVALID_SIZE);
            return;
        }
        if (offset < 0) {
            LOG_ERROR(Service_FS, "Offset is less than 0, offset={}", offset);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(FileSys::ERROR_INVALID_OFFSET);
            return;
        }

        // Read the data from the Storage backend
        std::vector<u8> output = backend->ReadBytes(length, offset);
        // Write the data to memory
        ctx.WriteBuffer(output);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void GetSize(Kernel::HLERequestContext& ctx) {
        const u64 size = backend->GetSize();
        LOG_DEBUG(Service_FS, "called, size={}", size);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u64>(size);
    }
};

class IFile final : public ServiceFramework<IFile> {
public:
    explicit IFile(FileSys::VirtualFile backend_)
        : ServiceFramework("IFile"), backend(std::move(backend_)) {
        static const FunctionInfo functions[] = {
            {0, &IFile::Read, "Read"},       {1, &IFile::Write, "Write"},
            {2, &IFile::Flush, "Flush"},     {3, &IFile::SetSize, "SetSize"},
            {4, &IFile::GetSize, "GetSize"}, {5, nullptr, "OperateRange"},
        };
        RegisterHandlers(functions);
    }

private:
    FileSys::VirtualFile backend;

    void Read(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 option = rp.Pop<u64>();
        const s64 offset = rp.Pop<s64>();
        const s64 length = rp.Pop<s64>();

        LOG_DEBUG(Service_FS, "called, option={}, offset=0x{:X}, length={}", option, offset,
                  length);

        // Error checking
        if (length < 0) {
            LOG_ERROR(Service_FS, "Length is less than 0, length={}", length);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(FileSys::ERROR_INVALID_SIZE);
            return;
        }
        if (offset < 0) {
            LOG_ERROR(Service_FS, "Offset is less than 0, offset={}", offset);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(FileSys::ERROR_INVALID_OFFSET);
            return;
        }

        // Read the data from the Storage backend
        std::vector<u8> output = backend->ReadBytes(length, offset);

        // Write the data to memory
        ctx.WriteBuffer(output);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push(static_cast<u64>(output.size()));
    }

    void Write(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 option = rp.Pop<u64>();
        const s64 offset = rp.Pop<s64>();
        const s64 length = rp.Pop<s64>();

        LOG_DEBUG(Service_FS, "called, option={}, offset=0x{:X}, length={}", option, offset,
                  length);

        // Error checking
        if (length < 0) {
            LOG_ERROR(Service_FS, "Length is less than 0, length={}", length);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(FileSys::ERROR_INVALID_SIZE);
            return;
        }
        if (offset < 0) {
            LOG_ERROR(Service_FS, "Offset is less than 0, offset={}", offset);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(FileSys::ERROR_INVALID_OFFSET);
            return;
        }

        const std::vector<u8> data = ctx.ReadBuffer();

        ASSERT_MSG(
            static_cast<s64>(data.size()) <= length,
            "Attempting to write more data than requested (requested={:016X}, actual={:016X}).",
            length, data.size());

        // Write the data to the Storage backend
        const auto write_size =
            static_cast<std::size_t>(std::distance(data.begin(), data.begin() + length));
        const std::size_t written = backend->Write(data.data(), write_size, offset);

        ASSERT_MSG(static_cast<s64>(written) == length,
                   "Could not write all bytes to file (requested={:016X}, actual={:016X}).", length,
                   written);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void Flush(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_FS, "called");

        // Exists for SDK compatibiltity -- No need to flush file.

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void SetSize(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 size = rp.Pop<u64>();
        LOG_DEBUG(Service_FS, "called, size={}", size);

        backend->Resize(size);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void GetSize(Kernel::HLERequestContext& ctx) {
        const u64 size = backend->GetSize();
        LOG_DEBUG(Service_FS, "called, size={}", size);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u64>(size);
    }
};

template <typename T>
static void BuildEntryIndex(std::vector<FileSys::Entry>& entries, const std::vector<T>& new_data,
                            FileSys::EntryType type) {
    entries.reserve(entries.size() + new_data.size());

    for (const auto& new_entry : new_data) {
        entries.emplace_back(new_entry->GetName(), type, new_entry->GetSize());
    }
}

class IDirectory final : public ServiceFramework<IDirectory> {
public:
    explicit IDirectory(FileSys::VirtualDir backend_)
        : ServiceFramework("IDirectory"), backend(std::move(backend_)) {
        static const FunctionInfo functions[] = {
            {0, &IDirectory::Read, "Read"},
            {1, &IDirectory::GetEntryCount, "GetEntryCount"},
        };
        RegisterHandlers(functions);

        // TODO(DarkLordZach): Verify that this is the correct behavior.
        // Build entry index now to save time later.
        BuildEntryIndex(entries, backend->GetFiles(), FileSys::File);
        BuildEntryIndex(entries, backend->GetSubdirectories(), FileSys::Directory);
    }

private:
    FileSys::VirtualDir backend;
    std::vector<FileSys::Entry> entries;
    u64 next_entry_index = 0;

    void Read(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_FS, "called.");

        // Calculate how many entries we can fit in the output buffer
        const u64 count_entries = ctx.GetWriteBufferSize() / sizeof(FileSys::Entry);

        // Cap at total number of entries.
        const u64 actual_entries = std::min(count_entries, entries.size() - next_entry_index);

        // Determine data start and end
        const auto* begin = reinterpret_cast<u8*>(entries.data() + next_entry_index);
        const auto* end = reinterpret_cast<u8*>(entries.data() + next_entry_index + actual_entries);
        const auto range_size = static_cast<std::size_t>(std::distance(begin, end));

        next_entry_index += actual_entries;

        // Write the data to memory
        ctx.WriteBuffer(begin, range_size);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push(actual_entries);
    }

    void GetEntryCount(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_FS, "called");

        u64 count = entries.size() - next_entry_index;

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push(count);
    }
};

class IFileSystem final : public ServiceFramework<IFileSystem> {
public:
    explicit IFileSystem(FileSys::VirtualDir backend, SizeGetter size)
        : ServiceFramework("IFileSystem"), backend(std::move(backend)), size(std::move(size)) {
        static const FunctionInfo functions[] = {
            {0, &IFileSystem::CreateFile, "CreateFile"},
            {1, &IFileSystem::DeleteFile, "DeleteFile"},
            {2, &IFileSystem::CreateDirectory, "CreateDirectory"},
            {3, &IFileSystem::DeleteDirectory, "DeleteDirectory"},
            {4, &IFileSystem::DeleteDirectoryRecursively, "DeleteDirectoryRecursively"},
            {5, &IFileSystem::RenameFile, "RenameFile"},
            {6, nullptr, "RenameDirectory"},
            {7, &IFileSystem::GetEntryType, "GetEntryType"},
            {8, &IFileSystem::OpenFile, "OpenFile"},
            {9, &IFileSystem::OpenDirectory, "OpenDirectory"},
            {10, &IFileSystem::Commit, "Commit"},
            {11, nullptr, "GetFreeSpaceSize"},
            {12, nullptr, "GetTotalSpaceSize"},
            {13, &IFileSystem::CleanDirectoryRecursively, "CleanDirectoryRecursively"},
            {14, nullptr, "GetFileTimeStampRaw"},
            {15, nullptr, "QueryEntry"},
        };
        RegisterHandlers(functions);
    }

    void CreateFile(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        const auto file_buffer = ctx.ReadBuffer();
        const std::string name = Common::StringFromBuffer(file_buffer);

        const u64 mode = rp.Pop<u64>();
        const u32 size = rp.Pop<u32>();

        LOG_DEBUG(Service_FS, "called. file={}, mode=0x{:X}, size=0x{:08X}", name, mode, size);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(backend.CreateFile(name, size));
    }

    void DeleteFile(Kernel::HLERequestContext& ctx) {
        const auto file_buffer = ctx.ReadBuffer();
        const std::string name = Common::StringFromBuffer(file_buffer);

        LOG_DEBUG(Service_FS, "called. file={}", name);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(backend.DeleteFile(name));
    }

    void CreateDirectory(Kernel::HLERequestContext& ctx) {
        const auto file_buffer = ctx.ReadBuffer();
        const std::string name = Common::StringFromBuffer(file_buffer);

        LOG_DEBUG(Service_FS, "called. directory={}", name);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(backend.CreateDirectory(name));
    }

    void DeleteDirectory(Kernel::HLERequestContext& ctx) {
        const auto file_buffer = ctx.ReadBuffer();
        const std::string name = Common::StringFromBuffer(file_buffer);

        LOG_DEBUG(Service_FS, "called. directory={}", name);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(backend.DeleteDirectory(name));
    }

    void DeleteDirectoryRecursively(Kernel::HLERequestContext& ctx) {
        const auto file_buffer = ctx.ReadBuffer();
        const std::string name = Common::StringFromBuffer(file_buffer);

        LOG_DEBUG(Service_FS, "called. directory={}", name);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(backend.DeleteDirectoryRecursively(name));
    }

    void CleanDirectoryRecursively(Kernel::HLERequestContext& ctx) {
        const auto file_buffer = ctx.ReadBuffer();
        const std::string name = Common::StringFromBuffer(file_buffer);

        LOG_DEBUG(Service_FS, "called. Directory: {}", name);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(backend.CleanDirectoryRecursively(name));
    }

    void RenameFile(Kernel::HLERequestContext& ctx) {
        std::vector<u8> buffer;
        buffer.resize(ctx.BufferDescriptorX()[0].Size());
        Memory::ReadBlock(ctx.BufferDescriptorX()[0].Address(), buffer.data(), buffer.size());
        const std::string src_name = Common::StringFromBuffer(buffer);

        buffer.resize(ctx.BufferDescriptorX()[1].Size());
        Memory::ReadBlock(ctx.BufferDescriptorX()[1].Address(), buffer.data(), buffer.size());
        const std::string dst_name = Common::StringFromBuffer(buffer);

        LOG_DEBUG(Service_FS, "called. file '{}' to file '{}'", src_name, dst_name);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(backend.RenameFile(src_name, dst_name));
    }

    void OpenFile(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        const auto file_buffer = ctx.ReadBuffer();
        const std::string name = Common::StringFromBuffer(file_buffer);

        const auto mode = static_cast<FileSys::Mode>(rp.Pop<u32>());

        LOG_DEBUG(Service_FS, "called. file={}, mode={}", name, static_cast<u32>(mode));

        auto result = backend.OpenFile(name, mode);
        if (result.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result.Code());
            return;
        }

        IFile file(result.Unwrap());

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IFile>(std::move(file));
    }

    void OpenDirectory(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        const auto file_buffer = ctx.ReadBuffer();
        const std::string name = Common::StringFromBuffer(file_buffer);

        // TODO(Subv): Implement this filter.
        const u32 filter_flags = rp.Pop<u32>();

        LOG_DEBUG(Service_FS, "called. directory={}, filter={}", name, filter_flags);

        auto result = backend.OpenDirectory(name);
        if (result.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result.Code());
            return;
        }

        IDirectory directory(result.Unwrap());

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDirectory>(std::move(directory));
    }

    void GetEntryType(Kernel::HLERequestContext& ctx) {
        const auto file_buffer = ctx.ReadBuffer();
        const std::string name = Common::StringFromBuffer(file_buffer);

        LOG_DEBUG(Service_FS, "called. file={}", name);

        auto result = backend.GetEntryType(name);
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

    void GetFreeSpaceSize(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_FS, "called");

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push(size.get_free_size());
    }

    void GetTotalSpaceSize(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_FS, "called");

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push(size.get_total_size());
    }

private:
    VfsDirectoryServiceWrapper backend;
    SizeGetter size;
};

class ISaveDataInfoReader final : public ServiceFramework<ISaveDataInfoReader> {
public:
    explicit ISaveDataInfoReader(FileSystemController& fsc,
                                 std::vector<FileSys::SaveDataSpaceId> spaces)
        : ServiceFramework("ISaveDataInfoReader"), fsc(fsc) {
        static const FunctionInfo functions[] = {
            {0, &ISaveDataInfoReader::ReadSaveDataInfo, "ReadSaveDataInfo"},
        };
        RegisterHandlers(functions);

        for (const auto& space : spaces) {
            FindAllSaves(space);
        }
    }

    void ReadSaveDataInfo(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_FS, "called");

        // Calculate how many entries we can fit in the output buffer
        const u64 count_entries = ctx.GetWriteBufferSize() / sizeof(SaveDataInfo);

        // Cap at total number of entries.
        const u64 actual_entries = std::min(count_entries, info.size() - next_entry_index);

        // Determine data start and end
        const auto* begin = reinterpret_cast<u8*>(info.data() + next_entry_index);
        const auto* end = reinterpret_cast<u8*>(info.data() + next_entry_index + actual_entries);
        const auto range_size = static_cast<std::size_t>(std::distance(begin, end));

        next_entry_index += actual_entries;

        // Write the data to memory
        ctx.WriteBuffer(begin, range_size);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(static_cast<u32>(actual_entries));
    }

private:
    static u64 stoull_be(std::string_view str) {
        if (str.size() != 16)
            return 0;

        const auto bytes = Common::HexStringToArray<0x8>(str);
        u64 out{};
        std::memcpy(&out, bytes.data(), sizeof(u64));

        return Common::swap64(out);
    }

    void FindAllSaves(FileSys::SaveDataSpaceId space) {
        const auto save_root = fsc.OpenSaveDataSpace(space);

        if (save_root.Failed() || *save_root == nullptr) {
            LOG_ERROR(Service_FS, "The save root for the space_id={:02X} was invalid!",
                      static_cast<u8>(space));
            return;
        }

        for (const auto& type : (*save_root)->GetSubdirectories()) {
            if (type->GetName() == "save") {
                for (const auto& save_id : type->GetSubdirectories()) {
                    for (const auto& user_id : save_id->GetSubdirectories()) {
                        const auto save_id_numeric = stoull_be(save_id->GetName());
                        auto user_id_numeric = Common::HexStringToArray<0x10>(user_id->GetName());
                        std::reverse(user_id_numeric.begin(), user_id_numeric.end());

                        if (save_id_numeric != 0) {
                            // System Save Data
                            info.emplace_back(SaveDataInfo{
                                0,
                                space,
                                FileSys::SaveDataType::SystemSaveData,
                                {},
                                user_id_numeric,
                                save_id_numeric,
                                0,
                                user_id->GetSize(),
                                {},
                            });

                            continue;
                        }

                        for (const auto& title_id : user_id->GetSubdirectories()) {
                            const auto device =
                                std::all_of(user_id_numeric.begin(), user_id_numeric.end(),
                                            [](u8 val) { return val == 0; });
                            info.emplace_back(SaveDataInfo{
                                0,
                                space,
                                device ? FileSys::SaveDataType::DeviceSaveData
                                       : FileSys::SaveDataType::SaveData,
                                {},
                                user_id_numeric,
                                save_id_numeric,
                                stoull_be(title_id->GetName()),
                                title_id->GetSize(),
                                {},
                            });
                        }
                    }
                }
            } else if (space == FileSys::SaveDataSpaceId::TemporaryStorage) {
                // Temporary Storage
                for (const auto& user_id : type->GetSubdirectories()) {
                    for (const auto& title_id : user_id->GetSubdirectories()) {
                        if (!title_id->GetFiles().empty() ||
                            !title_id->GetSubdirectories().empty()) {
                            auto user_id_numeric =
                                Common::HexStringToArray<0x10>(user_id->GetName());
                            std::reverse(user_id_numeric.begin(), user_id_numeric.end());

                            info.emplace_back(SaveDataInfo{
                                0,
                                space,
                                FileSys::SaveDataType::TemporaryStorage,
                                {},
                                user_id_numeric,
                                stoull_be(type->GetName()),
                                stoull_be(title_id->GetName()),
                                title_id->GetSize(),
                                {},
                            });
                        }
                    }
                }
            }
        }
    }

    struct SaveDataInfo {
        u64_le save_id_unknown;
        FileSys::SaveDataSpaceId space;
        FileSys::SaveDataType type;
        INSERT_PADDING_BYTES(0x6);
        std::array<u8, 0x10> user_id;
        u64_le save_id;
        u64_le title_id;
        u64_le save_image_size;
        u16_le index;
        FileSys::SaveDataRank rank;
        INSERT_PADDING_BYTES(0x25);
    };
    static_assert(sizeof(SaveDataInfo) == 0x60, "SaveDataInfo has incorrect size.");

    FileSystemController& fsc;
    std::vector<SaveDataInfo> info;
    u64 next_entry_index = 0;
};

class IEventNotifier final : public ServiceFramework<IEventNotifier> {
public:
    explicit IEventNotifier(Kernel::SharedPtr<Kernel::ReadableEvent> event)
        : ServiceFramework{"IEventNotifier"}, event(std::move(event)) {
        static const FunctionInfo functions[] = {
            {0, &IEventNotifier::GetEventHandle, "GetEventHandle"},
        };

        RegisterHandlers(functions);
    }

private:
    void GetEventHandle(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_FS, "called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(event);
    }

    Kernel::SharedPtr<Kernel::ReadableEvent> event;
};

FSP_SRV::FSP_SRV(Core::System& system)
    : ServiceFramework("fsp-srv"), system(system), fsc(system.GetFileSystemController()) {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "OpenFileSystem"},
        {1, &FSP_SRV::SetCurrentProcess, "SetCurrentProcess"},
        {2, nullptr, "OpenDataFileSystemByCurrentProcess"},
        {7, &FSP_SRV::OpenFileSystemWithPatch, "OpenFileSystemWithPatch"},
        {8, nullptr, "OpenFileSystemWithId"},
        {9, nullptr, "OpenDataFileSystemByApplicationId"},
        {11, &FSP_SRV::OpenBisFileSystem, "OpenBisFileSystem"},
        {12, &FSP_SRV::OpenBisStorage, "OpenBisStorage"},
        {13, &FSP_SRV::InvalidateBisCache, "InvalidateBisCache"},
        {17, nullptr, "OpenHostFileSystem"},
        {18, &FSP_SRV::OpenSdCardFileSystem, "OpenSdCardFileSystem"},
        {19, nullptr, "FormatSdCardFileSystem"},
        {21, nullptr, "DeleteSaveDataFileSystem"},
        {22, &FSP_SRV::CreateSaveDataFileSystem, "CreateSaveDataFileSystem"},
        {23, &FSP_SRV::CreateSaveDataFileSystemBySystemSaveDataId, "CreateSaveDataFileSystemBySystemSaveDataId"},
        {24, nullptr, "RegisterSaveDataFileSystemAtomicDeletion"},
        {25, nullptr, "DeleteSaveDataFileSystemBySaveDataSpaceId"},
        {26, nullptr, "FormatSdCardDryRun"},
        {27, nullptr, "IsExFatSupported"},
        {28, nullptr, "DeleteSaveDataFileSystemBySaveDataAttribute"},
        {30, nullptr, "OpenGameCardStorage"},
        {31, nullptr, "OpenGameCardFileSystem"},
        {32, nullptr, "ExtendSaveDataFileSystem"},
        {33, nullptr, "DeleteCacheStorage"},
        {34, nullptr, "GetCacheStorageSize"},
        {35, nullptr, "CreateSaveDataFileSystemByHashSalt"},
        {51, &FSP_SRV::OpenSaveDataFileSystem, "OpenSaveDataFileSystem"},
        {52, &FSP_SRV::OpenSaveDataFileSystemBySystemSaveDataId, "OpenSaveDataFileSystemBySystemSaveDataId"},
        {53, &FSP_SRV::OpenReadOnlySaveDataFileSystem, "OpenReadOnlySaveDataFileSystem"},
        {57, nullptr, "ReadSaveDataFileSystemExtraDataBySaveDataSpaceId"},
        {58, nullptr, "ReadSaveDataFileSystemExtraData"},
        {59, nullptr, "WriteSaveDataFileSystemExtraData"},
        {60, &FSP_SRV::OpenSaveDataInfoReader, "OpenSaveDataInfoReader"},
        {61, &FSP_SRV::OpenSaveDataInfoReaderBySaveDataSpaceId, "OpenSaveDataInfoReaderBySaveDataSpaceId"},
        {62, nullptr, "OpenCacheStorageList"},
        {64, nullptr, "OpenSaveDataInternalStorageFileSystem"},
        {65, nullptr, "UpdateSaveDataMacForDebug"},
        {66, nullptr, "WriteSaveDataFileSystemExtraData2"},
        {67, nullptr, "FindSaveDataWithFilter"},
        {68, nullptr, "OpenSaveDataInfoReaderBySaveDataFilter"},
        {80, nullptr, "OpenSaveDataMetaFile"},
        {81, nullptr, "OpenSaveDataTransferManager"},
        {82, nullptr, "OpenSaveDataTransferManagerVersion2"},
        {83, nullptr, "OpenSaveDataTransferProhibiterForCloudBackUp"},
        {84, nullptr, "ListApplicationAccessibleSaveDataOwnerId"},
        {100, &FSP_SRV::OpenImageDirectoryFileSystem, "OpenImageDirectoryFileSystem"},
        {110, &FSP_SRV::OpenContentStorageFileSystem, "OpenContentStorageFileSystem"},
        {120, nullptr, "OpenCloudBackupWorkStorageFileSystem"},
        {130, nullptr, "OpenCustomStorageFileSystem"},
        {200, &FSP_SRV::OpenDataStorageByCurrentProcess, "OpenDataStorageByCurrentProcess"},
        {201, nullptr, "OpenDataStorageByProgramId"},
        {202, &FSP_SRV::OpenDataStorageByDataId, "OpenDataStorageByDataId"},
        {203, &FSP_SRV::OpenPatchDataStorageByCurrentProcess, "OpenPatchDataStorageByCurrentProcess"},
        {204, nullptr, "OpenDataFileSystemByProgramIndex"},
        {205, nullptr, "OpenDataStorageByProgramIndex"},
        {400, nullptr, "OpenDeviceOperator"},
        {500, &FSP_SRV::OpenSdCardDetectionEventNotifier, "OpenSdCardDetectionEventNotifier"},
        {501, &FSP_SRV::OpenGameCardDetectionEventNotifier, "OpenGameCardDetectionEventNotifier"},
        {510, nullptr, "OpenSystemDataUpdateEventNotifier"},
        {511, nullptr, "NotifySystemDataUpdateEvent"},
        {520, nullptr, "SimulateGameCardDetectionEvent"},
        {600, nullptr, "SetCurrentPosixTime"},
        {601, nullptr, "QuerySaveDataTotalSize"},
        {602, nullptr, "VerifySaveDataFileSystem"},
        {603, nullptr, "CorruptSaveDataFileSystem"},
        {604, nullptr, "CreatePaddingFile"},
        {605, nullptr, "DeleteAllPaddingFiles"},
        {606, nullptr, "GetRightsId"},
        {607, nullptr, "RegisterExternalKey"},
        {608, nullptr, "UnregisterAllExternalKey"},
        {609, nullptr, "GetRightsIdByPath"},
        {610, nullptr, "GetRightsIdAndKeyGenerationByPath"},
        {611, nullptr, "SetCurrentPosixTimeWithTimeDifference"},
        {612, nullptr, "GetFreeSpaceSizeForSaveData"},
        {613, nullptr, "VerifySaveDataFileSystemBySaveDataSpaceId"},
        {614, nullptr, "CorruptSaveDataFileSystemBySaveDataSpaceId"},
        {615, nullptr, "QuerySaveDataInternalStorageTotalSize"},
        {616, nullptr, "GetSaveDataCommitId"},
        {617, nullptr, "UnregisterExternalKey"},
        {620, &FSP_SRV::SetSdCardEncryptionSeed, "SetSdCardEncryptionSeed"},
        {630, nullptr, "SetSdCardAccessibility"},
        {631, nullptr, "IsSdCardAccessible"},
        {640, nullptr, "IsSignedSystemPartitionOnSdCardValid"},
        {700, nullptr, "OpenAccessFailureResolver"},
        {701, nullptr, "GetAccessFailureDetectionEvent"},
        {702, nullptr, "IsAccessFailureDetected"},
        {710, nullptr, "ResolveAccessFailure"},
        {720, nullptr, "AbandonAccessFailure"},
        {800, nullptr, "GetAndClearFileSystemProxyErrorInfo"},
        {810, nullptr, "RegisterProgramIndexMapInfo"},
        {1000, nullptr, "SetBisRootForHost"},
        {1001, nullptr, "SetSaveDataSize"},
        {1002, nullptr, "SetSaveDataRootPath"},
        {1003, nullptr, "DisableAutoSaveDataCreation"},
        {1004, &FSP_SRV::SetGlobalAccessLogMode, "SetGlobalAccessLogMode"},
        {1005, &FSP_SRV::GetGlobalAccessLogMode, "GetGlobalAccessLogMode"},
        {1006, &FSP_SRV::OutputAccessLogToSdCard, "OutputAccessLogToSdCard"},
        {1007, nullptr, "RegisterUpdatePartition"},
        {1008, nullptr, "OpenRegisteredUpdatePartition"},
        {1009, nullptr, "GetAndClearMemoryReportInfo"},
        {1010, nullptr, "SetDataStorageRedirectTarget"},
        {1011, &FSP_SRV::GetAccessLogVersionInfo, "GetAccessLogVersionInfo"},
        {1100, nullptr, "OverrideSaveDataTransferTokenSignVerificationKey"},
        {1110, nullptr, "CorruptSaveDataFileSystemBySaveDataSpaceId2"},
        {1200, nullptr, "OpenMultiCommitManager"},
    };
    // clang-format on
    RegisterHandlers(functions);

    auto& kernel{system.Kernel()};
    sd_card_detection_event = Kernel::WritableEvent::CreateEventPair(
        kernel, Kernel::ResetType::Automatic, "fsp-srv:SdCardDetectionEvent");
    game_card_detection_event = Kernel::WritableEvent::CreateEventPair(
        kernel, Kernel::ResetType::Automatic, "fsp-srv:GameCardDetectionEvent");
}

FSP_SRV::~FSP_SRV() = default;

void FSP_SRV::SetCurrentProcess(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    current_process_id = rp.Pop<u64>();

    LOG_DEBUG(Service_FS, "called. current_process_id=0x{:016X}", current_process_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void FSP_SRV::OpenFileSystemWithPatch(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const auto type = rp.PopRaw<FileSystemType>();
    const auto title_id = rp.PopRaw<u64>();
    LOG_DEBUG(Service_FS, "called with type={}, title_id={:016X}", static_cast<u8>(type), title_id);

    const auto& prov{system.GetContentProvider()};

    FileSys::PatchManager pm{title_id};
    FileSys::ContentRecordType cr_type;

    switch (type) {
    case FileSystemType::ApplicationPackage:
    case FileSystemType::Logo:
        cr_type = FileSys::ContentRecordType::Program;
        break;
    case FileSystemType::ContentControl:
        cr_type = FileSys::ContentRecordType::Control;
        break;
    case FileSystemType::ContentManual:
        cr_type = FileSys::ContentRecordType::HtmlDocument;
        break;
    case FileSystemType::ContentMeta:
        cr_type = FileSys::ContentRecordType::Meta;
        break;
    case FileSystemType::ContentData:
        cr_type = FileSys::ContentRecordType::Data;
        break;
    default:
        LOG_WARNING(Service_FS, "called with invalid filesystem type!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(FileSys::ERROR_INVALID_ARGUMENT);
        return;
    }

    const auto& nca{prov.GetEntry(title_id, cr_type)};
    if (nca == nullptr) {
        LOG_WARNING(Service_FS, "NCA requested doesn't exist in content provider!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(FileSys::ERROR_INVALID_ARGUMENT);
        return;
    }

    FileSys::VirtualDir dir;

    if (type == FileSystemType::ApplicationPackage) {
        dir = nca->GetExeFS();
        if (dir != nullptr)
            dir = pm.PatchExeFS(dir);
    } else if (type == FileSystemType::Logo) {
        dir = nca->GetSubdirectories()[1];
    } else if (type == FileSystemType::ContentControl || type == FileSystemType::ContentManual ||
               type == FileSystemType::ContentData) {
        if (nca->GetRomFS() != nullptr) {
            const auto romfs = pm.PatchRomFS(nca->GetRomFS(), nca->GetBaseIVFCOffset(), cr_type);
            if (romfs != nullptr)
                dir = FileSys::ExtractRomFS(romfs);
        }
    } else {
        dir = nca->GetSubdirectories()[0];
    }

    if (dir == nullptr) {
        LOG_WARNING(Service_FS, "couldn't get requested NCA section!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(FileSys::ERROR_INVALID_ARGUMENT);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface(std::make_shared<IFileSystem>(
        dir, SizeGetter::FromStorageId(fsc, FileSys::StorageId::Host)));
}

void FSP_SRV::OpenBisFileSystem(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto partition = rp.PopRaw<FileSys::BisPartitionId>();

    LOG_DEBUG(Service_FS, "called with partition_id={:08X}", static_cast<u32>(partition));

    auto dir = fsc.OpenBISPartition(partition);

    if (dir.Failed()) {
        LOG_ERROR(Service_FS,
                  "Failed to mount BIS filesystem for partition_id={:08X}! Could be invalid "
                  "argument or uninitialized system.",
                  static_cast<u32>(partition));
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(dir.Code());
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);

    IFileSystem fs(dir.Unwrap(), SizeGetter::FromStorageId(fsc, FileSys::StorageId::Host));
    rb.PushIpcInterface<IFileSystem>(std::move(fs));
}

void FSP_SRV::OpenBisStorage(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto partition = rp.PopRaw<FileSys::BisPartitionId>();

    LOG_DEBUG(Service_FS, "called with partition_id={:08X}", static_cast<u32>(partition));

    auto file = fsc.OpenBISPartitionStorage(partition);

    if (file.Failed()) {
        LOG_ERROR(Service_FS,
                  "Failed to mount BIS storage for partition_id={:08X}! Could be invalid "
                  "argument or uninitialized system.",
                  static_cast<u32>(partition));
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(file.Code());
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);

    IStorage fs(file.Unwrap());
    rb.PushIpcInterface<IStorage>(std::move(fs));
}

void FSP_SRV::InvalidateBisCache(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_FS, "called");

    // Exists for SDK compatibility -- We do not emulate a BIS cache.

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void FSP_SRV::OpenSdCardFileSystem(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_FS, "called");

    IFileSystem filesystem(fsc.OpenSDMC().Unwrap(),
                           SizeGetter::FromStorageId(fsc, FileSys::StorageId::SdCard));

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IFileSystem>(std::move(filesystem));
}

void FSP_SRV::CreateSaveDataFileSystem(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    auto save_struct = rp.PopRaw<FileSys::SaveDataDescriptor>();
    auto save_create_struct = rp.PopRaw<std::array<u8, 0x40>>();
    u128 uid = rp.PopRaw<u128>();

    LOG_DEBUG(Service_FS, "called save_struct = {}, uid = {:016X}{:016X}", save_struct.DebugInfo(),
              uid[1], uid[0]);

    fsc.CreateSaveData(FileSys::SaveDataSpaceId::NandUser, save_struct);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void FSP_SRV::CreateSaveDataFileSystemBySystemSaveDataId(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    auto save_struct = rp.PopRaw<FileSys::SaveDataDescriptor>();
    auto save_create_struct = rp.PopRaw<std::array<u8, 0x40>>();

    LOG_DEBUG(Service_FS, "called save_struct = {}", save_struct.DebugInfo());

    const auto dir = fsc.CreateSaveData(FileSys::SaveDataSpaceId::NandSystem, save_struct);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(dir.Code());
}

namespace {

FileSys::StorageId StorageFromSaveDataSpace(FileSys::SaveDataSpaceId space) {
    switch (space) {
    case FileSys::SaveDataSpaceId::NandSystem:
    case FileSys::SaveDataSpaceId::ProperSystem:
    case FileSys::SaveDataSpaceId::TemporaryStorage:
        return FileSys::StorageId::NandSystem;
    case FileSys::SaveDataSpaceId::NandUser:
        return FileSys::StorageId::NandUser;
    case FileSys::SaveDataSpaceId::SdCardSystem:
    case FileSys::SaveDataSpaceId::SdCardUser:
        return FileSys::StorageId::SdCard;
    default:
        return FileSys::StorageId::None;
    }
}

template <SaveDataOpenMode mode>
void OpenSaveDataFileSystemGeneric(Kernel::HLERequestContext& ctx, FileSystemController& fsc) {
    IPC::RequestParser rp{ctx};

    struct Parameters {
        FileSys::SaveDataSpaceId save_data_space_id;
        FileSys::SaveDataDescriptor descriptor;
    };

    const auto parameters = rp.PopRaw<Parameters>();
    auto dir = fsc.OpenSaveData(parameters.save_data_space_id, parameters.descriptor);

    if (dir.Failed()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(FileSys::ERROR_ENTITY_NOT_FOUND);
        return;
    }

    auto diru = dir.Unwrap();

    if (diru == nullptr) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(FileSys::ERROR_ENTITY_NOT_FOUND);
        return;
    }

    if constexpr (mode == SaveDataOpenMode::ReadOnly) {
        diru = std::make_shared<FileSys::ReadOnlyVfsDirectoryLayer>(diru);
    }

    IFileSystem filesystem(
        std::move(diru),
        SizeGetter::FromStorageId(fsc, StorageFromSaveDataSpace(parameters.save_data_space_id)));

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IFileSystem>(std::move(filesystem));
}

} // namespace

void FSP_SRV::OpenSaveDataFileSystem(Kernel::HLERequestContext& ctx) {
    OpenSaveDataFileSystemGeneric<SaveDataOpenMode::Normal>(ctx, fsc);
}

void FSP_SRV::OpenReadOnlySaveDataFileSystem(Kernel::HLERequestContext& ctx) {
    OpenSaveDataFileSystemGeneric<SaveDataOpenMode::ReadOnly>(ctx, fsc);
}

void FSP_SRV::OpenSaveDataFileSystemBySystemSaveDataId(Kernel::HLERequestContext& ctx) {
    OpenSaveDataFileSystemGeneric<SaveDataOpenMode::System>(ctx, fsc);
}

void FSP_SRV::OpenSaveDataInfoReader(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_FS, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<ISaveDataInfoReader>(
        std::make_shared<ISaveDataInfoReader>(fsc, std::vector<FileSys::SaveDataSpaceId>{
                                                       FileSys::SaveDataSpaceId::NandSystem,
                                                       FileSys::SaveDataSpaceId::NandUser,
                                                       FileSys::SaveDataSpaceId::TemporaryStorage,
                                                       FileSys::SaveDataSpaceId::SdCardUser,
                                                   }));
}

void FSP_SRV::OpenSaveDataInfoReaderBySaveDataSpaceId(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto space = rp.PopRaw<FileSys::SaveDataSpaceId>();
    LOG_INFO(Service_FS, "called, space={}", static_cast<u8>(space));

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<ISaveDataInfoReader>(
        std::make_shared<ISaveDataInfoReader>(fsc, std::vector<FileSys::SaveDataSpaceId>{space}));
}

void FSP_SRV::OpenImageDirectoryFileSystem(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto storage = rp.PopRaw<ImageDirectoryId>();
    LOG_DEBUG(Service_FS, "called, storage={:08X}", static_cast<u32>(storage));

    auto dir = fsc.GetImageDirectory(storage);

    if (dir == nullptr) {
        LOG_ERROR(Service_FS, "The image directory requested was invalid!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(FileSys::ERROR_INVALID_ARGUMENT);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface(std::make_shared<IFileSystem>(
        std::move(dir), SizeGetter::FromStorageId(fsc, storage == ImageDirectoryId::NAND
                                                           ? FileSys::StorageId::NandUser
                                                           : FileSys::StorageId::SdCard)));
}

void FSP_SRV::OpenContentStorageFileSystem(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto storage = rp.PopRaw<ContentStorageId>();

    LOG_DEBUG(Service_FS, "called, storage={:08X}", static_cast<u32>(storage));

    auto dir = fsc.GetContentDirectory(storage);

    if (dir == nullptr) {
        LOG_ERROR(Service_FS, "The content storage requested was invalid!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(FileSys::ERROR_INVALID_ARGUMENT);
        return;
    }

    FileSys::StorageId storage_id = FileSys::StorageId::None;
    switch (storage) {
    case ContentStorageId::SdCard:
        storage_id = FileSys::StorageId::SdCard;
        break;
    case ContentStorageId::User:
        storage_id = FileSys::StorageId::NandUser;
        break;
    case ContentStorageId::System:
        storage_id = FileSys::StorageId::NandSystem;
        break;
    }

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface(
        std::make_shared<IFileSystem>(std::move(dir), SizeGetter::FromStorageId(fsc, storage_id)));
}

void FSP_SRV::SetGlobalAccessLogMode(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    log_mode = rp.PopEnum<LogMode>();

    LOG_DEBUG(Service_FS, "called, log_mode={:08X}", static_cast<u32>(log_mode));

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void FSP_SRV::GetGlobalAccessLogMode(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_FS, "called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.PushEnum(log_mode);
}

void FSP_SRV::OpenDataStorageByCurrentProcess(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_FS, "called");

    auto romfs = fsc.OpenRomFSCurrentProcess();
    if (romfs.Failed()) {
        // TODO (bunnei): Find the right error code to use here
        LOG_CRITICAL(Service_FS, "no file system interface available!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultCode(-1));
        return;
    }

    IStorage storage(std::move(romfs.Unwrap()));

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IStorage>(std::move(storage));
}

void FSP_SRV::OpenDataStorageByDataId(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto storage_id = rp.PopRaw<FileSys::StorageId>();
    const auto unknown = rp.PopRaw<u32>();
    const auto title_id = rp.PopRaw<u64>();

    LOG_DEBUG(Service_FS, "called with storage_id={:02X}, unknown={:08X}, title_id={:016X}",
              static_cast<u8>(storage_id), unknown, title_id);

    auto data = fsc.OpenRomFS(title_id, storage_id, FileSys::ContentRecordType::Data);

    if (data.Failed()) {
        const auto archive = FileSys::SystemArchive::SynthesizeSystemArchive(title_id);

        if (archive != nullptr) {
            IPC::ResponseBuilder rb{ctx, 2, 0, 1};
            rb.Push(RESULT_SUCCESS);
            rb.PushIpcInterface(std::make_shared<IStorage>(archive));
            return;
        }

        // TODO(DarkLordZach): Find the right error code to use here
        LOG_ERROR(Service_FS,
                  "could not open data storage with title_id={:016X}, storage_id={:02X}", title_id,
                  static_cast<u8>(storage_id));
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultCode(-1));
        return;
    }

    FileSys::PatchManager pm{title_id};

    IStorage storage(pm.PatchRomFS(std::move(data.Unwrap()), 0, FileSys::ContentRecordType::Data));

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IStorage>(std::move(storage));
}

void FSP_SRV::OpenPatchDataStorageByCurrentProcess(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    auto storage_id = rp.PopRaw<FileSys::StorageId>();
    auto title_id = rp.PopRaw<u64>();

    LOG_DEBUG(Service_FS, "called with storage_id={:02X}, title_id={:016X}",
              static_cast<u8>(storage_id), title_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(FileSys::ERROR_ENTITY_NOT_FOUND);
}

void FSP_SRV::OutputAccessLogToSdCard(Kernel::HLERequestContext& ctx) {
    const auto raw = ctx.ReadBuffer();
    auto log = Common::StringFromFixedZeroTerminatedBuffer(
        reinterpret_cast<const char*>(raw.data()), raw.size());

    LOG_DEBUG(Service_FS, "called, log='{}'", log);

    system.GetReporter().SaveFilesystemAccessReport(log_mode, std::move(log));

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void FSP_SRV::GetAccessLogVersionInfo(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_FS, "called");

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(RESULT_SUCCESS);
    rb.PushEnum(AccessLogVersion::Latest);
    rb.Push(access_log_program_index);
}

void FSP_SRV::OpenSdCardDetectionEventNotifier(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_FS, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface(std::make_shared<IEventNotifier>(sd_card_detection_event.readable));
}

void FSP_SRV::OpenGameCardDetectionEventNotifier(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_FS, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface(std::make_shared<IEventNotifier>(game_card_detection_event.readable));
}

void FSP_SRV::SetSdCardEncryptionSeed(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto seed = rp.PopRaw<u128>();

    LOG_INFO(Service_FS, "called with seed={:016X}{:016X}", seed[1], seed[0]);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

} // namespace Service::FileSystem
