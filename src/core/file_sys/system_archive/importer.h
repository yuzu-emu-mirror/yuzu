// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/file_sys/vfs_types.h"

namespace FileSys {

class NSP;
class XCI;

namespace SystemArchive {

/// Returns the file corresponding to the title_id if it exists in sysdata.
VirtualFile GetImportedSystemArchive(const VirtualDir& sysdata, u64 title_id);

/// Copies the provided file into sysdata, overwriting current data.
bool ImportSystemArchive(const VirtualDir& sysdata, u64 title_id, const VirtualFile& data);

/// Copies all system archives in the directory to sysdata.
bool ImportDirectorySystemUpdate(const VirtualDir& sysdata, const VirtualDir& dir);

/// Calls ImportDirectorySystemUpdate on the update partition of the XCI.
bool ImportXCISystemUpdate(const VirtualDir& sysdata, XCI& xci);

} // namespace SystemArchive

} // namespace FileSys