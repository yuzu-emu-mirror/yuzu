// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <utility>
#include "common/common_funcs.h"

namespace Common {

template <typename T, size_t Size = sizeof(T), size_t Align = alignof(T)>
struct TypedStorage {
    typename std::aligned_storage<Size, Align>::type _storage;
};

template <typename T>
static T* GetPointer(TypedStorage<T>& ts) {
    return std::launder(reinterpret_cast<T*>(std::addressof(ts._storage)));
}

template <typename T>
static const T* GetPointer(const TypedStorage<T>& ts) {
    return std::launder(reinterpret_cast<const T*>(std::addressof(ts._storage)));
}

template <typename T>
static T& GetReference(TypedStorage<T>& ts) {
    return *GetPointer(ts);
}

template <typename T>
static const T& GetReference(const TypedStorage<T>& ts) {
    return *GetPointer(ts);
}

namespace Impl {

template <typename T>
static T* GetPointerForConstructAt(TypedStorage<T>& ts) {
    return reinterpret_cast<T*>(std::addressof(ts._storage));
}

} // namespace Impl

template <typename T, typename... Args>
static T* ConstructAt(TypedStorage<T>& ts, Args&&... args) {
    return std::construct_at(Impl::GetPointerForConstructAt(ts), std::forward<Args>(args)...);
}

template <typename T>
static void DestroyAt(TypedStorage<T>& ts) {
    return std::destroy_at(GetPointer(ts));
}

namespace Impl {

template <typename T>
class TypedStorageGuard {
    YUZU_NON_COPYABLE(TypedStorageGuard);

private:
    TypedStorage<T>& m_ts;
    bool m_active;

public:
    template <typename... Args>
    TypedStorageGuard(TypedStorage<T>& ts, Args&&... args) : m_ts(ts), m_active(true) {
        ConstructAt(m_ts, std::forward<Args>(args)...);
    }

    ~TypedStorageGuard() {
        if (m_active) {
            DestroyAt(m_ts);
        }
    }

    void Cancel() {
        m_active = false;
    }

    TypedStorageGuard(TypedStorageGuard&& rhs) : m_ts(rhs.m_ts), m_active(rhs.m_active) {
        rhs.Cancel();
    }

    TypedStorageGuard& operator=(TypedStorageGuard&& rhs) = delete;
};

} // namespace Impl

template <typename T, typename... Args>
static Impl::TypedStorageGuard<T> ConstructAtGuarded(TypedStorage<T>& ts, Args&&... args) {
    return Impl::TypedStorageGuard<T>(ts, std::forward<Args>(args)...);
}

} // namespace Common
