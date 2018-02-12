// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <utility>
#include <vector>
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"
#include "core/hle/service/nvdrv/memory_manager.h"

namespace Service {
namespace Nvidia {
namespace Devices {

class nvmap;

class nvhost_as_gpu final : public nvdevice {
public:
    nvhost_as_gpu(std::shared_ptr<nvmap> nvmap_dev, std::shared_ptr<MemoryManager> memory_manager)
        : nvmap_dev(std::move(nvmap_dev)), memory_manager(std::move(memory_manager)) {}
    ~nvhost_as_gpu() override = default;

    u32 ioctl(Ioctl command, const std::vector<u8>& input, std::vector<u8>& output) override;

private:
    enum class IoctlCommand : u32_le {
        IocInitalizeExCommand = 0x40284109,
        IocAllocateSpaceCommand = 0xC0184102,
        IocMapBufferExCommand = 0xC0284106,
        IocBindChannelCommand = 0x40044101,
        IocGetVaRegionsCommand = 0xC0404108,
    };

    struct IoctlInitalizeEx {
        u32_le big_page_size; // depends on GPU's available_big_page_sizes; 0=default
        s32_le as_fd;         // ignored; passes 0
        u32_le flags;         // passes 0
        u32_le reserved;      // ignored; passes 0
        u64_le unk0;
        u64_le unk1;
        u64_le unk2;
    };
    static_assert(sizeof(IoctlInitalizeEx) == 40, "IoctlInitalizeEx is incorrect size");

    struct IoctlAllocSpace {
        u32_le pages;
        u32_le page_size;
        u32_le flags;
        INSERT_PADDING_WORDS(1);
        union {
            u64_le offset;
            u64_le align;
        };
    };
    static_assert(sizeof(IoctlAllocSpace) == 24, "IoctlInitalizeEx is incorrect size");

    struct IoctlMapBufferEx {
        u32_le flags; // bit0: fixed_offset, bit2: cacheable
        u32_le kind;  // -1 is default
        u32_le nvmap_handle;
        u32_le page_size; // 0 means don't care
        u64_le buffer_offset;
        u64_le mapping_size;
        u64_le offset;
    };
    static_assert(sizeof(IoctlMapBufferEx) == 40, "IoctlMapBufferEx is incorrect size");

    struct IoctlBindChannel {
        u32_le fd;
    };
    static_assert(sizeof(IoctlBindChannel) == 4, "IoctlBindChannel is incorrect size");

    struct IoctlVaRegion {
        u64_le offset;
        u32_le page_size;
        INSERT_PADDING_WORDS(1);
        u64_le pages;
    };
    static_assert(sizeof(IoctlVaRegion) == 24, "IoctlVaRegion is incorrect size");

    struct IoctlGetVaRegions {
        u64_le buf_addr; // (contained output user ptr on linux, ignored)
        u32_le buf_size; // forced to 2*sizeof(struct va_region)
        u32_le reserved;
        IoctlVaRegion regions[2];
    };
    static_assert(sizeof(IoctlGetVaRegions) == 16 + sizeof(IoctlVaRegion) * 2,
                  "IoctlGetVaRegions is incorrect size");

    u32 channel{};

    u32 InitalizeEx(const std::vector<u8>& input, std::vector<u8>& output);
    u32 AllocateSpace(const std::vector<u8>& input, std::vector<u8>& output);
    u32 MapBufferEx(const std::vector<u8>& input, std::vector<u8>& output);
    u32 BindChannel(const std::vector<u8>& input, std::vector<u8>& output);
    u32 GetVARegions(const std::vector<u8>& input, std::vector<u8>& output);

    std::shared_ptr<nvmap> nvmap_dev;
    std::shared_ptr<MemoryManager> memory_manager;
};

} // namespace Devices
} // namespace Nvidia
} // namespace Service
