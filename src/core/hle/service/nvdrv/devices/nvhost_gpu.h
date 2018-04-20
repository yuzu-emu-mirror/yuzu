// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"

namespace Service::Nvidia::Devices {

class nvmap;
constexpr u32 NVGPU_IOCTL_MAGIC('H');
constexpr u32 NVGPU_IOCTL_CHANNEL_SUBMIT_GPFIFO(0x8);

class nvhost_gpu final : public nvdevice {
public:
    nvhost_gpu(std::shared_ptr<nvmap> nvmap_dev) : nvmap_dev(std::move(nvmap_dev)) {}
    ~nvhost_gpu() override = default;

    u32 ioctl(Ioctl command, const std::vector<u8>& input, std::vector<u8>& output) override;

private:
    enum class IoctlCommand : u32_le {
        IocSetNVMAPfdCommand = 0x40044801,
        IocSetClientDataCommand = 0x40084714,
        IocGetClientDataCommand = 0x80084715,
        IocZCullBind = 0xc010480b,
        IocSetErrorNotifierCommand = 0xC018480C,
        IocChannelSetPriorityCommand = 0x4004480D,
        IocAllocGPFIFOEx2Command = 0xC020481A,
        IocAllocObjCtxCommand = 0xC0104809,
    };

    enum class CtxObjects : u32_le {
        Ctx2D = 0x902D,
        Ctx3D = 0xB197,
        CtxCompute = 0xB1C0,
        CtxKepler = 0xA140,
        CtxDMA = 0xB0B5,
        CtxChannelGPFIFO = 0xB06F,
    };

    struct IoctlSetNvmapFD {
        u32_le nvmap_fd;
    };
    static_assert(sizeof(IoctlSetNvmapFD) == 4, "IoctlSetNvmapFD is incorrect size");

    struct IoctlClientData {
        u64_le data;
    };
    static_assert(sizeof(IoctlClientData) == 8, "IoctlClientData is incorrect size");

    struct IoctlZCullBind {
        u64_le gpu_va;
        u32_le mode; // 0=global, 1=no_ctxsw, 2=separate_buffer, 3=part_of_regular_buf
        INSERT_PADDING_WORDS(1);
    };
    static_assert(sizeof(IoctlZCullBind) == 16, "IoctlZCullBind is incorrect size");

    struct IoctlSetErrorNotifier {
        u64_le offset;
        u64_le size;
        u32_le mem; // nvmap object handle
        INSERT_PADDING_WORDS(1);
    };
    static_assert(sizeof(IoctlSetErrorNotifier) == 24, "IoctlSetErrorNotifier is incorrect size");

    struct IoctlFence {
        u32_le id;
        u32_le value;
    };
    static_assert(sizeof(IoctlFence) == 8, "IoctlFence is incorrect size");

    struct IoctlAllocGpfifoEx2 {
        u32_le num_entries;   // in
        u32_le flags;         // in
        u32_le unk0;          // in (1 works)
        IoctlFence fence_out; // out
        u32_le unk1;          // in
        u32_le unk2;          // in
        u32_le unk3;          // in
    };
    static_assert(sizeof(IoctlAllocGpfifoEx2) == 32, "IoctlAllocGpfifoEx2 is incorrect size");

    struct IoctlAllocObjCtx {
        u32_le class_num; // 0x902D=2d, 0xB197=3d, 0xB1C0=compute, 0xA140=kepler, 0xB0B5=DMA,
                          // 0xB06F=channel_gpfifo
        u32_le flags;
        u64_le obj_id; // (ignored) used for FREE_OBJ_CTX ioctl, which is not supported
    };
    static_assert(sizeof(IoctlAllocObjCtx) == 16, "IoctlAllocObjCtx is incorrect size");

    struct IoctlGpfifoEntry {
        u32_le entry0; // gpu_va_lo
        union {
            u32_le entry1; // gpu_va_hi | (unk_0x02 << 0x08) | (size << 0x0A) | (unk_0x01 << 0x1F)
            BitField<0, 8, u32_le> gpu_va_hi;
            BitField<8, 2, u32_le> unk1;
            BitField<10, 21, u32_le> sz;
            BitField<31, 1, u32_le> unk2;
        };

        VAddr Address() const {
            return (static_cast<VAddr>(gpu_va_hi) << 32) | entry0;
        }
    };
    static_assert(sizeof(IoctlGpfifoEntry) == 8, "IoctlGpfifoEntry is incorrect size");

    struct IoctlSubmitGpfifo {
        u64_le gpfifo;      // (ignored) pointer to gpfifo fence structs
        u32_le num_entries; // number of fence objects being submitted
        u32_le flags;
        IoctlFence fence_out; // returned new fence object for others to wait on
    };
    static_assert(sizeof(IoctlSubmitGpfifo) == 16 + sizeof(IoctlFence),
                  "submit_gpfifo is incorrect size");

    u32_le nvmap_fd{};
    u64_le user_data{};
    IoctlZCullBind zcull_params{};
    u32_le channel_priority{};

    u32 SetNVMAPfd(const std::vector<u8>& input, std::vector<u8>& output);
    u32 SetClientData(const std::vector<u8>& input, std::vector<u8>& output);
    u32 GetClientData(const std::vector<u8>& input, std::vector<u8>& output);
    u32 ZCullBind(const std::vector<u8>& input, std::vector<u8>& output);
    u32 SetErrorNotifier(const std::vector<u8>& input, std::vector<u8>& output);
    u32 SetChannelPriority(const std::vector<u8>& input, std::vector<u8>& output);
    u32 AllocGPFIFOEx2(const std::vector<u8>& input, std::vector<u8>& output);
    u32 AllocateObjectContext(const std::vector<u8>& input, std::vector<u8>& output);
    u32 SubmitGPFIFO(const std::vector<u8>& input, std::vector<u8>& output);

    std::shared_ptr<nvmap> nvmap_dev;
};

} // namespace Service::Nvidia::Devices
