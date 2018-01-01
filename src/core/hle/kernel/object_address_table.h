// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>
#include "common/common_types.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

/**
 * This class is used to keep a table of Kernel objects and their respective addresses in emulated
 * memory. For certain Switch SVCs, Kernel objects are referenced by an address to an object the
 * guest application manages, so we use this table to look these kernel objects up. This is similiar
 * to the HandleTable class.
 */
class ObjectAddressTable final : NonCopyable {
public:
    ObjectAddressTable() = default;

    /**
     * Inserts an object and address pair into the table.
     */
    void Insert(VAddr addr, SharedPtr<Object> obj);

    /**
     * Closes an object by its address, removing it from the table and decreasing the object's
     * ref-count.
     * @return `RESULT_SUCCESS` or one of the following errors:
     *           - `ERR_INVALID_HANDLE`: an invalid handle was passed in.
     */
    void Close(VAddr addr);

    /**
     * Looks up an object by its address.
     * @return Pointer to the looked-up object, or `nullptr` if the handle is not valid.
     */
    SharedPtr<Object> GetGeneric(VAddr addr) const;

    /**
     * Looks up an object by its address while verifying its type.
     * @return Pointer to the looked-up object, or `nullptr` if the handle is not valid or its
     *         type differs from the requested one.
     */
    template <class T>
    SharedPtr<T> Get(VAddr addr) const {
        return DynamicObjectCast<T>(GetGeneric(addr));
    }

    /// Closes all addresses held in this table.
    void Clear();

private:
    /// Stores the Object referenced by the address
    std::map<VAddr, SharedPtr<Object>> objects;
};

extern ObjectAddressTable g_object_address_table;

} // namespace Kernel
