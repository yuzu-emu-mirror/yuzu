// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/fatal/fatal.h"

namespace Service {
namespace Fatal {

void InstallInterfaces(SM::ServiceManager& service_manager) {
	std::make_shared<Fatal_U>()->InstallAsService(service_manager);
}

Fatal_U::Fatal_U() : ServiceFramework("fatal:u") {
	static const FunctionInfo functions[] = {
		{ 2, &Fatal_U::TransitionToFatalError, "TransitionToFatalError" },
	};
	RegisterHandlers(functions);
}

void Fatal_U::TransitionToFatalError(Kernel::HLERequestContext& ctx) {
	LOG_WARNING(Service, "(STUBBED) called");
	IPC::RequestBuilder rb{ ctx, 2 };
	rb.Push(RESULT_SUCCESS);
}


}
}