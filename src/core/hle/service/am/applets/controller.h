// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/am/am.h"
#include "core/hle/service/am/applets/applets.h"

namespace Service::AM::Applets {

struct ControllerConfigPrivate {
    u32 private_arg_size; // unknown, seems to be an offset of some kind
    u32 normal_arg_size;
    u32 flags;
    u32 npad_style_set;
    u32 npad_hold_type;
};
static_assert(sizeof(ControllerConfigPrivate) == 0x14,
              "ControllerConfigPrivate has incorrect size");

struct ControllerConfig {
    u8 minimum_player_count;
    u8 maximum_player_count;
    u8 keep_current_connections;
    u8 left_justify;
    u8 permit_dual_joycons;
    u8 enable_single_mode;
    INSERT_PADDING_BYTES(0x216);
};
static_assert(sizeof(ControllerConfig) == 0x21C, "ControllerConfig has incorrect size");

struct ControllerConfigOutput {
    u8 player_code;
    INSERT_PADDING_BYTES(0x3);
    u32 selected_npad_id;
    u32 result;
};
static_assert(sizeof(ControllerConfigOutput) == 0xC, "ControllerConfigOutput has incorrect size");

class Controller final : public Applet {
public:
    Controller();
    ~Controller() override;

    void Initialize() override;

    bool TransactionComplete() const override;
    ResultCode GetStatus() const override;
    void ExecuteInteractive() override;
    void Execute() override;

    void ConfigurationComplete(bool ok);

private:
    ControllerConfigPrivate private_config;
    ControllerConfig config;
    bool complete = false;
};

} // namespace Service::AM::Applets
