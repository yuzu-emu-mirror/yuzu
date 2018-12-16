// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "core/core_timing.h"
#include "core/memory.h"
#include "video_core/engines/fermi_2d.h"
#include "video_core/engines/kepler_memory.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/maxwell_compute.h"
#include "video_core/engines/maxwell_dma.h"
#include "video_core/gpu.h"
#include "video_core/rasterizer_interface.h"

namespace Tegra {

u32 FramebufferConfig::BytesPerPixel(PixelFormat format) {
    switch (format) {
    case PixelFormat::ABGR8:
        return 4;
    default:
        return 4;
    }

    UNREACHABLE();
}

GPU::GPU(VideoCore::RasterizerInterface& rasterizer) {
    memory_manager = std::make_unique<Tegra::MemoryManager>();
    dma_pusher = std::make_unique<Tegra::DmaPusher>(*this);
    maxwell_3d = std::make_unique<Engines::Maxwell3D>(rasterizer, *memory_manager);
    fermi_2d = std::make_unique<Engines::Fermi2D>(rasterizer, *memory_manager);
    maxwell_compute = std::make_unique<Engines::MaxwellCompute>();
    maxwell_dma = std::make_unique<Engines::MaxwellDMA>(rasterizer, *memory_manager);
    kepler_memory = std::make_unique<Engines::KeplerMemory>(rasterizer, *memory_manager);
}

GPU::~GPU() = default;

Engines::Maxwell3D& GPU::Maxwell3D() {
    return *maxwell_3d;
}

const Engines::Maxwell3D& GPU::Maxwell3D() const {
    return *maxwell_3d;
}

MemoryManager& GPU::MemoryManager() {
    return *memory_manager;
}

const MemoryManager& GPU::MemoryManager() const {
    return *memory_manager;
}

DmaPusher& GPU::DmaPusher() {
    return *dma_pusher;
}

const DmaPusher& GPU::DmaPusher() const {
    return *dma_pusher;
}

u32 RenderTargetBytesPerPixel(RenderTargetFormat format) {
    ASSERT(format != RenderTargetFormat::NONE);

    switch (format) {
    case RenderTargetFormat::RGBA32_FLOAT:
    case RenderTargetFormat::RGBA32_UINT:
        return 16;
    case RenderTargetFormat::RGBA16_UINT:
    case RenderTargetFormat::RGBA16_UNORM:
    case RenderTargetFormat::RGBA16_FLOAT:
    case RenderTargetFormat::RG32_FLOAT:
    case RenderTargetFormat::RG32_UINT:
        return 8;
    case RenderTargetFormat::RGBA8_UNORM:
    case RenderTargetFormat::RGBA8_SNORM:
    case RenderTargetFormat::RGBA8_SRGB:
    case RenderTargetFormat::RGBA8_UINT:
    case RenderTargetFormat::RGB10_A2_UNORM:
    case RenderTargetFormat::BGRA8_UNORM:
    case RenderTargetFormat::BGRA8_SRGB:
    case RenderTargetFormat::RG16_UNORM:
    case RenderTargetFormat::RG16_SNORM:
    case RenderTargetFormat::RG16_UINT:
    case RenderTargetFormat::RG16_SINT:
    case RenderTargetFormat::RG16_FLOAT:
    case RenderTargetFormat::R32_FLOAT:
    case RenderTargetFormat::R11G11B10_FLOAT:
    case RenderTargetFormat::R32_UINT:
        return 4;
    case RenderTargetFormat::R16_UNORM:
    case RenderTargetFormat::R16_SNORM:
    case RenderTargetFormat::R16_UINT:
    case RenderTargetFormat::R16_SINT:
    case RenderTargetFormat::R16_FLOAT:
    case RenderTargetFormat::RG8_UNORM:
    case RenderTargetFormat::RG8_SNORM:
        return 2;
    case RenderTargetFormat::R8_UNORM:
    case RenderTargetFormat::R8_UINT:
        return 1;
    default:
        UNIMPLEMENTED_MSG("Unimplemented render target format {}", static_cast<u32>(format));
    }
}

u32 DepthFormatBytesPerPixel(DepthFormat format) {
    switch (format) {
    case DepthFormat::Z32_S8_X24_FLOAT:
        return 8;
    case DepthFormat::Z32_FLOAT:
    case DepthFormat::S8_Z24_UNORM:
    case DepthFormat::Z24_X8_UNORM:
    case DepthFormat::Z24_S8_UNORM:
    case DepthFormat::Z24_C8_UNORM:
        return 4;
    case DepthFormat::Z16_UNORM:
        return 2;
    default:
        UNIMPLEMENTED_MSG("Unimplemented Depth format {}", static_cast<u32>(format));
    }
}

enum class BufferMethods {
    BindObject = 0x0,
    Nop = 0x8,
    SemaphoreAddressHigh = 0x10,
    SemaphoreAddressLow = 0x14,
    SemaphoreSequence = 0x18,
    SemaphoreTrigger = 0x1C,
    NotifyIntr = 0x20,
    WrcacheFlush = 0x24,
    Unk28 = 0x28,
    Unk2c = 0x2C,
    RefCnt = 0x50,
    SemaphoreAcquire = 0x68,
    SemaphoreRelease = 0x6C,
    Unk70 = 0x70,
    Unk74 = 0x74,
    UNK78 = 0x78,
    Unk7c = 0x7C,
    Yield = 0x80,
    NonPullerMethods = 0x100,
};

enum class GpuSemaphoreOperation {
    ACQUIRE_EQUAL = 0x1,
    WRITE_LONG = 0x2,
    ACQUIRE_GEQUAL = 0x4,
    ACQUIRE_MASK = 0x8,
};

void GPU::CallMethod(const MethodCall& method_call) {
    LOG_TRACE(HW_GPU, "Processing method {:08X} on subchannel {}", method_call.method,
              method_call.subchannel);

    ASSERT(method_call.subchannel < bound_engines.size());

    // Note that, traditionally, methods are treated as 4-byte addressable locations, and hence
    // their numbers are written down multiplied by 4 in Docs. Hence why we multiply by 4 here.
    const auto method = static_cast<BufferMethods>(method_call.method * 4);
    if (method < BufferMethods::NonPullerMethods) {
        switch (method) {
        case BufferMethods::BindObject: {
            // Bind the current subchannel to the desired engine id.
            LOG_DEBUG(HW_GPU, "Binding subchannel {} to engine {}", method_call.subchannel,method_call.argument);
            bound_engines[method_call.subchannel] = static_cast<EngineID>(method_call.argument);
            break;
        }
        case BufferMethods::Nop:
            break;
        case BufferMethods::SemaphoreAddressHigh: {
            if (method_call.argument & 0xffffff00) {
                LOG_ERROR(HW_GPU, "SemaphoreAddressHigh too large");
                return;
            }
            semaphore_addr.high.Assign(method_call.argument);
            break;
        }
        case BufferMethods::SemaphoreAddressLow: {
            if (method_call.argument & 3) {
                LOG_ERROR(HW_GPU, "SemaphoreAddressLow unaligned");
                return;
            }
            semaphore_addr.low.Assign(method_call.argument);
            break;
        }
        case BufferMethods::SemaphoreSequence: {
            semaphore_sequence = method_call.argument;
            break;
        }
        case BufferMethods::SemaphoreTrigger: {
            const auto op = static_cast<GpuSemaphoreOperation>(method_call.argument & 7);
            // TODO(Kmather73): Generate a real GPU timestamp and write it here instead of CoreTiming
            const auto acquire_timestamp = CoreTiming::GetTicks();
            if (method_call.argument == 2) {
                 Memory::Write32(semaphore_addr.addr, method_call.argument);
                 Memory::Write32(semaphore_addr.addr + 0x4, 0);
                 Memory::Write64(semaphore_addr.addr + 0x8, acquire_timestamp);
             } else {
                 const u32 word = Memory::Read32(semaphore_addr.addr);
                 if ((op == GpuSemaphoreOperation::ACQUIRE_EQUAL && word == semaphore_sequence) ||
                     (op == GpuSemaphoreOperation::ACQUIRE_GEQUAL && static_cast<s32>(word - semaphore_sequence) > 0) ||
                     (op == GpuSemaphoreOperation::ACQUIRE_MASK && (word & semaphore_sequence))) {
                     // Nothing to do in this case
                 } else {
                     acquire_source = true;
                     acquire_value = semaphore_sequence;
                     if (op == GpuSemaphoreOperation::ACQUIRE_EQUAL) {
                         acquire_active = true;
                         acquire_mode = false;
                     } else if (op == GpuSemaphoreOperation::ACQUIRE_GEQUAL) {
                         acquire_active = true;
                         acquire_mode = true;
                     } else {
                         LOG_ERROR(HW_GPU, "Invalid semaphore operation");
                     }
                 }
             }
             break;
         }
        case BufferMethods::NotifyIntr: {
            // TODO(Kmather73): Research and implement this method.
            LOG_ERROR(HW_GPU, "Special puller engine method NotifyIntr not implemented");
            break;
        }
        case BufferMethods::WrcacheFlush: {
            // TODO(Kmather73): Research and implement this method.
            LOG_ERROR(HW_GPU, "Special puller engine method WrcacheFlush not implemented");
            break;
        }
        case BufferMethods::Unk28: {
            // TODO(Kmather73): Research and implement this method.
            LOG_ERROR(HW_GPU, "Special puller engine method Unk28 not implemented");
            break;
        }
        case BufferMethods::Unk2c: {
            // TODO(Kmather73): Research and implement this method.
            LOG_ERROR(HW_GPU, "Special puller engine method Unk2c not implemented");
            break;
        }
        case BufferMethods::SemaphoreAcquire: {
            if (!semaphore_off_val) {
                LOG_ERROR(HW_GPU, "Semaphore has already be acquired");
                return;
            }
            const u32 word = Memory::Read32(semaphore_addr.addr);
            if (word != method_call.argument) {
                acquire_active = true;
                acquire_value = method_call.argument;
                acquire_mode = false;
                acquire_source = false;
            }
            break;
        }
        case BufferMethods::SemaphoreRelease: {
            if (!semaphore_off_val) {
                LOG_ERROR(HW_GPU, "Semaphore can't be released since it is not currently been acquired");
                return;
            }
            Memory::Write32(semaphore_addr.addr, method_call.method);
            break;
        }
        case BufferMethods::Yield: {
            // TODO(Kmather73): Research and implement this method.
            LOG_ERROR(HW_GPU, "Special puller engine method Yield not implemented");
            break;
        }
        default:
            LOG_ERROR(HW_GPU, "Special puller engine method {:X} not implemented", static_cast<u32>(method));
            break;
        }
        return;
    }

    const EngineID engine = bound_engines[method_call.subchannel];

    switch (engine) {
    case EngineID::FERMI_TWOD_A:
        fermi_2d->CallMethod(method_call);
        break;
    case EngineID::MAXWELL_B:
        maxwell_3d->CallMethod(method_call);
        break;
    case EngineID::MAXWELL_COMPUTE_B:
        maxwell_compute->CallMethod(method_call);
        break;
    case EngineID::MAXWELL_DMA_COPY_A:
        maxwell_dma->CallMethod(method_call);
        break;
    case EngineID::KEPLER_INLINE_TO_MEMORY_B:
        kepler_memory->CallMethod(method_call);
        break;
    default:
        UNIMPLEMENTED_MSG("Unimplemented engine");
    }
}

} // namespace Tegra
