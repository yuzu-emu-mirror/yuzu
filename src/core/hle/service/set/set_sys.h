// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service::Set {

class SET_SYS final : public ServiceFramework<SET_SYS> {
public:
    explicit SET_SYS();
    ~SET_SYS() override;

private:
    /// Indicates the current theme set by the system settings
    enum class ColorSet : u32 {
        BasicWhite = 0,
        BasicBlack = 1,
    };

    void GetColorSetId(Kernel::HLERequestContext& ctx);
    void SetColorSetId(Kernel::HLERequestContext& ctx);
    void GetAccountSettings(Kernel::HLERequestContext& ctx);
    void SetAccountSettings(Kernel::HLERequestContext& ctx);
    void GetDataDeletionSettings(Kernel::HLERequestContext& ctx);
    void SetDataDeletionSettings(Kernel::HLERequestContext& ctx);
    void GetLdnChannel(Kernel::HLERequestContext& ctx);
    void SetLdnChannel(Kernel::HLERequestContext& ctx);

    ColorSet color_set = ColorSet::BasicWhite;
    u32 account_settings = 0;
    u64 data_deletion_settings = 0;
    s32 ldn_channel = 0;
};

} // namespace Service::Set
