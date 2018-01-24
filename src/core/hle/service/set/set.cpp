// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/service/set/set.h"

namespace Service {
namespace Set {

void SET::GetAvailableLanguageCodes(Kernel::HLERequestContext& ctx) {
    constexpr std::array<u8, 13> lang_codes{};

    const auto& output_buffer = ctx.BufferDescriptorC()[0];

    Memory::WriteBlock(output_buffer.Address(), lang_codes.data(), lang_codes.size());

    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(RESULT_SUCCESS);
    rb.Push(static_cast<u64>(lang_codes.size()));

    LOG_WARNING(Service, "(STUBBED) called");
}

SET::SET(const char* name) : ServiceFramework(name) {
    static const FunctionInfo functions[] = {
        {1, &SET::GetAvailableLanguageCodes, "GetAvailableLanguageCodes"},
    };
    RegisterHandlers(functions);
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<SET>("set")->InstallAsService(service_manager);
}

} // namespace Set
} // namespace Service
