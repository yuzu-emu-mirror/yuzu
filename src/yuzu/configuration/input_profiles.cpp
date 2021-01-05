// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <fmt/format.h>

#include "common/common_paths.h"
#include "common/file_util.h"
#include "yuzu/configuration/config.h"
#include "yuzu/configuration/input_profiles.h"

namespace FS = Common::FS;

namespace {

bool ProfileExistsInFilesystem(std::string_view profile_name) {
    return FS::Exists(fmt::format("{}input" DIR_SEP "{}.ini",
                                  FS::GetUserPath(FS::UserPath::ConfigDir), profile_name));
}

bool IsINI(std::string_view filename) {
    const std::size_t index = filename.rfind('.');

    if (index == std::string::npos) {
        return false;
    }

    return filename.substr(index) == ".ini";
}

std::string GetNameWithoutExtension(const std::string& filename) {
    const std::size_t index = filename.rfind('.');

    if (index == std::string::npos) {
        return filename;
    }

    return filename.substr(0, index);
}

} // namespace

InputProfiles::InputProfiles() {
    const std::string input_profile_loc =
        fmt::format("{}input", FS::GetUserPath(FS::UserPath::ConfigDir));

    FS::ForeachDirectoryEntry(
        nullptr, input_profile_loc,
        [this](u64* entries_out, const std::string& directory, const std::string& filename) {
            if (IsINI(filename) && IsProfileNameValid(GetNameWithoutExtension(filename))) {
                map_profiles_old.insert_or_assign(
                    GetNameWithoutExtension(filename),
                    std::make_unique<Config>(GetNameWithoutExtension(filename),
                                             Config::ConfigType::InputProfile));
            }
            return true;
        });

    for (const auto& profile : map_profiles_old) {
        Settings::InputProfile new_profile;
        profile.second->ReadToProfileStruct(new_profile);

        map_profiles.insert_or_assign(profile.first, new_profile);
    }
}

InputProfiles::~InputProfiles() = default;

std::vector<std::string> InputProfiles::GetInputProfileNames() {
    std::vector<std::string> profile_names;
    profile_names.reserve(map_profiles.size());

    for (const auto& profile : map_profiles) {
        profile_names.push_back(profile.first);
    }

    return profile_names;
}

bool InputProfiles::IsProfileNameValid(std::string_view profile_name) {
    return profile_name.find_first_of("<>:;\"/\\|,.!?*") == std::string::npos;
}

void InputProfiles::SaveAllProfiles() {
    for (const auto& old_profile : map_profiles_old) {
        if (!map_profiles.contains(old_profile.first)) {
            FS::Delete(old_profile.second->GetConfigFilePath());
        }
    }

    for (const auto& profile : map_profiles) {
        const auto& config_profile =
            std::make_unique<Config>(profile.first, Config::ConfigType::InputProfile);

        config_profile->WriteFromProfileStruct(profile.second);
    }
}

bool InputProfiles::ProfileExistsInMap(const std::string& profile_name) const {
    return map_profiles.contains(profile_name);
}
