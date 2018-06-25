// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "boost/container/flat_map.hpp"
#include "common/common_paths.h"
#include "common/file_util.h"
#include "core/file_sys/filesystem.h"
#include "core/file_sys/vfs.h"
#include "core/file_sys/vfs_real.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/filesystem/fsp_srv.h"

namespace Service::FileSystem {

VfsDirectoryServiceWrapper::VfsDirectoryServiceWrapper(v_dir backing_) : backing(backing_) { }

std::string VfsDirectoryServiceWrapper::GetName() const {
    return backing->GetName();
}

// TODO(DarkLordZach): Verify path usage.
ResultCode VfsDirectoryServiceWrapper::CreateFile(const std::string& path, u64 size) const {
    filesystem::path s_path(path);
    auto dir = backing->GetDirectoryRelative(s_path.parent_path());
    auto file = dir->CreateFile(path.filename().string());
    if (file == nullptr) return ResultCode(-1);
    if (!file->Resize(size)) return ResultCode(-1);
    return RESULT_SUCCESS;
}

// TODO(DarkLordZach): Verify path usage.
ResultCode VfsDirectoryServiceWrapper::DeleteFile(const std::string& path) const {
    filesystem::path s_path(path);
    auto dir = backing->GetDirectoryRelative(s_path.parent_path());
    if (!backing->DeleteFile(s_path.filename().string())) return ResultCode(-1);
    return RESULT_SUCCESS;
}

// TODO(DarkLordZach): Verify path usage.
ResultCode VfsDirectoryServiceWrapper::CreateDirectory(const std::string& path) const {
    filesystem::path s_path(path);
    auto dir = backing->GetDirectoryRelative(s_path.parent_path());
    auto new_dir = dir->CreateSubdirectory(path.filename().name);
    if (new_dir == nullptr) return ResultCode(-1);
    return RESULT_SUCCESS;
}

// TODO(DarkLordZach): Verify path usage.
ResultCode VfsDirectoryServiceWrapper::DeleteDirectory(const std::string& path) const {
    filesystem::path s_path(path);
    auto dir = backing->GetDirectoryRelative(s_path.parent_path());
    if (!dir->DeleteSubdirectory(path.filename().string())) return ResultCode(-1);
    return RESULT_SUCCESS;
}

// TODO(DarkLordZach): Verify path usage.
ResultCode VfsDirectoryServiceWrapper::DeleteDirectoryRecursively(const std::string& path) const {
    filesystem::path s_path(path);
    auto dir = backing->GetDirectoryRelative(s_path.parent_path());
    if (!dir->DeleteSubdirectoryRecursive(path.filename().string())) return ResultCode(-1);
    return RESULT_SUCCESS;
}

// TODO(DarkLordZach): Verify path usage.
ResultCode VfsDirectoryServiceWrapper::RenameFile(const std::string& src_path, const std::string& dest_path) const {
    filesystem::path s_path(src_path);
    auto file = backing->GetFileRelative(s_path);
    file->GetContainingDirectory()->DeleteFile(file->GetName());
    auto res_code = CreateFile(dest_path, file->GetSize());
    if (res_code != RESULT_SUCCESS) return res_code;
    auto file2 = backing->GetFileRelative(filesystem::path(dest_path));
    if (file2->WriteBytes(file->ReadAllBytes() != file->GetSize()) return ResultCode(-1);
    return RESULT_SUCCESS;
}

// TODO(DarkLordZach): Verify path usage.
ResultCode
VfsDirectoryServiceWrapper::RenameDirectory(const std::string& src_path, const std::string& dest_path) const {
    filesystem::path s_path(src_path);
    auto file = backing->GetFileRelative(s_path);
    file->GetContainingDirectory()->DeleteFile(file->GetName());
    auto res_code = CreateFile(dest_path, file->GetSize());
    if (res_code != RESULT_SUCCESS) return res_code;
    auto file2 = backing->GetFileRelative(filesystem::path(dest_path));
    if (file2->WriteBytes(file->ReadAllBytes() != file->GetSize())) return ResultCode(-1);
    return RESULT_SUCCESS;
}

// TODO(DarkLordZach): Verify path usage.
ResultVal<v_file> VfsDirectoryServiceWrapper::OpenFile(const std::string& path, FileSys::Mode mode) const {
    auto file = backing->GetFileRelative(filesystem::path(path));
    if (file == nullptr) return ResultVal<v_file>(-1);
    if (mode == FileSys::Mode::Append) return MakeResult(std::make_shared<OffsetVfsFile>(file, 0, file->GetSize()));
    else if (mode == FileSys::Mode::Write && file->IsWritable()) return file;
    else if (mode == FileSys::Mode::Read && file->IsReadable()) return file;
    return ResultVal<v_file>(-1);
}

// TODO(DarkLordZach): Verify path usage.
ResultVal<v_dir> VfsDirectoryServiceWrapper::OpenDirectory(const std::string& path) const {
    auto dir = backing->GetDirectoryRelative(filesystem::path(path));
    if (dir == nullptr) return ResultVal<v_dir>(-1);
    return MakeResult(RESULT_SUCCESS, dir);
}

u64 VfsDirectoryServiceWrapper::GetFreeSpaceSize() const {
    // TODO(DarkLordZach): Infinite? Actual? Is this actually used productively or...?
    return 0;
}

// TODO(DarkLordZach): Verify path usage.
ResultVal<FileSys::EntryType> VfsDirectoryServiceWrapper::GetEntryType(const std::string& path) const {
    filesystem::path r_path(path);
    auto dir = backing->GetDirectoryRelative(r_path.parent_path());
    if (dir == nullptr) return ResultVal<FileSys::EntryType>(-1);
    if (dir->GetFile(r_path.filename().string()) != nullptr) return MakeResult(FileSys::EntryType::File);
    if (dir->GetSubdirectory(r_path.filename().string()) != nullptr) return MakeResult(FileSys::EntryType::Directory);
    return ResultVal<FileSys::EntryType>(-1);
}

/**
 * Map of registered file systems, identified by type. Once an file system is registered here, it
 * is never removed until UnregisterFileSystems is called.
 */
static boost::container::flat_map<Type, v_dir> filesystem_map;
static v_file filesystem_romfs;

ResultCode RegisterFileSystem(v_dir factory, Type type) {
    auto result = filesystem_map.emplace(type, factory);

    bool inserted = result.second;
    ASSERT_MSG(inserted, "Tried to register more than one system with same id code");

    auto& filesystem = result.first->second;
    LOG_DEBUG(Service_FS, "Registered file system {} with id code 0x{:08X}", filesystem->GetName(),
              static_cast<u32>(type));
    return RESULT_SUCCESS;
}

ResultCode RegisterRomFS(v_file filesystem) {
    bool inserted = filesystem_romfs == nullptr;
    ASSERT_MSG(inserted, "Tried to register more than one system with same id code");

    filesystem_romfs = filesystem;
    NGLOG_DEBUG(Service_FS, "Registered file system {} with id code 0x{:08X}",
                filesystem->GetName(), static_cast<u32>(Type::RomFS));
    return RESULT_SUCCESS;
}

ResultVal<v_dir> OpenFileSystem(Type type) {
    NGLOG_TRACE(Service_FS, "Opening FileSystem with type={}", static_cast<u32>(type));

    auto itr = filesystem_map.find(type);
    if (itr == filesystem_map.end()) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return MakeResult(itr->second);
}

ResultVal<v_file> OpenRomFS() {
    if (filesystem_romfs == nullptr) return ResultVal<v_file>(-1);
    return MakeResult(filesystem_romfs);
}

ResultCode FormatFileSystem(Type type) {
    LOG_TRACE(Service_FS, "Formatting FileSystem with type={}", static_cast<u32>(type));

    auto itr = filesystem_map.find(type);
    if (itr == filesystem_map.end()) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return itr->second->GetParentDirectory()->DeleteSubdirectory(itr->second->GetName())
               ? RESULT_SUCCESS
               : ResultCode(-1);
}

void RegisterFileSystems() {
    filesystem_map.clear();

    std::string nand_directory = FileUtil::GetUserPath(D_NAND_IDX);
    std::string sd_directory = FileUtil::GetUserPath(D_SDMC_IDX);

    auto savedata =
        std::make_unique<FileSys::RealVfsDirectory>(nand_directory, filesystem::perms::all);
    RegisterFileSystem(std::move(savedata), Type::SaveData);

    auto sdcard = std::make_unique<FileSys::RealVfsDirectory>(sd_directory, filesystem::perms::all);
    RegisterFileSystem(std::move(sdcard), Type::SDMC);
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    RegisterFileSystems();
    std::make_shared<FSP_SRV>()->InstallAsService(service_manager);
}

} // namespace Service::FileSystem
