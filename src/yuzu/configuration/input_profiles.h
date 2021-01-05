// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "core/settings.h"

class Config;

class InputProfiles {

public:
    explicit InputProfiles();
    virtual ~InputProfiles();

    std::unordered_map<std::string, Settings::InputProfile> map_profiles;

    std::vector<std::string> GetInputProfileNames();

    bool ProfileExistsInMap(const std::string& profile_name) const;
    static bool IsProfileNameValid(std::string_view profile_name);

    void SaveAllProfiles();

private:
    std::unordered_map<std::string, std::unique_ptr<Config>> map_profiles_old;
};
