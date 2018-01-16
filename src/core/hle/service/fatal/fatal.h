// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/service.h"

namespace Service {
namespace Fatal {

void InstallInterfaces(SM::ServiceManager& service_manager);


class Fatal_U final : public ServiceFramework<Fatal_U> {
public:
	Fatal_U();
	~Fatal_U() = default;
private:
	void TransitionToFatalError(Kernel::HLERequestContext& ctx);
};

	}
}