// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::DMNT {

constexpr Result ResultDebuggingDisabled(ErrorModule::DMNT, 2);

constexpr Result ResultCheatNotAttached(ErrorModule::DMNT, 6500);
constexpr Result ResultCheatNullBuffer(ErrorModule::DMNT, 6501);
constexpr Result ResultCheatInvalidBuffer(ErrorModule::DMNT, 6502);
constexpr Result ResultCheatUnknownId(ErrorModule::DMNT, 6503);
constexpr Result ResultCheatOutOfResource(ErrorModule::DMNT, 6504);
constexpr Result ResultCheatInvalid(ErrorModule::DMNT, 6505);
constexpr Result ResultCheatCannotDisable(ErrorModule::DMNT, 6506);
constexpr Result ResultFrozenAddressInvalidWidth(ErrorModule::DMNT, 6600);
constexpr Result ResultFrozenAddressAlreadyExists(ErrorModule::DMNT, 6601);
constexpr Result ResultFrozenAddressNotFound(ErrorModule::DMNT, 6602);
constexpr Result ResultFrozenAddressOutOfResource(ErrorModule::DMNT, 6603);
constexpr Result ResultVirtualMachineInvalidConditionDepth(ErrorModule::DMNT, 6700);

} // namespace Service::DMNT
