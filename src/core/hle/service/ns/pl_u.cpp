// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/common_paths.h"
#include "common/file_util.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/ns/pl_u.h"

namespace Service::NS {

struct FontRegion {
    u32 offset;
    u32 size;
};

// The below data is specific to shared font data dumped from Switch on f/w 2.2
// Virtual address and offsets/sizes likely will vary by dump
static constexpr VAddr SHARED_FONT_MEM_VADDR{0x00000009d3016000ULL};
static constexpr u64 SHARED_FONT_MEM_SIZE{0x1100000};
static constexpr std::array<FontRegion, 6> SHARED_FONT_REGIONS{
    FontRegion{0x00000008, 0x001fe764}, FontRegion{0x001fe774, 0x00773e58},
    FontRegion{0x009725d4, 0x0001aca8}, FontRegion{0x0098d284, 0x00369cec},
    FontRegion{0x00cf6f78, 0x0039b858}, FontRegion{0x010927d8, 0x00019e80},
};

enum class LoadState : u32 {
    Loading = 0,
    Done = 1,
};

PL_U::PL_U() : ServiceFramework("pl:u") {
    static const FunctionInfo functions[] = {
        {0, &PL_U::RequestLoad, "RequestLoad"},
        {1, &PL_U::GetLoadState, "GetLoadState"},
        {2, &PL_U::GetSize, "GetSize"},
        {3, &PL_U::GetSharedMemoryAddressOffset, "GetSharedMemoryAddressOffset"},
        {4, &PL_U::GetSharedMemoryNativeHandle, "GetSharedMemoryNativeHandle"},
        {5, &PL_U::GetSharedFontInOrderOfPriority, "GetSharedFontInOrderOfPriority"},
    };
    RegisterHandlers(functions);

    // Attempt to load shared font data from disk
    const std::string filepath{FileUtil::GetUserPath(D_SYSDATA_IDX) + SHARED_FONT};
    FileUtil::CreateFullPath(filepath); // Create path if not already created
    FileUtil::IOFile file(filepath, "rb");

    shared_font = std::make_shared<std::vector<u8>>(SHARED_FONT_MEM_SIZE);
    if (file.IsOpen()) {
        // Read shared font data
        ASSERT(file.GetSize() == SHARED_FONT_MEM_SIZE);
        file.ReadBytes(shared_font->data(), shared_font->size());
    } else {
        LOG_WARNING(Service_NS, "Unable to load shared font: {}", filepath);
    }
}

void PL_U::RequestLoad(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const u32 shared_font_type{rp.Pop<u32>()};

    LOG_DEBUG(Service_NS, "called, shared_font_type={}", shared_font_type);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void PL_U::GetLoadState(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const u32 font_id{rp.Pop<u32>()};

    LOG_DEBUG(Service_NS, "called, font_id={}", font_id);
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(static_cast<u32>(LoadState::Done));
}

void PL_U::GetSize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const u32 font_id{rp.Pop<u32>()};

    LOG_DEBUG(Service_NS, "called, font_id={}", font_id);
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(SHARED_FONT_REGIONS[font_id].size);
}

void PL_U::GetSharedMemoryAddressOffset(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const u32 font_id{rp.Pop<u32>()};

    LOG_DEBUG(Service_NS, "called, font_id={}", font_id);
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(SHARED_FONT_REGIONS[font_id].offset);
}

void PL_U::GetSharedMemoryNativeHandle(Kernel::HLERequestContext& ctx) {
    // TODO(bunnei): This is a less-than-ideal solution to load a RAM dump of the Switch shared
    // font data. This (likely) relies on exact address, size, and offsets from the original
    // dump. In the future, we need to replace this with a more robust solution.

    // Map backing memory for the font data
    Core::CurrentProcess()->vm_manager.MapMemoryBlock(
        SHARED_FONT_MEM_VADDR, shared_font, 0, SHARED_FONT_MEM_SIZE, Kernel::MemoryState::Shared);

    // Create shared font memory object
    shared_font_mem = Kernel::SharedMemory::Create(
        Core::CurrentProcess(), SHARED_FONT_MEM_SIZE, Kernel::MemoryPermission::ReadWrite,
        Kernel::MemoryPermission::Read, SHARED_FONT_MEM_VADDR, Kernel::MemoryRegion::BASE,
        "PL_U:shared_font_mem");

    LOG_DEBUG(Service_NS, "called");
    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyObjects(shared_font_mem);
}

void PL_U::GetSharedFontInOrderOfPriority(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const u64 language_code{rp.Pop<u64>()}; // TODO(ogniK): Find out what this is used for
    LOG_DEBUG(Service_NS, "called, language_code=%lx", language_code);
    IPC::ResponseBuilder rb{ctx, 4};
    std::vector<u32> font_codes;
    std::vector<u32> font_offsets;
    std::vector<u32> font_sizes;

    // TODO(ogniK): Have actual priority order
    for (size_t i = 0; i < SHARED_FONT_REGIONS.size(); i++) {
        font_codes.push_back(static_cast<u32>(i));
        font_offsets.push_back(SHARED_FONT_REGIONS[i].offset);
        font_sizes.push_back(SHARED_FONT_REGIONS[i].size);
    }

    ctx.WriteBuffer(font_codes.data(), font_codes.size() * sizeof(u32), 0);
    ctx.WriteBuffer(font_offsets.data(), font_offsets.size() * sizeof(u32), 1);
    ctx.WriteBuffer(font_sizes.data(), font_sizes.size() * sizeof(u32), 2);

    rb.Push(RESULT_SUCCESS);
    rb.Push<u8>(static_cast<u8>(LoadState::Done)); // Fonts Loaded
    rb.Push<u32>(static_cast<u32>(font_codes.size()));
}

} // namespace Service::NS
