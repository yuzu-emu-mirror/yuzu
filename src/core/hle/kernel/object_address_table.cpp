// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <utility>

#include "common/assert.h"
#include "core/hle/kernel/object_address_table.h"

namespace Kernel {

ObjectAddressTable g_object_address_table;

void ObjectAddressTable::Insert(VAddr addr, SharedPtr<Object> obj) {
    ASSERT_MSG(objects.find(addr) == objects.end(), "Object already exists with addr=0x{:X}", addr);
    objects[addr] = std::move(obj);
}

void ObjectAddressTable::Close(VAddr addr) {
    ASSERT_MSG(objects.find(addr) != objects.end(), "Object does not exist with addr=0x{:X}", addr);
    objects.erase(addr);
}

SharedPtr<Object> ObjectAddressTable::GetGeneric(VAddr addr) const {
    auto iter = objects.find(addr);
    if (iter != objects.end()) {
        return iter->second;
    }
    return {};
}

void ObjectAddressTable::Clear() {
    objects.clear();
}

} // namespace Kernel
