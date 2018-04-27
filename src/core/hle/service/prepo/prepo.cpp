#include <cinttypes>
#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/service/prepo/prepo.h"

namespace Service::PlayReport {
PlayReport::PlayReport(const char* name) : ServiceFramework(name) {
    static const FunctionInfo functions[] = {
        {10100, nullptr, "SaveReport"},
        {10101, &PlayReport::SaveReportWithUser, "SaveReportWithUser"},
        {10200, nullptr, "RequestImmediateTransmission"},
        {10300, nullptr, "GetTransmissionStatus"},
        {20100, nullptr, "SaveSystemReport"},
        {20200, nullptr, "SetOperationMode"},
        {20101, nullptr, "SaveSystemReportWithUser"},
        {30100, nullptr, "ClearStorage"},
        {40100, nullptr, "IsUserAgreementCheckEnabled"},
        {40101, nullptr, "SetUserAgreementCheckEnabled"},
        {90100, nullptr, "GetStorageUsage"},
        {90200, nullptr, "GetStatistics"},
        {90201, nullptr, "GetThroughputHistory"},
        {90300, nullptr, "GetLastUploadError"},
    };
    RegisterHandlers(functions);
};

void PlayReport::SaveReportWithUser(Kernel::HLERequestContext& ctx) {
    // TODO(ogniK): Do we want to add play report?
    NGLOG_WARNING(Service_PREPO, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
};

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<PlayReport>("prepo:a")->InstallAsService(service_manager);
    std::make_shared<PlayReport>("prepo:m")->InstallAsService(service_manager);
    std::make_shared<PlayReport>("prepo:s")->InstallAsService(service_manager);
    std::make_shared<PlayReport>("prepo:u")->InstallAsService(service_manager);
}

} // namespace Service::PlayReport
