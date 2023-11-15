// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>

#include "core/device_memory.h"
#include "core/result.h"
#include "kernel/k_auto_object.h"
#include "kernel/k_light_lock.h"
#include "kernel/k_page_group.h"
#include "kernel/k_process.h"
#include "kernel/k_typed_address.h"
#include "kernel/slab_helpers.h"
#include "kernel/svc_types.h"

namespace Kernel {

enum class CodeMemoryOperation : u32 {
    Map = 0,
    MapToOwner = 1,
    Unmap = 2,
    UnmapFromOwner = 3,
};

class KCodeMemory final
    : public KAutoObjectWithSlabHeapAndContainer<KCodeMemory, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KCodeMemory, KAutoObject);

public:
    explicit KCodeMemory(KernelCore& kernel);

    Result Initialize(Core::DeviceMemory& device_memory, KProcessAddress address, size_t size);
    void Finalize() override;

    Result Map(KProcessAddress address, size_t size);
    Result Unmap(KProcessAddress address, size_t size);
    Result MapToOwner(KProcessAddress address, size_t size, Svc::MemoryPermission perm);
    Result UnmapFromOwner(KProcessAddress address, size_t size);

    bool IsInitialized() const override {
        return m_is_initialized;
    }
    static void PostDestroy(uintptr_t arg) {}

    KProcess* GetOwner() const override {
        return m_owner;
    }
    KProcessAddress GetSourceAddress() const {
        return m_address;
    }
    size_t GetSize() const {
        return m_is_initialized ? m_page_group->GetNumPages() * PageSize : 0;
    }

private:
    std::optional<KPageGroup> m_page_group{};
    KProcess* m_owner{};
    KProcessAddress m_address{};
    KLightLock m_lock;
    bool m_is_initialized{};
    bool m_is_owner_mapped{};
    bool m_is_mapped{};
};

} // namespace Kernel
