// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include "common/common_funcs.h"
#include "common/file_util.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/file_sys/content_archive.h"
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

AppLoader_DeconstructedRomDirectory::AppLoader_DeconstructedRomDirectory(v_file file)
    : AppLoader(file) {}

FileType AppLoader_DeconstructedRomDirectory::IdentifyType(v_file file) {
    if (FileSys::IsDirectoryExeFS(file->GetContainingDirectory())) {
        return FileType::DeconstructedRomDirectory;
    }

    return FileType::Error;
}

ResultStatus AppLoader_DeconstructedRomDirectory::Load(
    Kernel::SharedPtr<Kernel::Process>& process) {
    if (is_loaded) {
        return ResultStatus::ErrorAlreadyLoaded;
    }

    const v_dir dir = file->GetContainingDirectory();
    const v_file npdm = dir->GetFile("main.npdm");
    if (npdm == nullptr)
        return ResultStatus::ErrorInvalidFormat;

    ResultStatus result = metadata.Load(npdm);
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
        const VAddr load_addr = next_load_addr;
        const v_file module_file = dir->GetFile(module);
        if (module_file != nullptr)
            next_load_addr = AppLoader_NSO::LoadModule(module_file, load_addr);
        if (next_load_addr) {
            LOG_DEBUG(Loader, "loaded module {} @ 0x{:X}", module, load_addr);
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
    const auto romfs_iter =
        std::find_if(dir->GetFiles().begin(), dir->GetFiles().end(), [](const v_file& file) {
            return file->GetName().find(".romfs") != std::string::npos;
        });

    // TODO(DarkLordZach): Identify RomFS if its a subdirectory.
    romfs = (romfs_iter == dir->GetFiles().end()) ? nullptr : *romfs_iter;

    is_loaded = true;
    return ResultStatus::Success;
}

ResultStatus AppLoader_DeconstructedRomDirectory::ReadRomFS(v_file& file) {
    if (romfs == nullptr) {
        LOG_DEBUG(Loader, "No RomFS available");
        return ResultStatus::ErrorNotUsed;
    }

    file = romfs;

    return ResultStatus::Success;
}

} // namespace Loader
