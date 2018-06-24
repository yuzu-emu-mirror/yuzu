// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "partition_filesystem.h"

namespace FileSys {

enum class NcaContentType : u8 { Program = 0, Meta = 1, Control = 2, Manual = 3, Data = 4 };

struct NcaSectionTableEntry {
    u32_le media_offset;
    u32_le media_end_offset;
    INSERT_PADDING_BYTES(0x8);
};
static_assert(sizeof(NcaSectionTableEntry) == 0x10, "NcaSectionTableEntry has incorrect size.");

struct NcaHeader {
    std::array<u8, 0x100> rsa_signature_1;
    std::array<u8, 0x100> rsa_signature_2;
    u32_le magic;
    u8 is_system;
    NcaContentType content_type;
    u8 crypto_type;
    u8 key_index;
    u64_le size;
    u64_le title_id;
    INSERT_PADDING_BYTES(0x4);
    u32_le sdk_version;
    u8 crypto_type_2;
    INSERT_PADDING_BYTES(15);
    std::array<u8, 0x10> rights_id;
    std::array<NcaSectionTableEntry, 0x4> section_tables;
    std::array<std::array<u8, 0x20>, 0x4> hash_tables;
    std::array<std::array<u8, 0x10>, 0x4> key_area;
    INSERT_PADDING_BYTES(0xC0);
};
static_assert(sizeof(NcaHeader) == 0x400, "NcaHeader has incorrect size.");

static bool IsDirectoryExeFs(std::shared_ptr<FileSys::VfsDirectory> pfs) {
    // According to switchbrew, an exefs must only contain these two files:
    return pfs->GetFile("main") != nullptr && pfs->GetFile("main.ndpm") != nullptr;
}

static bool IsValidNca(const NcaHeader& header) {
    return header.magic == Common::MakeMagic('N', 'C', 'A', '2') ||
           header.magic == Common::MakeMagic('N', 'C', 'A', '3');
}

class Nca : public ReadOnlyVfsDirectory {
    std::vector<v_dir> entries;
    std::vector<u64> entry_offset;

    v_dir romfs = nullptr;
    v_dir exefs = nullptr;
    v_file file;

    NcaHeader header{};

public:
    Loader::ResultStatus Load(v_file file);

    std::vector<std::shared_ptr<VfsFile>> GetFiles() const override;
    std::vector<std::shared_ptr<VfsDirectory>> GetSubdirectories() const override;
    std::string GetName() const override;
    std::shared_ptr<VfsDirectory> GetParentDirectory() const override;

    NcaContentType GetType() const;
    u64 GetTitleId() const;

    v_dir GetRomFs() const;
    v_dir GetExeFs() const;
};

} // namespace FileSys
