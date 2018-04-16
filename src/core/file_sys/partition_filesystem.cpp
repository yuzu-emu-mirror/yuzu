// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include "common/file_util.h"
#include "common/logging/log.h"
#include "core/file_sys/partition_filesystem.h"
#include "core/loader/loader.h"

namespace FileSys {

Loader::ResultStatus PartitionFilesystem::Load(const std::string& file_path, size_t offset) {
    FileUtil::IOFile file(file_path, "rb");
    if (!file.IsOpen())
        return Loader::ResultStatus::Error;

    // At least be as large as the header
    if (file.GetSize() < sizeof(Header))
        return Loader::ResultStatus::Error;

    // For cartridges, HFSs can get very large, so we need to calculate the size up to
    // the actual content itself instead of just blindly reading in the entire file.
    Header pfs_header;
    if (!file.ReadBytes(&pfs_header, sizeof(Header)))
        return Loader::ResultStatus::Error;

    bool is_hfs = (memcmp(pfs_header.magic.data(), "HFS", 3) == 0);
    size_t entry_size = is_hfs ? sizeof(HFSEntry) : sizeof(PFSEntry);
    size_t metadata_size =
        sizeof(Header) + (pfs_header.num_entries * entry_size) + pfs_header.strtab_size;

    // Actually read in now...
    file.Seek(offset, SEEK_SET);
    std::vector<u8> file_data(metadata_size);

    if (!file.ReadBytes(file_data.data(), metadata_size))
        return Loader::ResultStatus::Error;

    Loader::ResultStatus result = Load(file_data);
    if (result != Loader::ResultStatus::Success)
        LOG_ERROR(Service_FS, "Failed to load PFS from file %s!", file_path.c_str());

    return result;
}

Loader::ResultStatus PartitionFilesystem::Load(const std::vector<u8> file_data, size_t offset) {
    size_t total_size = static_cast<size_t>(file_data.size() - offset);
    if (total_size < sizeof(Header))
        return Loader::ResultStatus::Error;

    memcpy(&pfs_header, &file_data[offset], sizeof(Header));
    is_hfs = (memcmp(pfs_header.magic.data(), "HFS", 3) == 0);

    size_t entries_offset = offset + sizeof(Header);
    size_t entry_size = is_hfs ? sizeof(HFSEntry) : sizeof(PFSEntry);
    size_t strtab_offset = entries_offset + (pfs_header.num_entries * entry_size);
    for (u16 i = 0; i < pfs_header.num_entries; i++) {
        FileEntry entry;

        memcpy(&entry.fs_entry, &file_data[entries_offset + (i * entry_size)], sizeof(FSEntry));
        entry.name = std::string(reinterpret_cast<const char*>(
            &file_data[strtab_offset + entry.fs_entry.strtab_offset]));
        pfs_entries.push_back(entry);
    }

    content_offset = strtab_offset + pfs_header.strtab_size;

    return Loader::ResultStatus::Success;
}

u32 PartitionFilesystem::GetNumEntries(void) const {
    return pfs_header.num_entries;
}

u64 PartitionFilesystem::GetEntryOffset(int index) const {
    if (index > GetNumEntries())
        return 0;

    return content_offset + pfs_entries[index].fs_entry.offset;
}

u64 PartitionFilesystem::GetEntrySize(int index) const {
    if (index > GetNumEntries())
        return 0;

    return pfs_entries[index].fs_entry.size;
}

std::string PartitionFilesystem::GetEntryName(int index) const {
    if (index > GetNumEntries())
        return "";

    return pfs_entries[index].name;
}

u64 PartitionFilesystem::GetFileOffset(const std::string& name) const {
    for (u32 i = 0; i < pfs_header.num_entries; i++) {
        if (pfs_entries[i].name == name)
            return content_offset + pfs_entries[i].fs_entry.offset;
    }

    return 0;
}

u64 PartitionFilesystem::GetFileSize(const std::string& name) const {
    for (u32 i = 0; i < pfs_header.num_entries; i++) {
        if (pfs_entries[i].name == name)
            return pfs_entries[i].fs_entry.size;
    }

    return 0;
}

void PartitionFilesystem::Print() const {
    LOG_DEBUG(Service_FS, "Magic:                  %.4s", pfs_header.magic.data());
    LOG_DEBUG(Service_FS, "Files:                  %u", pfs_header.num_entries);
    for (u32 i = 0; i < pfs_header.num_entries; i++) {
        LOG_DEBUG(Service_FS, " > File %u:              %s (0x%" PRIX64 " bytes, at 0x%" PRIX64 ")",
                  i, pfs_entries[i].name.c_str(), pfs_entries[i].fs_entry.size,
                  GetFileOffset(pfs_entries[i].name));
    }
}
} // namespace FileSys
