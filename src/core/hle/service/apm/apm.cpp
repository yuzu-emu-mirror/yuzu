// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/service/apm/apm.h"

namespace Service {
namespace APM {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<APM>()->InstallAsService(service_manager);
}

class ISession final : public ServiceFramework<ISession> {
public:
	ISession() : ServiceFramework("ISession") {
		static const FunctionInfo functions[] = {
			{0, &ISession::SetPerformanceConfiguration, "SetPerformanceConfiguration" },
		};
		RegisterHandlers(functions);
	}
private:
	void SetPerformanceConfiguration(Kernel::HLERequestContext& ctx) {
		LOG_WARNING(Service, "(STUBBED) called");
		IPC::RequestBuilder rb{ ctx, 2 };
		rb.Push(RESULT_SUCCESS);
	}
};

void APM::OpenSession(Kernel::HLERequestContext& ctx) {
	auto client_port = std::make_shared<ISession>()->CreatePort();
	auto session = client_port->Connect();
	if (session.Succeeded()) {
		LOG_DEBUG(Service_SM, "called, initialized ISession -> session=%u",
			(*session)->GetObjectId());
		IPC::RequestBuilder rb{ ctx, 2, 0, 1 };
		rb.Push(RESULT_SUCCESS);
		rb.PushMoveObjects(std::move(session).Unwrap());
	}
	else {
		UNIMPLEMENTED();
	}

	LOG_INFO(Service_SM, "called");
}

APM::APM() : ServiceFramework("apm") {
    static const FunctionInfo functions[] = {
        {0x00000000, &APM::OpenSession, "OpenSession"},
        {0x00000001, nullptr, "GetPerformanceMode"},
    };
    RegisterHandlers(functions);
}


} // namespace APM
} // namespace Service
