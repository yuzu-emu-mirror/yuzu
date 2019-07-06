// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <fstream>
#include <json.hpp>

#include "common/assert.h"
#include "common/common_paths.h"
#include "common/common_types.h"
#include "common/file_util.h"
#include "core/core.h"
#include "core/hle/kernel/process.h"
#include "video_core/texture_cache/resolution_scaling/database.h"

namespace VideoCommon::Resolution {

using namespace nlohmann;

ScalingDatabase::ScalingDatabase(Core::System& system) : database{}, blacklist{}, system{system} {
    title_id = system.CurrentProcess()->GetTitleID();
}

ScalingDatabase::~ScalingDatabase() {
    SaveDatabase();
}

void ScalingDatabase::SaveDatabase() {
    json out;
    out["version"] = DBVersion;
    auto entries = json::array();
    for (const auto& pair : database) {
        const auto [key, value] = pair;
        if (value) {
            entries.push_back({
                {"format", static_cast<u32>(key.format)},
                {"width", key.width},
                {"height", key.height},
            });
        }
    }
    out["entries"] = std::move(entries);
    std::ofstream file(GetProfilePath());
    file << std::setw(4) << out << std::endl;
}

void ScalingDatabase::Register(const PixelFormat format, const u32 width, const u32 height) {
    ResolutionKey key{format, width, height};
    if (blacklist.count(key) == 0) {
        ResolutionKey key{format, width, height};
        database.emplace(key, false);
    }
}

void ScalingDatabase::MarkRendered(const PixelFormat format, const u32 width, const u32 height) {
    ResolutionKey key{format, width, height};
    auto search = database.find(key);
    if (search != database.end()) {
        search->second = true;
    }
}

void ScalingDatabase::Unregister(const PixelFormat format, const u32 width, const u32 height) {
    ResolutionKey key{format, width, height};
    database.erase(key);
    blacklist.insert(key);
}

std::string GetBaseDir() {
    return FileUtil::GetUserPath(FileUtil::UserPath::RescalingDir);
}

std::string ScalingDatabase::GetTitleID() const {
    return fmt::format("{:016X}", title_id);
}

std::string ScalingDatabase::GetProfilePath() const {
    return FileUtil::SanitizePath(GetBaseDir() + DIR_SEP_CHR + GetTitleID() + ".json");
}

} // namespace VideoCommon::Resolution
