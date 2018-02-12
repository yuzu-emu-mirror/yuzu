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
#include "video_core/renderer_base.h"
#include "video_core/video_core.h"

namespace Tegra {

namespace CommandProcessor {

enum class BufferMethods {
    BindObject = 0,
    CountBufferMethods = 0x100,
};

enum class EngineID {
    FERMI_TWOD_A = 0x902D, // 2D Engine
    MAXWELL_B = 0xB197,    // 3D Engine
    MAXWELL_COMPUTE_B = 0xB1C0,
    KEPLER_INLINE_TO_MEMORY_B = 0xA140,
    MAXWELL_DMA_COPY_A = 0xB0B5,
};

// Mapping of subchannels to their bound engine ids.
static std::unordered_map<u32, EngineID> bound_engines;

static void WriteReg(u32 method, u32 subchannel, u32 value) {
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
        Engines::Fermi2D::WriteReg(method, value);
        break;
    case EngineID::MAXWELL_B:
        Engines::Maxwell3D::WriteReg(method, value);
        break;
    case EngineID::MAXWELL_COMPUTE_B:
        Engines::MaxwellCompute::WriteReg(method, value);
        break;
    default:
        UNIMPLEMENTED();
    }
}

void ProcessCommandList(VAddr address, u32 size) {
    VAddr current_addr = address;
    while (current_addr < address + size * sizeof(CommandHeader)) {
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
            WriteReg(header.method, header.subchannel, Memory::Read32(current_addr));
            current_addr += sizeof(u32);
            // Use the same method value for all arguments.
            for (unsigned i = 1; i < header.arg_count; ++i) {
                WriteReg(header.method + 1, header.subchannel, Memory::Read32(current_addr));
                current_addr += sizeof(u32);
            }
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

} // namespace CommandProcessor

} // namespace Tegra
