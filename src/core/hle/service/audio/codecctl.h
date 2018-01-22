// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Kernel {
class HLERequestContext;
}

namespace Service {
namespace Audio {

class CodecCtl final : public ServiceFramework<CodecCtl> {
public:
    explicit CodecCtl();
    ~CodecCtl() = default;
};

} // namespace Audio
} // namespace Service
