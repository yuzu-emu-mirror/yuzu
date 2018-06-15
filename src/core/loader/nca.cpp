// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>

#include "common/common_funcs.h"
#include "common/file_util.h"
#include "common/logging/log.h"
#include "common/swap.h"
#include "core/core.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/loader/nro.h"
#include "core/memory.h"
#include "nca.h"

namespace Loader {

AppLoader_NCA::AppLoader_NCA(FileUtil::IOFile&& file, std::string filepath)
    : AppLoader(std::move(file)), filepath(std::move(filepath)) {}

FileType AppLoader_NCA::IdentifyType(FileUtil::IOFile& file, const std::string&) {
    // Check for NCA header here -- that part might be enc. too :(
    return FileType::Error;
}

ResultStatus AppLoader_NCA::Load(Kernel::SharedPtr<Kernel::Process>& process) {
    NGLOG_CRITICAL(Loader, "UNIMPLEMENTED!");
    return ResultStatus::Error;
}

} // namespace Loader
