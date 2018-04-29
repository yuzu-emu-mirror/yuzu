// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/service/set/set.h"

namespace Service::Set {

void SET::GetAvailableLanguageCodes(Kernel::HLERequestContext& ctx) {
    static constexpr std::array<std::array<char, 8>, 17> language_codes = {
        {"ja", "en-US", "fr", "de", "it", "es", "zh-CN", "ko", "nl", "pt", "pt-BR" "ru", "zh-TW", "en-GB",
         "fr-CA", "es-419", "zh-Hans", "zh-Hant"}};

    IPC::RequestParser rp{ctx};
    u32 id = rp.Pop<u32>();
    constexpr std::array<u8, 13> lang_codes{};

    ctx.WriteBuffer(language_codes.data(), language_codes.size());

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(RESULT_SUCCESS);
    rb.Push(language_codes.size());

    NGLOG_DEBUG(Service_SET, "called");
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
        {8, nullptr, "GetQuestFlag"},
    };
    RegisterHandlers(functions);
}

} // namespace Service::Set
