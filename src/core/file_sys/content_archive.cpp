// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/file_sys/content_archive.h"
#include "core/loader/loader.h"
#include "vfs_offset.h"

// Media offsets in headers are stored divided by 512. Mult. by this to get real offset.
constexpr u64 MEDIA_OFFSET_MULTIPLIER = 0x200;

constexpr u64 SECTION_HEADER_SIZE = 0x200;
constexpr u64 SECTION_HEADER_OFFSET = 0x400;

namespace FileSys {
enum class NcaSectionFilesystemType : u8 { PFS0 = 0x2, ROMFS = 0x3 };

struct NcaSectionHeaderBlock {
    INSERT_PADDING_BYTES(3);
    NcaSectionFilesystemType filesystem_type;
    u8 crypto_type;
    INSERT_PADDING_BYTES(3);
};
static_assert(sizeof(NcaSectionHeaderBlock) == 0x8, "NcaSectionHeaderBlock has incorrect size.");

struct Pfs0Superblock {
    NcaSectionHeaderBlock header_block;
    std::array<u8, 0x20> hash;
    u32_le size;
    INSERT_PADDING_BYTES(4);
    u64_le hash_table_offset;
    u64_le hash_table_size;
    u64_le pfs0_header_offset;
    u64_le pfs0_size;
    INSERT_PADDING_BYTES(432);
};
static_assert(sizeof(Pfs0Superblock) == 0x200, "Pfs0Superblock has incorrect size.");

Loader::ResultStatus Nca::Load(v_file file_) {
    file = std::move(file_);

    if (sizeof(NcaHeader) != file->ReadObject(&header))
        NGLOG_CRITICAL(Loader, "File reader errored out during header read.");

    if (!IsValidNca(header))
        return Loader::ResultStatus::ErrorInvalidFormat;

    int number_sections =
        std::count_if(std::begin(header.section_tables), std::end(header.section_tables),
                      [](NcaSectionTableEntry entry) { return entry.media_offset > 0; });

    for (int i = 0; i < number_sections; ++i) {
        // Seek to beginning of this section.
        NcaSectionHeaderBlock block{};
        if (sizeof(NcaSectionHeaderBlock) !=
            file->ReadObject(&block, SECTION_HEADER_OFFSET + i * SECTION_HEADER_SIZE))
            NGLOG_CRITICAL(Loader, "File reader errored out during header read.");

        if (block.filesystem_type == NcaSectionFilesystemType::ROMFS) {
            const size_t romfs_offset =
                header.section_tables[i].media_offset * MEDIA_OFFSET_MULTIPLIER;
            const size_t romfs_size =
                header.section_tables[i].media_end_offset * MEDIA_OFFSET_MULTIPLIER - romfs_offset;
            entries.emplace_back(std::make_shared<RomFs>(
                std::make_shared<OffsetVfsFile>(file, romfs_size, romfs_offset)));
            romfs = entries.back();
        } else if (block.filesystem_type == NcaSectionFilesystemType::PFS0) {
            Pfs0Superblock sb{};
            // Seek back to beginning of this section.
            if (sizeof(Pfs0Superblock) !=
                file->ReadObject(&sb, SECTION_HEADER_OFFSET + i * SECTION_HEADER_SIZE))
                NGLOG_CRITICAL(Loader, "File reader errored out during header read.");

            u64 offset = (static_cast<u64>(header.section_tables[i].media_offset) *
                          MEDIA_OFFSET_MULTIPLIER) +
                         sb.pfs0_header_offset;
            u64 size = MEDIA_OFFSET_MULTIPLIER * (header.section_tables[i].media_end_offset -
                                                  header.section_tables[i].media_offset);
            FileSys::PartitionFilesystem npfs{};
            Loader::ResultStatus status =
                npfs.Load(std::make_shared<OffsetVfsFile>(file, size, offset));

            if (status == Loader::ResultStatus::Success) {
                entries.emplace_back(npfs);
                if (IsDirectoryExeFs(entries.back()))
                    exefs = entries.back();
            }
        }
    }

    return Loader::ResultStatus::Success;
}

std::vector<std::shared_ptr<VfsFile>> Nca::GetFiles() const {
    return {};
}

std::vector<std::shared_ptr<VfsDirectory>> Nca::GetSubdirectories() const {
    return entries;
}

std::string Nca::GetName() const {
    return file->GetName();
}

std::shared_ptr<VfsDirectory> Nca::GetParentDirectory() const {
    return file->GetContainingDirectory();
}

NcaContentType Nca::GetType() const {
    return header.content_type;
}

u64 Nca::GetTitleId() const {
    return header.title_id;
}

v_dir Nca::GetRomFs() const {
    return romfs;
}

v_dir Nca::GetExeFs() const {
    return exefs;
}
} // namespace FileSys
