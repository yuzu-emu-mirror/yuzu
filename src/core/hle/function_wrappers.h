// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/arm/arm_interface.h"
#include "core/core.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/result.h"
#include "core/hle/svc.h"
#include "core/memory.h"

namespace HLE {

#define PARAM(n) Core::CPU().GetReg(n)

/**
 * HLE a function return from the current ARM11 userland process
 * @param res Result to return
 */
static inline void FuncReturn(u64 res) {
    Core::CPU().SetReg(0, res);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function wrappers that return type ResultCode

template <ResultCode func(u64)>
void Wrap() {
    FuncReturn(func(PARAM(0)).raw);
}

template <ResultCode func(u32, u64, u32)>
void Wrap() {
    FuncReturn(func(PARAM(0), PARAM(1), PARAM(2)).raw);
}

template <ResultCode func(u64, u32)>
void Wrap() {
    FuncReturn(func(PARAM(0), PARAM(1)).raw);
}

template <ResultCode func(u64, u64, u64)>
void Wrap() {
    FuncReturn(func(PARAM(0), PARAM(1), PARAM(2)).raw);
}

template <ResultCode func(u64, u64, s64)>
void Wrap() {
    FuncReturn(func(PARAM(1), PARAM(2), (s64)PARAM(3)).raw);
}

template <ResultCode func(u64*, u64)>
void Wrap() {
    u64 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u64*, u64, u64, u64)>
void Wrap() {
    u64 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1), PARAM(2), PARAM(3)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32, u32, u32, u32)>
void Wrap() {
    FuncReturn(func(PARAM(0), PARAM(1), PARAM(2), PARAM(3)).raw);
}

template <ResultCode func(u32*, u32, u32, u32, u32, u32)>
void Wrap() {
    u32 param_1 = 0;
    u32 retval = func(&param_1, PARAM(0), PARAM(1), PARAM(2), PARAM(3), PARAM(4)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32*, u32, u32, u32, u32, s32)>
void Wrap() {
    u32 param_1 = 0;
    u32 retval = func(&param_1, PARAM(0), PARAM(1), PARAM(2), PARAM(3), PARAM(4)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(s32*, VAddr, s32, bool, s64)>
void Wrap() {
    s32 param_1 = 0;
    s32 retval =
        func(&param_1, PARAM(1), (s32)PARAM(2), (PARAM(3) != 0), (((s64)PARAM(4) << 32) | PARAM(0)))
            .raw;

    Core::CPU().SetReg(1, (u32)param_1);
    FuncReturn(retval);
}

template <ResultCode func(s32*, VAddr, s32, u32)>
void Wrap() {
    s32 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1), (s32)PARAM(2), PARAM(3)).raw;

    Core::CPU().SetReg(1, (u32)param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32, u32, u32, u32, s64)>
void Wrap() {
    FuncReturn(
        func(PARAM(0), PARAM(1), PARAM(2), PARAM(3), (((s64)PARAM(5) << 32) | PARAM(4))).raw);
}

template <ResultCode func(u32, u64*)>
void Wrap() {
    u64 param_1 = 0;
    u32 retval = func(PARAM(0), &param_1).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32*)>
void Wrap() {
    u32 param_1 = 0;
    u32 retval = func(&param_1).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32, s64)>
void Wrap() {
    s32 retval = func(PARAM(0), (((s64)PARAM(3) << 32) | PARAM(2))).raw;

    FuncReturn(retval);
}

template <ResultCode func(MemoryInfo*, PageInfo*, u64)>
void Wrap() {
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

template <ResultCode func(s32*, u32)>
void Wrap() {
    s32 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32, s32)>
void Wrap() {
    FuncReturn(func(PARAM(0), (s32)PARAM(1)).raw);
}

template <ResultCode func(u32*, u64)>
void Wrap() {
    u32 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32*, Kernel::Handle)>
void Wrap() {
    u32 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32)>
void Wrap() {
    FuncReturn(func(PARAM(0)).raw);
}

template <ResultCode func(u32*, s32, s32)>
void Wrap() {
    u32 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1), PARAM(2)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(s32*, u32, s32)>
void Wrap() {
    s32 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1), PARAM(2)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(s64*, u32, s32)>
void Wrap() {
    s64 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1), PARAM(2)).raw;
    Core::CPU().SetReg(1, (u32)param_1);
    Core::CPU().SetReg(2, (u32)(param_1 >> 32));
    FuncReturn(retval);
}

template <ResultCode func(u32*, u32, u32, u32, u32)>
void Wrap() {
    u32 param_1 = 0;
    // The last parameter is passed in R0 instead of R4
    u32 retval = func(&param_1, PARAM(1), PARAM(2), PARAM(3), PARAM(0)).raw;
    Core::CPU().SetReg(1, param_1);
    FuncReturn(retval);
}

template <ResultCode func(u32, s64, s64)>
void Wrap() {
    s64 param1 = ((u64)PARAM(3) << 32) | PARAM(2);
    s64 param2 = ((u64)PARAM(4) << 32) | PARAM(1);
    FuncReturn(func(PARAM(0), param1, param2).raw);
}

template <ResultCode func(s64*, Kernel::Handle, u32)>
void Wrap() {
    s64 param_1 = 0;
    u32 retval = func(&param_1, PARAM(1), PARAM(2)).raw;
    Core::CPU().SetReg(1, (u32)param_1);
    Core::CPU().SetReg(2, (u32)(param_1 >> 32));
    FuncReturn(retval);
}

template <ResultCode func(Kernel::Handle, u32)>
void Wrap() {
    FuncReturn(func(PARAM(0), PARAM(1)).raw);
}

template <ResultCode func(Kernel::Handle*, Kernel::Handle*, VAddr, u32)>
void Wrap() {
    Kernel::Handle param_1 = 0;
    Kernel::Handle param_2 = 0;
    u32 retval = func(&param_1, &param_2, PARAM(2), PARAM(3)).raw;
    Core::CPU().SetReg(1, param_1);
    Core::CPU().SetReg(2, param_2);
    FuncReturn(retval);
}

template <ResultCode func(Kernel::Handle*, Kernel::Handle*)>
void Wrap() {
    Kernel::Handle param_1 = 0;
    Kernel::Handle param_2 = 0;
    u32 retval = func(&param_1, &param_2).raw;
    Core::CPU().SetReg(1, param_1);
    Core::CPU().SetReg(2, param_2);
    FuncReturn(retval);
}

template <ResultCode func(u32, u32, u32)>
void Wrap() {
    FuncReturn(func(PARAM(0), PARAM(1), PARAM(2)).raw);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function wrappers that return type u32

template <u32 func()>
void Wrap() {
    FuncReturn(func());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function wrappers that return type s64

template <s64 func()>
void Wrap() {
    FuncReturn64(func());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Function wrappers that return type void

template <void func(s64)>
void Wrap() {
    func(((s64)PARAM(1) << 32) | PARAM(0));
}

template <void func(VAddr, int len)>
void Wrap() {
    func(PARAM(0), PARAM(1));
}

template <void func(u64, u64, u64)>
void Wrap() {
    func(PARAM(0), PARAM(1), PARAM(2));
}

#undef PARAM
#undef FuncReturn

} // namespace HLE
