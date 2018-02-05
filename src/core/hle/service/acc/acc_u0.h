// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace Account {

// TODO: RE this structure
struct UserData {
    INSERT_PADDING_BYTES(0x80);
};
static_assert(sizeof(UserData) == 0x80, "UserData structure has incorrect size");

// TODO: RE this structure
struct ProfileBase {
    INSERT_PADDING_BYTES(0x38);
};
static_assert(sizeof(ProfileBase) == 0x38, "ProfileBase structure has incorrect size");

class ACC_U0 final : public ServiceFramework<ACC_U0> {
public:
    ACC_U0();
    ~ACC_U0() = default;

private:
    void GetUserExistence(Kernel::HLERequestContext& ctx);
    void GetLastOpenedUser(Kernel::HLERequestContext& ctx);
    void GetProfile(Kernel::HLERequestContext& ctx);
    void InitializeApplicationInfo(Kernel::HLERequestContext& ctx);
    void GetBaasAccountManagerForApplication(Kernel::HLERequestContext& ctx);
};

} // namespace Account
} // namespace Service
