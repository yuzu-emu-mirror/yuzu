// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/microprofile.h"
#include "common/vector_math.h"
#include "core/memory.h"
#include "core/tracer/recorder.h"
#include "video_core/command_processor.h"
#include "video_core/engines/fermi_2d.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/maxwell_compute.h"
#include "video_core/gpu.h"
#include "video_core/renderer_base.h"
#include "video_core/video_core.h"

namespace Tegra {

enum class BufferMethods {
    BindObject = 0,
    CountBufferMethods = 0x100,
};

void GPU::WriteReg(u32 method, u32 subchannel, u32 value) {
    LOG_WARNING(HW_GPU, "Processing method %08X on subchannel %u value %08X", method, subchannel,
                value);

    if (method == static_cast<u32>(BufferMethods::BindObject)) {
        // Bind the current subchannel to the desired engine id.
        LOG_DEBUG(HW_GPU, "Binding subchannel %u to engine %u", subchannel, value);
        ASSERT(bound_engines.find(subchannel) == bound_engines.end());
        bound_engines[subchannel] = static_cast<EngineID>(value);
        return;
    }

    if (method < static_cast<u32>(BufferMethods::CountBufferMethods)) {
        // TODO(Subv): Research and implement these methods.
        LOG_ERROR(HW_GPU, "Special buffer methods other than Bind are not implemented");
        return;
    }

    ASSERT(bound_engines.find(subchannel) != bound_engines.end());

    const EngineID engine = bound_engines[subchannel];

    switch (engine) {
    case EngineID::FERMI_TWOD_A:
        fermi_2d->WriteReg(method, value);
        break;
    case EngineID::MAXWELL_B:
        maxwell_3d->WriteReg(method, value);
        break;
    case EngineID::MAXWELL_COMPUTE_B:
        maxwell_compute->WriteReg(method, value);
        break;
    default:
        UNIMPLEMENTED();
    }
}

void GPU::CallMethod(u32 method, u32 subchannel, const std::vector<u32>& parameters) {
    LOG_WARNING(HW_GPU, "Processing method %08X on subchannel %u num params %zu", method,
                subchannel, parameters.size());

    if (method < static_cast<u32>(BufferMethods::CountBufferMethods)) {
        // TODO(Subv): Research and implement these methods.
        LOG_ERROR(HW_GPU, "Special buffer methods other than Bind are not implemented");
        return;
    }

    ASSERT(bound_engines.find(subchannel) != bound_engines.end());

    const EngineID engine = bound_engines[subchannel];

    switch (engine) {
    case EngineID::FERMI_TWOD_A:
        fermi_2d->CallMethod(method, parameters);
        break;
    case EngineID::MAXWELL_B:
        maxwell_3d->CallMethod(method, parameters);
        break;
    case EngineID::MAXWELL_COMPUTE_B:
        maxwell_compute->CallMethod(method, parameters);
        break;
    default:
        UNIMPLEMENTED();
    }
}

void GPU::ProcessCommandList(GPUVAddr address, u32 size) {
    // TODO(Subv): PhysicalToVirtualAddress is a misnomer, it converts a GPU VAddr into an
    // application VAddr.
    const VAddr head_address = memory_manager->PhysicalToVirtualAddress(address);
    VAddr current_addr = head_address;
    while (current_addr < head_address + size * sizeof(CommandHeader)) {
        const CommandHeader header = {Memory::Read32(current_addr)};
        current_addr += sizeof(u32);

        switch (header.mode.Value()) {
        case SubmissionMode::IncreasingOld:
        case SubmissionMode::Increasing: {
            // Increase the method value with each argument.
            for (unsigned i = 0; i < header.arg_count; ++i) {
                WriteReg(header.method + i, header.subchannel, Memory::Read32(current_addr));
                current_addr += sizeof(u32);
            }
            break;
        }
        case SubmissionMode::NonIncreasingOld:
        case SubmissionMode::NonIncreasing: {
            // Use the same method value for all arguments.
            for (unsigned i = 0; i < header.arg_count; ++i) {
                WriteReg(header.method, header.subchannel, Memory::Read32(current_addr));
                current_addr += sizeof(u32);
            }
            break;
        }
        case SubmissionMode::IncreaseOnce: {
            ASSERT(header.arg_count.Value() >= 1);
            // Use the original method for the first argument and then the next method for all other
            // arguments.

            // Process this command as a method call instead of a register write. Gather
            // all the parameters first and then pass them at once to the CallMethod function.
            std::vector<u32> parameters(header.arg_count);

            for (unsigned i = 0; i < header.arg_count; ++i) {
                parameters[i] = Memory::Read32(current_addr);
                current_addr += sizeof(u32);
            }

            CallMethod(header.method, header.subchannel, parameters);
            break;
        }
        case SubmissionMode::Inline: {
            // The register value is stored in the bits 16-28 as an immediate
            WriteReg(header.method, header.subchannel, header.inline_data);
            break;
        }
        default:
            UNIMPLEMENTED();
        }
    }
}

} // namespace Tegra
