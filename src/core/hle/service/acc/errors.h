// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/result.h"

namespace Service::Account {

constexpr ResultCode ERR_ACCOUNTINFO_NULL_ARGUMENT{ErrorModule::Account, 20};
constexpr ResultCode ERR_ACCOUNTINFO_BAD_APPLICATION{ErrorModule::Account, 22};
constexpr ResultCode ERR_ACCOUNTINFO_NULL_INPUT_BUFFER_SIZE{ErrorModule::Account, 30};
constexpr ResultCode ERR_ACCOUNTINFO_INVALID_INPUT_BUFFER_SIZE{ErrorModule::Account, 31};
constexpr ResultCode ERR_ACCOUNTINFO_INVALID_INPUT_BUFFER{ErrorModule::Account, 32};
constexpr ResultCode ERR_ACCOUNTINFO_ALREADY_INITIALIZED{ErrorModule::Account, 41};
constexpr ResultCode ERR_ACCOUNTINFO_INTERNET_REQUEST_ACCEPT_FALSE{ErrorModule::Account, 59};
constexpr ResultCode ERR_ACCOUNTINFO_USER_NOT_FOUND{ErrorModule::Account, 100};
constexpr ResultCode ERR_ACCOUNTINFO_NULL_OBJECT{ErrorModule::Account, 302};

} // namespace Service::Account
