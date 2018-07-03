// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "boost/container/flat_map.hpp"
#include "common/assert.h"
#include "common/common_paths.h"
#include "common/file_util.h"
#include "core/core.h"
#include "core/file_sys/errors.h"
#include "core/file_sys/filesystem.h"
#include "core/file_sys/vfs.h"
#include "core/file_sys/vfs_offset.h"
#include "core/file_sys/vfs_real.h"
#include "core/hle/kernel/process.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/filesystem/fsp_srv.h"

namespace Service::FileSystem {

// Size of emulated sd card free space, reported in bytes.
// Just using 32GB because thats reasonable
// TODO(DarkLordZach): Eventually make this configurable in settings.
constexpr u64 EMULATED_SD_REPORTED_SIZE = 32000000000;

static FileSys::VirtualDir GetDirectoryRelativeWrapped(FileSys::VirtualDir base,
                                                       const std::string& dir_name) {
    if (dir_name == "." || dir_name == "" || dir_name == "/" || dir_name == "\\")
        return base;

    return base->GetDirectoryRelative(dir_name);
}

VfsDirectoryServiceWrapper::VfsDirectoryServiceWrapper(FileSys::VirtualDir backing_)
    : backing(backing_) {}

std::string VfsDirectoryServiceWrapper::GetName() const {
    return backing->GetName();
}

ResultCode VfsDirectoryServiceWrapper::CreateFile(const std::string& path, u64 size) const {
    auto dir = GetDirectoryRelativeWrapped(backing, FileUtil::GetParentPath(path));
    auto file = dir->CreateFile(FileUtil::GetFilename(path));
    if (file == nullptr)
        return ResultCode(-1);
    if (!file->Resize(size))
        return ResultCode(-1);
    return RESULT_SUCCESS;
}

ResultCode VfsDirectoryServiceWrapper::DeleteFile(const std::string& path) const {
    auto dir = GetDirectoryRelativeWrapped(backing, FileUtil::GetParentPath(path));
    if (!backing->DeleteFile(FileUtil::GetFilename(path)))
        return ResultCode(-1);
    return RESULT_SUCCESS;
}

ResultCode VfsDirectoryServiceWrapper::CreateDirectory(const std::string& path) const {
    auto dir = GetDirectoryRelativeWrapped(backing, FileUtil::GetParentPath(path));
    if (dir == nullptr && FileUtil::GetFilename(FileUtil::GetParentPath(path)).empty())
        dir = backing;
    auto new_dir = dir->CreateSubdirectory(FileUtil::GetFilename(path));
    if (new_dir == nullptr)
        return ResultCode(-1);
    return RESULT_SUCCESS;
}

ResultCode VfsDirectoryServiceWrapper::DeleteDirectory(const std::string& path) const {
    auto dir = GetDirectoryRelativeWrapped(backing, FileUtil::GetParentPath(path));
    if (!dir->DeleteSubdirectory(FileUtil::GetFilename(path)))
        return ResultCode(-1);
    return RESULT_SUCCESS;
}

ResultCode VfsDirectoryServiceWrapper::DeleteDirectoryRecursively(const std::string& path) const {
    auto dir = GetDirectoryRelativeWrapped(backing, FileUtil::GetParentPath(path));
    if (!dir->DeleteSubdirectoryRecursive(FileUtil::GetFilename(path)))
        return ResultCode(-1);
    return RESULT_SUCCESS;
}

ResultCode VfsDirectoryServiceWrapper::RenameFile(const std::string& src_path,
                                                  const std::string& dest_path) const {
    auto src = backing->GetFileRelative(src_path);
    if (FileUtil::GetParentPath(src_path) == FileUtil::GetParentPath(dest_path)) {
        // Use more-optimized vfs implementation rename.
        if (src == nullptr)
            return FileSys::ERROR_PATH_NOT_FOUND;
        if (!src->Rename(FileUtil::GetFilename(dest_path)))
            return ResultCode(-1);
        return RESULT_SUCCESS;
    }

    // Move by hand -- TODO(DarkLordZach): Optimize
    auto c_res = CreateFile(dest_path, src->GetSize());
    if (c_res != RESULT_SUCCESS)
        return c_res;

    auto dest = backing->GetFileRelative(dest_path);
    ASSERT_MSG(dest != nullptr, "Newly created file with success cannot be found.");

    ASSERT_MSG(dest->WriteBytes(src->ReadAllBytes()) == src->GetSize(),
               "Could not write all of the bytes but everything else has succeded.");

    if (!src->GetContainingDirectory()->DeleteFile(FileUtil::GetFilename(src_path)))
        return ResultCode(-1);

    return RESULT_SUCCESS;
}

ResultCode VfsDirectoryServiceWrapper::RenameDirectory(const std::string& src_path,
                                                       const std::string& dest_path) const {
    auto src = GetDirectoryRelativeWrapped(backing, src_path);
    if (FileUtil::GetParentPath(src_path) == FileUtil::GetParentPath(dest_path)) {
        // Use more-optimized vfs implementation rename.
        if (src == nullptr)
            return FileSys::ERROR_PATH_NOT_FOUND;
        if (!src->Rename(FileUtil::GetFilename(dest_path)))
            return ResultCode(-1);
        return RESULT_SUCCESS;
    }

    // TODO(DarkLordZach): Implement renaming across the tree (move).
    ASSERT_MSG(false,
               "Could not rename directory with path \"{}\" to new path \"{}\" because parent dirs "
               "don't match -- UNIMPLEMENTED",
               src_path, dest_path);

    return ResultCode(-1);
}

ResultVal<FileSys::VirtualFile> VfsDirectoryServiceWrapper::OpenFile(const std::string& path,
                                                                     FileSys::Mode mode) const {
    auto file = backing->GetFileRelative(path);
    if (file == nullptr)
        return FileSys::ERROR_PATH_NOT_FOUND;

    if ((static_cast<u32>(mode) & static_cast<u32>(FileSys::Mode::Append)) != 0) {
        return MakeResult<FileSys::VirtualFile>(
            std::make_shared<FileSys::OffsetVfsFile>(file, 0, file->GetSize()));
    }

    return MakeResult<FileSys::VirtualFile>(file);
}

ResultVal<FileSys::VirtualDir> VfsDirectoryServiceWrapper::OpenDirectory(const std::string& path) {
    auto dir = GetDirectoryRelativeWrapped(backing, path);
    if (dir == nullptr)
        return ResultCode(-1);
    return MakeResult(dir);
}

u64 VfsDirectoryServiceWrapper::GetFreeSpaceSize() const {
    if (backing->IsWritable())
        return EMULATED_SD_REPORTED_SIZE;

    return 0;
}

ResultVal<FileSys::EntryType> VfsDirectoryServiceWrapper::GetEntryType(
    const std::string& path) const {
    auto dir = GetDirectoryRelativeWrapped(backing, FileUtil::GetParentPath(path));
    if (dir == nullptr)
        return ResultCode(-1);
    auto filename = FileUtil::GetFilename(path);
    if (dir->GetFile(filename) != nullptr)
        return MakeResult(FileSys::EntryType::File);
    if (dir->GetSubdirectory(filename) != nullptr)
        return MakeResult(FileSys::EntryType::Directory);
    return ResultCode(-1);
}

// A deferred filesystem for nand save data.
// This must be deferred because the directory is dependent on title id, which is not set at
// registration time.
struct SaveDataDeferredFilesystem : DeferredFilesystem {
protected:
    FileSys::VirtualDir CreateFilesystem() override {
        u64 title_id = Core::CurrentProcess()->program_id;
        // TODO(DarkLordZach): Users
        u32 user_id = 0;
        std::string nand_directory = fmt::format(
            "{}save/{:016X}/{:08X}/", FileUtil::GetUserPath(D_NAND_IDX), title_id, user_id);

        auto savedata =
            std::make_shared<FileSys::RealVfsDirectory>(nand_directory, FileSys::Mode::Write);
        return savedata;
    }
};

/**
 * Map of registered file systems, identified by type. Once an file system is registered here, it
 * is never removed until UnregisterFileSystems is called.
 */
static boost::container::flat_map<Type, std::unique_ptr<DeferredFilesystem>> filesystem_map;
static FileSys::VirtualFile filesystem_romfs = nullptr;

ResultCode RegisterFileSystem(std::unique_ptr<DeferredFilesystem>&& factory, Type type) {
    auto result = filesystem_map.emplace(type, std::move(factory));

    bool inserted = result.second;
    ASSERT_MSG(inserted, "Tried to register more than one system with same id code");

    auto& filesystem = result.first->second;
    LOG_DEBUG(Service_FS, "Registered file system with id code 0x{:08X}", static_cast<u32>(type));
    return RESULT_SUCCESS;
}

ResultCode RegisterRomFS(FileSys::VirtualFile filesystem) {
    ASSERT_MSG(filesystem_romfs == nullptr,
               "Tried to register more than one system with same id code");

    filesystem_romfs = filesystem;
    LOG_DEBUG(Service_FS, "Registered file system {} with id code 0x{:08X}", filesystem->GetName(),
              static_cast<u32>(Type::RomFS));
    return RESULT_SUCCESS;
}

ResultVal<FileSys::VirtualDir> OpenFileSystem(Type type) {
    LOG_TRACE(Service_FS, "Opening FileSystem with type={}", static_cast<u32>(type));

    auto itr = filesystem_map.find(type);
    if (itr == filesystem_map.end()) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return MakeResult(itr->second->Get());
}

ResultVal<FileSys::VirtualFile> OpenRomFS() {
    if (filesystem_romfs == nullptr)
        return ResultCode(-1);
    return MakeResult(filesystem_romfs);
}

ResultCode FormatFileSystem(Type type) {
    LOG_TRACE(Service_FS, "Formatting FileSystem with type={}", static_cast<u32>(type));

    auto itr = filesystem_map.find(type);
    if (itr == filesystem_map.end()) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return itr->second->Get()->GetParentDirectory()->DeleteSubdirectory(
               itr->second->Get()->GetName())
               ? RESULT_SUCCESS
               : ResultCode(-1);
}

void RegisterFileSystems() {
    filesystem_map.clear();
    filesystem_romfs = nullptr;

    std::string sd_directory = FileUtil::GetUserPath(D_SDMC_IDX);
    auto sdcard = std::make_shared<FileSys::RealVfsDirectory>(sd_directory, FileSys::Mode::Write);
    RegisterFileSystem(std::make_unique<DeferredFilesystem>(sdcard), Type::SDMC);

    RegisterFileSystem(std::make_unique<SaveDataDeferredFilesystem>(), Type::SaveData);
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    RegisterFileSystems();
    std::make_shared<FSP_SRV>()->InstallAsService(service_manager);
}

} // namespace Service::FileSystem
