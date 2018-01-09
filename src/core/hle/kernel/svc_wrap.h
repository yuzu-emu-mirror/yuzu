// Copyright 2018 Yuzu Emulator Team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/arm/arm_interface.h"
#include "core/core.h"
#include "core/hle/kernel/svc.h"
#include "core/hle/result.h"
#include "core/memory.h"

namespace Kernel {

#define PARAM(n) Core::CPU().GetReg(n)

/**
 * HLE a function return from the current ARM userland process
 * @param res Result to return
 */
static inline void FuncReturn(u64 res) {
    Core::CPU().SetReg(0, res);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function wrappers that return type ResultCode

template <ResultCode func(u64)>
void SvcWrap() {
    FuncReturn(func(PARAM(0)).raw);
}

template <ResultCode func(u32)>
void SvcWrap() {
    FuncReturn(func((u32)PARAM(0)).raw);
}

template <ResultCode func(u32, u32)>
void SvcWrap() {
    FuncReturn(func((u32)PARAM(0), (u32)PARAM(1)).raw);
}

template <ResultCode func(u32*, u32)>
void SvcWrap() {
    u32 param_1 = 0;
    u32 retval = func(&param_1, (u32)PARAM(1)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32*, u64)>
void SvcWrap() {
    u32 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u64, s32)>
void SvcWrap() {
    FuncReturn(func(PARAM(0), (s32)PARAM(1)).raw);
}

template <ResultCode func(u64*, u64)>
void SvcWrap() {
    u64 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32, u64, u32)>
void SvcWrap() {
    FuncReturn(func((u32)PARAM(0), PARAM(1), (u32)PARAM(2)).raw);
}

template <ResultCode func(u64, u64, u64)>
void SvcWrap() {
    FuncReturn(func(PARAM(0), PARAM(1), PARAM(2)).raw);
}

template <ResultCode func(u64, u64, s64)>
void SvcWrap() {
    FuncReturn(func(PARAM(1), PARAM(2), (s64)PARAM(3)).raw);
}

template <ResultCode func(u64, u64, u32, s64)>
void SvcWrap() {
    FuncReturn(func(PARAM(0), PARAM(1), (u32)PARAM(2), (s64)PARAM(3)).raw);
}

template <ResultCode func(u64*, u64, u64, u64)>
void SvcWrap() {
    u64 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1), PARAM(2), PARAM(3)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32*, u64, u64, u64, u32, s32)>
void SvcWrap() {
    u32 param_1 = 0;
    u32 retval =
        func(&param_1, PARAM(1), PARAM(2), PARAM(3), (u32)PARAM(4), (s32)(PARAM(5) & 0xFFFFFFFF))
            .raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(MemoryInfo*, PageInfo*, u64)>
void SvcWrap() {
    MemoryInfo memory_info = {};
    PageInfo page_info = {};
    u32 retval = func(&memory_info, &page_info, PARAM(2)).raw;

    Memory::Write64(PARAM(0), memory_info.base_address);
    Memory::Write64(PARAM(0) + 8, memory_info.size);
    Memory::Write32(PARAM(0) + 16, memory_info.type);
    Memory::Write32(PARAM(0) + 20, memory_info.attributes);
    Memory::Write32(PARAM(0) + 24, memory_info.permission);

    FuncReturn(retval);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function wrappers that return type u32

template <u32 func()>
void SvcWrap() {
    FuncReturn(func());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Function wrappers that return type void

template <void func()>
void SvcWrap() {
    func();
}

template <void func(s64)>
void SvcWrap() {
    func((s64)PARAM(0));
}

template <void func(u64, s32 len)>
void SvcWrap() {
    func(PARAM(0), (s32)(PARAM(1) & 0xFFFFFFFF));
}

template <void func(u64, u64, u64)>
void SvcWrap() {
    func(PARAM(0), PARAM(1), PARAM(2));
}

#undef PARAM
#undef FuncReturn

} // namespace Kernel
