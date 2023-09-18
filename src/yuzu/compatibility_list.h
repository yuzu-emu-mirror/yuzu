// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <tsl/robin_map.h>
#include <QString>

#include "common/common_types.h"

using CompatibilityList = tsl::robin_map<std::string, std::pair<QString, QString>>;

CompatibilityList::const_iterator FindMatchingCompatibilityEntry(
    const CompatibilityList& compatibility_list, u64 program_id);
