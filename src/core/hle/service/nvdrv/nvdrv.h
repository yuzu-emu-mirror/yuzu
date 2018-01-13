// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include "common/common_types.h"
#include "core/hle/service/service.h"

namespace Service {
namespace NVDRV {

class nvdevice {
public:
    virtual ~nvdevice() = default;

    virtual u32 ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) = 0;
};

class nvmap : public nvdevice {
public:
    /// Returns the allocated address of an nvmap object given its handle.
    VAddr GetObjectAddress(u32 handle) const;

    u32 ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) override;
private:
    // Represents an nvmap object.
    struct Object {
        enum class Status { Created, Allocated };
        u32 id;
        u32 size;
        u32 flags;
        u32 align;
        u8 kind;
        VAddr addr;
        Status status;
    };

    u32 next_handle = 1;
    u32 next_id = 1;
    std::unordered_map<u32, std::shared_ptr<Object>> handles;

    enum IoctlCommands {
        IocCreateCommand = 0xC0080101,
        IocFromIdCommand = 0xC0080103,
        IocAllocCommand = 0xC0200104,
        IocParamCommand = 0xC00C0109,
        IocGetIdCommand = 0xC008010E
    };

    struct IocCreateParams {
        // Input
        u32_le size;
        // Output
        u32_le handle;
    };

    struct IocAllocParams {
        // Input
        u32_le handle;
        u32_le heap_mask;
        u32_le flags;
        u32_le align;
        u8 kind;
        INSERT_PADDING_BYTES(7);
        u64_le addr;
    };

    struct IocGetIdParams {
        // Output
        u32_le id;
        // Input
        u32_le handle;
    };

    struct IocFromIdParams {
        // Input
        u32_le id;
        // Output
        u32_le handle;
    };

    struct IocParamParams {
        // Input
        u32_le handle;
        u32_le type;
        // Output
        u32_le value;
    };

    u32 IocCreate(const std::vector<u8>& input, std::vector<u8>& output);
    u32 IocAlloc(const std::vector<u8>& input, std::vector<u8>& output);
    u32 IocGetId(const std::vector<u8>& input, std::vector<u8>& output);
    u32 IocFromId(const std::vector<u8>& input, std::vector<u8>& output);
    u32 IocParam(const std::vector<u8>& input, std::vector<u8>& output);
};

class nvdisp_disp0 : public nvdevice {
public:
    nvdisp_disp0(std::shared_ptr<nvmap> nvmap_dev) : nvdevice(), nvmap_dev(std::move(nvmap_dev)) {}
    ~nvdisp_disp0() = default;

    u32 ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) override;

    /// Performs a screen flip, drawing the buffer pointed to by the handle.
    void flip(u32 buffer_handle, u32 offset, u32 format, u32 width, u32 height, u32 stride);

private:
    std::shared_ptr<nvmap> nvmap_dev;
};

/// Registers all NVDRV services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace NVDRV
} // namespace Service
