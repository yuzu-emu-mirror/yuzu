// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/service.h"

namespace Service {
namespace Account {

void InstallInterfaces(SM::ServiceManager& service_manager);


class Acc_U0 final : public ServiceFramework<Acc_U0> {
public:
	Acc_U0();
	~Acc_U0() = default;
private:
	void InitializeApplicationInfo(Kernel::HLERequestContext& ctx);
};

}
}