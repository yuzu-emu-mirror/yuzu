// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include "common/common_types.h"

class ARM_Interface;

namespace Kernel {
class Scheduler;
}

namespace Core {

class Cpu {
public:
    Cpu();

    void RunLoop(bool tight_loop = true);

    void SingleStep();

    void PrepareReschedule();

    ARM_Interface& CPU() {
        return *arm_interface;
    }

    Kernel::Scheduler& Scheduler() {
        return *scheduler;
    }

private:
    void Reschedule();

    std::shared_ptr<ARM_Interface> arm_interface;
    std::unique_ptr<Kernel::Scheduler> scheduler;

    bool reschedule_pending{};
};

} // namespace Core
