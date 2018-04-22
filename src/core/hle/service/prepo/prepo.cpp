#include <cinttypes>
#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/service/prepo/prepo.h"

namespace Service::Playreport {
Playreport::Playreport(const char* name) : ServiceFramework(name) {
    static const FunctionInfo functions[] = {
        {10101, &Playreport::SaveReportWithUser, "SaveReportWithUser"},
    };
    RegisterHandlers(functions);
};

void Playreport::SaveReportWithUser(Kernel::HLERequestContext& ctx) {
    /*IPC::RequestParser rp{ctx};
    auto Uid = rp.PopRaw<std::array<u64, 2>>();
    u64 unk = rp.Pop<u64>();
    std::vector<u8> buffer;
    buffer.reserve(ctx.BufferDescriptorX()[0].Size());
    Memory::ReadBlock(ctx.BufferDescriptorX()[0].Address(), buffer.data(), buffer.size());

    std::vector<u8> buffer2;
    buffer.reserve(ctx.BufferDescriptorA()[0].Size());
    Memory::ReadBlock(ctx.BufferDescriptorA()[0].Address(), buffer.data(), buffer.size());*/

    // If we ever want to add play reports

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
};

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<Playreport>("prepo:a")->InstallAsService(service_manager);
    std::make_shared<Playreport>("prepo:m")->InstallAsService(service_manager);
    std::make_shared<Playreport>("prepo:s")->InstallAsService(service_manager);
    std::make_shared<Playreport>("prepo:u")->InstallAsService(service_manager);
}

} // namespace Service::Playreport
