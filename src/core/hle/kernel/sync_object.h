// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include "core/hle/kernel/kernel.h"
#include "core/hle/result.h"

namespace Kernel {

class Thread;

/// Class that represents a Kernel object that svcSendSyncRequest can be called on
class SyncObject : public Object {
public:
    /**
     * Handle a sync request from the emulated application.
     * @param thread Thread that initiated the request.
     * @returns ResultCode from the operation.
     */
    virtual ResultCode SendSyncRequest(SharedPtr<Thread> thread) = 0;
};

// Specialization of DynamicObjectCast for SyncObjects
template <>
inline SharedPtr<SyncObject> DynamicObjectCast<SyncObject>(SharedPtr<Object> object) {
    if (object != nullptr && object->IsSyncable()) {
        return boost::static_pointer_cast<SyncObject>(std::move(object));
    }
    return nullptr;
}

} // namespace Kernel
