// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/api_version.h"
#include "core/file_sys/system_archive/system_version.h"
#include "core/file_sys/vfs_vector.h"

namespace FileSys::SystemArchive {

std::string GetLongDisplayVersion() {
    return Core::ApiVersion::DISPLAY_TITLE;
}

VirtualDir SystemVersion() {
    LOG_WARNING(Common_Filesystem, "called - Using hardcoded firmware version '{}'",
                GetLongDisplayVersion());

    VirtualFile file = std::make_shared<VectorVfsFile>(std::vector<u8>(0x100), "file");
    file->WriteObject(Core::ApiVersion::HOS_VERSION_MAJOR, 0);
    file->WriteObject(Core::ApiVersion::HOS_VERSION_MINOR, 1);
    file->WriteObject(Core::ApiVersion::HOS_VERSION_MICRO, 2);
    file->WriteObject(Core::ApiVersion::SDK_REVISION_MAJOR, 4);
    file->WriteObject(Core::ApiVersion::SDK_REVISION_MINOR, 5);
    file->WriteArray(Core::ApiVersion::PLATFORM_STRING,
                     std::min<u64>(sizeof(Core::ApiVersion::PLATFORM_STRING), 0x20ULL), 0x8);
    file->WriteArray(Core::ApiVersion::VERSION_HASH,
                     std::min<u64>(sizeof(Core::ApiVersion::VERSION_HASH), 0x40ULL), 0x28);
    file->WriteArray(Core::ApiVersion::DISPLAY_VERSION,
                     std::min<u64>(sizeof(Core::ApiVersion::DISPLAY_VERSION), 0x18ULL), 0x68);
    file->WriteArray(Core::ApiVersion::DISPLAY_TITLE,
                     std::min<u64>(sizeof(Core::ApiVersion::DISPLAY_TITLE), 0x80ULL), 0x80);
    return std::make_shared<VectorVfsDirectory>(std::vector<VirtualFile>{file},
                                                std::vector<VirtualDir>{}, "data");
}

} // namespace FileSys::SystemArchive
