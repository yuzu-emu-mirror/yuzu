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
    IPC::RequestParser rp{ctx};
    u32 id = rp.Pop<u32>();
    constexpr std::array<u8, 13> lang_codes{};

    ctx.WriteBuffer(lang_codes.data(), lang_codes.size());

    IPC::ResponseBuilder rb{ctx, 2};

    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_SET, "(STUBBED) called");
}

SET::SET() : ServiceFramework("set") {
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetLanguageCode"},
        {1, &SET::GetAvailableLanguageCodes, "GetAvailableLanguageCodes"},
        {2, nullptr, "MakeLanguageCode"},
        {3, nullptr, "GetAvailableLanguageCodeCount"},
        {4, nullptr, "GetRegionCode"},
        {5, nullptr, "GetAvailableLanguageCodes2"},
        {6, nullptr, "GetAvailableLanguageCodeCount2"},
        {7, nullptr, "GetKeyCodeMap"},
    };
    RegisterHandlers(functions);
}

} // namespace Set
} // namespace Service
