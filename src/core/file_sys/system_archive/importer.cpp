// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <fmt/format.h>
#include "common/file_util.h"
#include "core/file_sys/card_image.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/system_archive/importer.h"
#include "core/file_sys/vfs.h"
#include "core/loader/loader.h"

namespace FileSys::SystemArchive {

VirtualFile GetImportedSystemArchive(const VirtualDir& sysdata, u64 title_id) {
    const auto filename = fmt::format("{:016X}.arc", title_id);
    return sysdata->GetFile(filename);
}

bool ImportSystemArchive(const VirtualDir& sysdata, u64 title_id, const VirtualFile& data) {
    const auto filename = fmt::format("{:016X}.arc", title_id);
    const auto out = sysdata->CreateFile(filename);
    return out != nullptr && VfsRawCopy(data, out);
}

bool ImportDirectorySystemUpdate(const VirtualDir& sysdata, const VirtualDir& dir) {
    Core::Crypto::KeyManager keys;

    for (const auto& file : dir->GetFiles()) {
        NCA nca{file, nullptr, 0, keys};
        if (nca.GetStatus() == Loader::ResultStatus::Success &&
            nca.GetType() == NCAContentType::Data && nca.GetRomFS() != nullptr) {
            if (!ImportSystemArchive(sysdata, nca.GetTitleId(), nca.GetRomFS())) {
                return false;
            }
        }
    }

    return true;
}

bool ImportXCISystemUpdate(const VirtualDir& sysdata, XCI& xci) {
    return ImportDirectorySystemUpdate(sysdata, xci.GetUpdatePartition());
}

} // namespace FileSys::SystemArchive
