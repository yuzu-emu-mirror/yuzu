// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/gsp_gpu.h"

namespace Service {
namespace GSP {

/**
 * Signals that the specified interrupt type has occurred to userland code
 * @param interrupt_id ID of interrupt that is being signalled
 * @todo This should probably take a thread_id parameter and only signal this thread?
 * @todo This probably does not belong in the GSP module, instead move to video_core
 */
void SignalInterrupt(InterruptId interrupt_id) {
    UNIMPLEMENTED();
}

GSP_GPU::GSP_GPU() {
}

GSP_GPU::~GSP_GPU() {
}

} // namespace GSP
} // namespace Service
