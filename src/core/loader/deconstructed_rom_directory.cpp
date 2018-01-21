// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/common_funcs.h"
#include "common/common_paths.h"
#include "common/file_util.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/loader/deconstructed_rom_directory.h"
#include "core/loader/nso.h"
#include "core/memory.h"

namespace Loader {

AppLoader_DeconstructedRomDirectory::AppLoader_DeconstructedRomDirectory(FileUtil::IOFile&& file,
                                                                         std::string filepath)
    : AppLoader(std::move(file)), filepath(std::move(filepath)) {}

FileType AppLoader_DeconstructedRomDirectory::IdentifyType(FileUtil::IOFile& file,
                                                           const std::string& filepath) {
    bool is_main_found{};
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
        } else if (Common::ToLower(virtual_name) == "rtld") {
            is_rtld_found = true;
        } else if (Common::ToLower(virtual_name) == "sdk") {
            is_sdk_found = true;
        } else {
            // Contrinue searching
            return true;
        }

        // Verify file is an NSO
        FileUtil::IOFile file(physical_name, "rb");
        if (AppLoader_NSO::IdentifyType(file, physical_name) != FileType::NSO) {
            return false;
        }

        // We are done if we've found and verified all required NSOs
        return !(is_main_found && is_rtld_found && is_sdk_found);
    };

    // Search the directory recursively, looking for the required modules
    const std::string directory = filepath.substr(0, filepath.find_last_of("/\\")) + DIR_SEP;
    FileUtil::ForeachDirectoryEntry(nullptr, directory, callback);

    if (is_main_found && is_rtld_found && is_sdk_found) {
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

    process = Kernel::Process::Create("main");

    // Load NSO modules
    VAddr next_load_addr{Memory::PROCESS_IMAGE_VADDR};
    for (const auto& module : {"rtld", "main", "subsdk0", "subsdk1", "subsdk2", "subsdk3",
                               "subsdk4", "subsdk5", "subsdk6", "subsdk7", "sdk"}) {
        const std::string path =
            filepath.substr(0, filepath.find_last_of("/\\")) + DIR_SEP + module;
        const VAddr load_addr = next_load_addr;
        next_load_addr = AppLoader_NSO::LoadModule(path, load_addr);
        if (next_load_addr) {
            LOG_DEBUG(Loader, "loaded module %s @ 0x%llx", module, load_addr);
        } else {
            next_load_addr = load_addr;
        }
    }

    process->svc_access_mask.set();
    process->address_mappings = default_address_mappings;
    process->resource_limit =
        Kernel::ResourceLimit::GetForCategory(Kernel::ResourceLimitCategory::APPLICATION);
    process->Run(Memory::PROCESS_IMAGE_VADDR, 48, Kernel::DEFAULT_STACK_SIZE);

    is_loaded = true;
    return ResultStatus::Success;
}

} // namespace Loader
