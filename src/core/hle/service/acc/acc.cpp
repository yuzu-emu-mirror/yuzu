// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/acc/acc.h"

namespace Service {
namespace Account {

void InstallInterfaces(SM::ServiceManager& service_manager) {
	std::make_shared<Acc_U0>()->InstallAsService(service_manager);
}

Acc_U0::Acc_U0() : ServiceFramework("acc:u0") {
	static const FunctionInfo functions[] = {
		{100, &Acc_U0::InitializeApplicationInfo, "InitializeApplicationInfo" },
	};
	RegisterHandlers(functions);
}

void Acc_U0::InitializeApplicationInfo(Kernel::HLERequestContext& ctx) {
	LOG_WARNING(Service, "(STUBBED) called");
	IPC::RequestBuilder rb{ ctx, 2 };
	rb.Push(RESULT_SUCCESS);
}


}
}
