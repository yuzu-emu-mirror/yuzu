// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <string>
#include "common/common_types.h"
#include "common/file_util.h"
#include "core/hle/kernel/kernel.h"
#include "core/loader/loader.h"

namespace Loader {

/// Loads an NSO file
class AppLoader_NSO final : public AppLoader {
public:
    AppLoader_NSO(FileUtil::IOFile&& file, std::string filename, std::string filepath)
        : AppLoader(std::move(file)), filename(std::move(filename)), filepath(std::move(filepath)) {
    }

    /**
     * Returns the type of the file
     * @param file FileUtil::IOFile open file
     * @return FileType found, or FileType::Error if this loader doesn't know it
     */
    static FileType IdentifyType(FileUtil::IOFile& file);

    FileType GetFileType() override {
        return IdentifyType(file);
    }

    ResultStatus Load() override;

private:
    struct Symbol {
        Symbol(std::string&& name, u64 value) : name(std::move(name)), value(value) {}
        std::string name;
        u64 value;
    };

    struct Import {
        VAddr ea;
        s64 addend;
    };

    void WriteRelocations(const std::vector<Symbol>& symbols, VAddr load_base,
                          u64 relocation_offset, u64 size, bool is_jump_relocation);
    VAddr GetEntryPoint() const;
    bool LoadNso(const std::string& path, VAddr load_base);
    void Relocate(VAddr load_base, VAddr dynamic_section_addr);

    std::map<std::string, Import> imports;
    std::map<std::string, VAddr> exports;

    std::string filename;
    std::string filepath;
};

} // namespace Loader
