// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <string>
#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/service/lm/lm.h"

namespace Service {
namespace LM {

class Logger final : public ServiceFramework<Logger> {
public:
    Logger() : ServiceFramework("Logger") {
        static const FunctionInfo functions[] = {
            {0x00000000, &Logger::Log, "Log"},
        };
        RegisterHandlers(functions);
    }

    ~Logger() = default;

private:
    struct MessageHeader {
        u64_le pid;
        u64_le threadContext;
        union {
            BitField<0, 16, u32_le> flags;
            BitField<16, 8, u32_le> severity;
            BitField<24, 8, u32_le> verbosity;
        };
        u32_le payload_size;
        INSERT_PADDING_WORDS(2);
    };
    static_assert(sizeof(MessageHeader) == 0x20, "MessageHeader is incorrect size");

    /**
     * LM::Initialize service function
     *  Inputs:
     *      0: 0x00000000
     *  Outputs:
     *      0: ResultCode
     */
    void Log(Kernel::HLERequestContext& ctx) {
        MessageHeader header{};
        Memory::ReadBlock(ctx.BufferDescriptorX()[0].Address(), &header, sizeof(MessageHeader));

        std::vector<char> string(header.payload_size);
        Memory::ReadBlock(ctx.BufferDescriptorX()[0].Address() + sizeof(MessageHeader),
                          string.data(), header.payload_size);
        LOG_DEBUG(Debug_Emulated, "%.*s", header.payload_size, string.data());

        IPC::RequestBuilder rb{ctx, 1};
        rb.Push(RESULT_SUCCESS);
    }
};

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<LM>()->InstallAsService(service_manager);
}

/**
 * LM::Initialize service function
 *  Inputs:
 *      0: 0x00000000
 *  Outputs:
 *      0: ResultCode
 */
void LM::Initialize(Kernel::HLERequestContext& ctx) {
    auto client_port = std::make_shared<Logger>()->CreatePort();
    auto session = client_port->Connect();
    if (session.Succeeded()) {
        LOG_DEBUG(Service_SM, "called, initialized logger -> session=%u",
                  (*session)->GetObjectId());
        IPC::RequestBuilder rb{ctx, 1, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushObjects(std::move(session).Unwrap());
        registered_loggers.emplace_back(std::move(client_port));
    } else {
        UNIMPLEMENTED();
    }

    LOG_INFO(Service_SM, "called");
}

LM::LM() : ServiceFramework("lm") {
    static const FunctionInfo functions[] = {
        {0x00000000, &LM::Initialize, "Initialize"},
    };
    RegisterHandlers(functions);
}

LM::~LM() = default;

} // namespace LM
} // namespace Service
