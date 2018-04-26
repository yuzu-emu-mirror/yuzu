// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include "common/common_funcs.h"
#include "common/common_paths.h"
#include "common/file_util.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/file_sys/romfs_factory.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/loader/deconstructed_rom_directory.h"
#include "core/loader/nso.h"
#include "core/memory.h"

namespace Loader {

static std::string FindRomFS(const std::string& directory) {
    std::string filepath_romfs;
    const auto callback = [&filepath_romfs](unsigned*, const std::string& directory,
                                            const std::string& virtual_name) -> bool {
        const std::string physical_name = directory + virtual_name;
        if (FileUtil::IsDirectory(physical_name)) {
            // Skip directories
            return true;
        }

        // Verify extension
        const std::string extension = physical_name.substr(physical_name.find_last_of(".") + 1);
        if (Common::ToLower(extension) != "romfs") {
            return true;
        }

        // Found it - we are done
        filepath_romfs = std::move(physical_name);
        return false;
    };

    // Search the specified directory recursively, looking for the first .romfs file, which will
    // be used for the RomFS
    FileUtil::ForeachDirectoryEntry(nullptr, directory, callback);

    return filepath_romfs;
}

AppLoader_DeconstructedRomDirectory::AppLoader_DeconstructedRomDirectory(FileUtil::IOFile&& file,
                                                                         std::string filepath)
    : AppLoader(std::move(file)), filepath(std::move(filepath)) {}

FileType AppLoader_DeconstructedRomDirectory::IdentifyType(FileUtil::IOFile& file,
                                                           const std::string& filepath) {
    bool is_main_found{};
    bool is_npdm_found{};
    bool is_rtld_found{};
    bool is_sdk_found{};

    const auto callback = [&](unsigned* num_entries_out, const std::string& directory,
                              const std::string& virtual_name) -> bool {
        // Skip directories
        std::string physical_name = directory + virtual_name;
        if (FileUtil::IsDirectory(physical_name)) {
            return true;
        }

        // Verify filename
        if (Common::ToLower(virtual_name) == "main") {
            is_main_found = true;
        } else if (Common::ToLower(virtual_name) == "main.npdm") {
            is_npdm_found = true;
            return true;
        } else if (Common::ToLower(virtual_name) == "rtld") {
            is_rtld_found = true;
        } else if (Common::ToLower(virtual_name) == "sdk") {
            is_sdk_found = true;
        } else {
            // Continue searching
            return true;
        }

        // Verify file is an NSO
        FileUtil::IOFile file(physical_name, "rb");
        if (AppLoader_NSO::IdentifyType(file, physical_name) != FileType::NSO) {
            return false;
        }

        // We are done if we've found and verified all required NSOs
        return !(is_main_found && is_npdm_found && is_rtld_found && is_sdk_found);
    };

    // Search the directory recursively, looking for the required modules
    const std::string directory = filepath.substr(0, filepath.find_last_of("/\\")) + DIR_SEP;
    FileUtil::ForeachDirectoryEntry(nullptr, directory, callback);

    if (is_main_found && is_npdm_found && is_rtld_found && is_sdk_found) {
        return FileType::DeconstructedRomDirectory;
    }

    return FileType::Error;
}

ResultStatus AppLoader_DeconstructedRomDirectory::Load(
    Kernel::SharedPtr<Kernel::Process>& process) {
    if (is_loaded) {
        return ResultStatus::ErrorAlreadyLoaded;
    }
    if (!file.IsOpen()) {
        return ResultStatus::Error;
    }

    const std::string directory = filepath.substr(0, filepath.find_last_of("/\\")) + DIR_SEP;
    const std::string npdm_path = directory + DIR_SEP + "main.npdm";

    ResultStatus result = metadata.Load(npdm_path);
    if (result != ResultStatus::Success) {
        return result;
    }
    metadata.Print();

    const FileSys::ProgramAddressSpaceType arch_bits{metadata.GetAddressSpaceType()};
    if (arch_bits == FileSys::ProgramAddressSpaceType::Is32Bit) {
        return ResultStatus::ErrorUnsupportedArch;
    }

    // Load NSO modules
    VAddr next_load_addr{Memory::PROCESS_IMAGE_VADDR};
    for (const auto& module : {"rtld", "main", "subsdk0", "subsdk1", "subsdk2", "subsdk3",
                               "subsdk4", "subsdk5", "subsdk6", "subsdk7", "sdk"}) {
        const std::string path = directory + DIR_SEP + module;
        const VAddr load_addr = next_load_addr;
        next_load_addr = AppLoader_NSO::LoadModule(path, load_addr);
        if (next_load_addr) {
            NGLOG_DEBUG(Loader, "loaded module {} @ {:#X}", module, load_addr);
        } else {
            next_load_addr = load_addr;
        }
    }

    process->program_id = metadata.GetTitleID();
    process->svc_access_mask.set();
    process->address_mappings = default_address_mappings;
    process->resource_limit =
        Kernel::ResourceLimit::GetForCategory(Kernel::ResourceLimitCategory::APPLICATION);
    process->Run(Memory::PROCESS_IMAGE_VADDR, metadata.GetMainThreadPriority(),
                 metadata.GetMainThreadStackSize());

    // Find the RomFS by searching for a ".romfs" file in this directory
    filepath_romfs = FindRomFS(directory);

    // Register the RomFS if a ".romfs" file was found
    if (!filepath_romfs.empty()) {
        Service::FileSystem::RegisterFileSystem(std::make_unique<FileSys::RomFS_Factory>(*this),
                                                Service::FileSystem::Type::RomFS);
    }

    is_loaded = true;
    return ResultStatus::Success;
}

ResultStatus AppLoader_DeconstructedRomDirectory::ReadRomFS(
    std::shared_ptr<FileUtil::IOFile>& romfs_file, u64& offset, u64& size) {

    if (filepath_romfs.empty()) {
        NGLOG_DEBUG(Loader, "No RomFS available");
        return ResultStatus::ErrorNotUsed;
    }

    // We reopen the file, to allow its position to be independent
    romfs_file = std::make_shared<FileUtil::IOFile>(filepath_romfs, "rb");
    if (!romfs_file->IsOpen()) {
        return ResultStatus::Error;
    }

    offset = 0;
    size = romfs_file->GetSize();

    NGLOG_DEBUG(Loader, "RomFS offset:           {:#018X}", offset);
    NGLOG_DEBUG(Loader, "RomFS size:             {:#018X}", size);

    // Reset read pointer
    file.Seek(0, SEEK_SET);

    return ResultStatus::Success;
}

} // namespace Loader
