// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/file_util.h"
#include "common/logging/log.h"
#include "core/file_sys/program_metadata.h"
#include "core/loader/loader.h"

namespace FileSys {

Loader::ResultStatus ProgramMetadata::Load(const std::string& file_path) {
    FileUtil::IOFile file(file_path, "rb");
    if (!file.IsOpen())
        return Loader::ResultStatus::Error;

    std::vector<u8> file_data(file.GetSize());

    if (!file.ReadBytes(file_data.data(), file_data.size()))
        return Loader::ResultStatus::Error;

    Loader::ResultStatus result = Load(file_data);
    if (result != Loader::ResultStatus::Success)
        LOG_ERROR(Service_FS, "Failed to load NPDM from file {}!", file_path);

    return result;
}

Loader::ResultStatus ProgramMetadata::Load(const std::vector<u8> file_data, size_t offset) {
    size_t total_size = static_cast<size_t>(file_data.size() - offset);
    if (total_size < sizeof(Header))
        return Loader::ResultStatus::Error;

    size_t header_offset = offset;
    memcpy(&npdm_header, &file_data[offset], sizeof(Header));

    size_t aci_offset = header_offset + npdm_header.aci_offset;
    size_t acid_offset = header_offset + npdm_header.acid_offset;
    memcpy(&aci_header, &file_data[aci_offset], sizeof(AciHeader));
    memcpy(&acid_header, &file_data[acid_offset], sizeof(AcidHeader));

    size_t fac_offset = acid_offset + acid_header.fac_offset;
    size_t fah_offset = aci_offset + aci_header.fah_offset;
    memcpy(&acid_file_access, &file_data[fac_offset], sizeof(FileAccessControl));
    memcpy(&aci_file_access, &file_data[fah_offset], sizeof(FileAccessHeader));

    return Loader::ResultStatus::Success;
}

bool ProgramMetadata::Is64BitProgram() const {
    return npdm_header.has_64_bit_instructions;
}

ProgramAddressSpaceType ProgramMetadata::GetAddressSpaceType() const {
    return npdm_header.address_space_type;
}

u8 ProgramMetadata::GetMainThreadPriority() const {
    return npdm_header.main_thread_priority;
}

u8 ProgramMetadata::GetMainThreadCore() const {
    return npdm_header.main_thread_cpu;
}

u32 ProgramMetadata::GetMainThreadStackSize() const {
    return npdm_header.main_stack_size;
}

u64 ProgramMetadata::GetTitleID() const {
    return aci_header.title_id;
}

u64 ProgramMetadata::GetFilesystemPermissions() const {
    return aci_file_access.permissions;
}

void ProgramMetadata::Print() const {
    LOG_DEBUG(Service_FS, "Magic:                  {:.4}", npdm_header.magic.data());
    LOG_DEBUG(Service_FS, "Main thread priority:   0x{:02X}", npdm_header.main_thread_priority);
    LOG_DEBUG(Service_FS, "Main thread core:       {}", npdm_header.main_thread_cpu);
    LOG_DEBUG(Service_FS, "Main thread stack size: 0x{:X} bytes", npdm_header.main_stack_size);
    LOG_DEBUG(Service_FS, "Process category:       {}", npdm_header.process_category);
    LOG_DEBUG(Service_FS, "Flags:                  0x{:02X}", npdm_header.flags);
    LOG_DEBUG(Service_FS, " > 64-bit instructions: {}",
              npdm_header.has_64_bit_instructions ? "YES" : "NO");

    auto address_space = "Unknown";
    switch (npdm_header.address_space_type) {
    case ProgramAddressSpaceType::Is64Bit:
        address_space = "64-bit";
        break;
    case ProgramAddressSpaceType::Is32Bit:
        address_space = "32-bit";
        break;
    }

    LOG_DEBUG(Service_FS, " > Address space:       {}\n", address_space);

    // Begin ACID printing (potential perms, signed)
    LOG_DEBUG(Service_FS, "Magic:                  {:.4}", acid_header.magic.data());
    LOG_DEBUG(Service_FS, "Flags:                  0x{:02X}", acid_header.flags);
    LOG_DEBUG(Service_FS, " > Is Retail:           {}", acid_header.is_retail ? "YES" : "NO");
    LOG_DEBUG(Service_FS, "Title ID Min:           0x{:016X}", acid_header.title_id_min);
    LOG_DEBUG(Service_FS, "Title ID Max:           0x{:016X}", acid_header.title_id_max);
    LOG_DEBUG(Service_FS, "Filesystem Access:      0x{:016X}\n", acid_file_access.permissions);

    // Begin ACI0 printing (actual perms, unsigned)
    LOG_DEBUG(Service_FS, "Magic:                  {:.4}", aci_header.magic.data());
    LOG_DEBUG(Service_FS, "Title ID:               0x{:016X}", aci_header.title_id);
    LOG_DEBUG(Service_FS, "Filesystem Access:      0x{:016X}\n", aci_file_access.permissions);
}
} // namespace FileSys
