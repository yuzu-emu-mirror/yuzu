// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include <unordered_map>
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"

namespace Service::Nvidia::Devices {

class nvhost_nvdec final : public nvdevice {
public:
    explicit nvhost_nvdec(Core::System& system);
    ~nvhost_nvdec() override;

    u32 ioctl(Ioctl command, const std::vector<u8>& input, const std::vector<u8>& input2,
              std::vector<u8>& output, std::vector<u8>& output2, IoctlCtrl& ctrl,
              IoctlVersion version) override;

private:
    enum class IoctlCommand : u32_le {
        IocSetNVMAPfdCommand = 0x40044801,
        IocChannelGetSyncPoint = 0xC0080002,
        IocChannelGetWaitBase = 0xC0080003,
    };

    struct IoctlSetNvmapFD {
        u32_le nvmap_fd;
    };
    static_assert(sizeof(IoctlSetNvmapFD) == 4, "IoctlSetNvmapFD is incorrect size");

    struct IoctlCmdBuf {
        u32_le mem;
        u32_le offset;
        u32_le words;
    };
    static_assert(sizeof(IoctlCmdBuf) == 12, "IoctlCmdBuf is incorrect size");

    struct IoctlReloc {
        u32_le cmdbuf_mem;
        u32_le cmdbuf_offset;
        u32_le target;
        u32_le target_offset;
    };
    static_assert(sizeof(IoctlReloc) == 16, "IoctlReloc is incorrect size");

    struct IoctlRelocShift {
        u32_le shift;
    };
    static_assert(sizeof(IoctlRelocShift) == 4, "IoctlRelocShift is incorrect size");

    struct IoctlSyncPtIncr {
        u32_le syncpt_id;
        u32_le syncpt_incrs;
    };
    static_assert(sizeof(IoctlSyncPtIncr) == 8, "IoctlSyncPtIncr is incorrect size");

    struct IoctlFence {
        u32_le id;
        u32_le thresh;
    };
    static_assert(sizeof(IoctlFence) == 8, "IoctlFence is incorrect size");

    struct IoctlSubmit {
        // [in]
        u32_le num_cmdbufs;
        // [in]
        u32_le num_relocs;
        // [in]
        u32_le num_syncpt_incrs;
        // [in]
        u32_le num_fences;
        // __in    struct cmdbuf cmdbufs[];
        // __in    struct reloc relocs[];
        // __in    struct reloc_shift reloc_shifts[];
        // __in    struct syncpt_incr syncpt_incrs[];
        // __out   struct fence fences[];
    };
    static_assert(sizeof(IoctlSubmit) == 16, "IoctlSubmit is incorrect size");

    struct IoctlHandleMapBuffer {
        u32_le handle_id_in;  // nvmap handle to map
        u32_le phys_addr_out; // returned device physical address mapped to the handle
    };
    static_assert(sizeof(IoctlHandleMapBuffer) == 8, "IoctlHandleMapBuffer is incorrect size");

    struct IoctlMapCmdBuffer {
        // [in] number of nvmap handles to map
        u32_le num_handles;
        // [in] ignored
        u32_le reserved;
        // [in] memory to map is compressed
        u8 is_compressed;
        // [in] padding[3] u8 ignored
        INSERT_PADDING_BYTES(0x3);
    };
    static_assert(sizeof(IoctlMapCmdBuffer) == 12, "IoctlMapCmdBuffer is incorrect size");

    struct IoctChannelWaitBase {
        // [in]
        u32_le module_id;
        // [out] Returns the current waitbase value for a given module. Always returns 0
        u32_le waitbase_value;
    };
    static_assert(sizeof(IoctChannelWaitBase) == 8, "IoctChannelWaitBase is incorrect size");

    struct IoctChannelSyncPoint {
        // [in]
        u32_le syncpt_id;
        // [out] Returns the current syncpoint value for a given module. Identical to Linux driver.
        u32_le syncpt_value;
    };
    static_assert(sizeof(IoctChannelSyncPoint) == 8, "IoctChannelSyncPoint is incorrect size");

    u32_le nvmap_fd{};

    std::unordered_map<u32, u32> sync_point_values;

    u32 SetNVMAPfd(const std::vector<u8>& input, std::vector<u8>& output);
    u32 ChannelSubmit(const std::vector<u8>& input, std::vector<u8>& output);
    u32 ChannelGetSyncPoint(const std::vector<u8>& input, std::vector<u8>& output);
    u32 ChannelGetWaitBase(const std::vector<u8>& input, std::vector<u8>& output);
    u32 ChannelMapCmdBuffer(const std::vector<u8>& input, std::vector<u8>& output);
};

} // namespace Service::Nvidia::Devices
