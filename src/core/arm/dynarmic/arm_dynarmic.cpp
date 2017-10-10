// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <dynarmic/dynarmic.h>
#include "common/assert.h"
#include "common/microprofile.h"
#include "core/arm/dynarmic/arm_dynarmic.h"
#include "core/arm/dynarmic/arm_dynarmic_cp15.h"
#include "core/arm/dyncom/arm_dyncom_interpreter.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/svc.h"
#include "core/memory.h"

static void InterpreterFallback(u64 pc, Dynarmic::Jit* jit, void* user_arg) {
    UNIMPLEMENTED_MSG("InterpreterFallback for ARM64 JIT does not exist!");
}

static bool IsReadOnlyMemory(u64 vaddr) {
    // TODO(bunnei): ImplementMe
    return false;
}

u8 MemoryRead8(const u64 addr) {
    return Memory::Read8(static_cast<VAddr>(addr));
}

u16 MemoryRead16(const u64 addr) {
    return Memory::Read16(static_cast<VAddr>(addr));
}

u32 MemoryRead32(const u64 addr) {
    return Memory::Read32(static_cast<VAddr>(addr));
}

u64 MemoryRead64(const u64 addr) {
    return Memory::Read64(static_cast<VAddr>(addr));
}

void MemoryWrite8(const u64 addr, const u8 data) {
    Memory::Write8(static_cast<VAddr>(addr), data);
}

void MemoryWrite16(const u64 addr, const u16 data) {
    Memory::Write16(static_cast<VAddr>(addr), data);
}

void MemoryWrite32(const u64 addr, const u32 data) {
    Memory::Write32(static_cast<VAddr>(addr), data);
}

void MemoryWrite64(const u64 addr, const u64 data) {
    Memory::Write64(static_cast<VAddr>(addr), data);
}

static Dynarmic::UserCallbacks GetUserCallbacks(ARM_Dynarmic* this_) {
    Dynarmic::UserCallbacks user_callbacks{};
    user_callbacks.InterpreterFallback = &InterpreterFallback;
    user_callbacks.user_arg = static_cast<void*>(this_);
    user_callbacks.CallSVC = &SVC::CallSVC;
    user_callbacks.memory.IsReadOnlyMemory = &IsReadOnlyMemory;
    user_callbacks.memory.ReadCode = &MemoryRead32;
    user_callbacks.memory.Read8 = &MemoryRead8;
    user_callbacks.memory.Read16 = &MemoryRead16;
    user_callbacks.memory.Read32 = &MemoryRead32;
    user_callbacks.memory.Read64 = &MemoryRead64;
    user_callbacks.memory.Write8 = &MemoryWrite8;
    user_callbacks.memory.Write16 = &MemoryWrite16;
    user_callbacks.memory.Write32 = &MemoryWrite32;
    user_callbacks.memory.Write64 = &MemoryWrite64;
    //user_callbacks.page_table = Memory::GetCurrentPageTablePointers();
    return user_callbacks;
}

ARM_Dynarmic::ARM_Dynarmic(PrivilegeMode initial_mode) {
}

void ARM_Dynarmic::MapBackingMemory(VAddr address, size_t size, u8* memory, Kernel::VMAPermission perms) {
}

void ARM_Dynarmic::SetPC(u64 pc) {
    jit->Regs64()[32] = pc;
}

u64 ARM_Dynarmic::GetPC() const {
    return jit->Regs64()[32];
}

u64 ARM_Dynarmic::GetReg(int index) const {
    return jit->Regs64()[index];
}

void ARM_Dynarmic::SetReg(int index, u64 value) {
    jit->Regs64()[index] = value;
}

const u128& ARM_Dynarmic::GetExtReg(int index) const {
    return jit->ExtRegs64()[index];
}

void ARM_Dynarmic::SetExtReg(int index, u128& value) {
    jit->ExtRegs64()[index] = value;
}

u32 ARM_Dynarmic::GetVFPReg(int index) const {
    return {};
}

void ARM_Dynarmic::SetVFPReg(int index, u32 value) {
}

u32 ARM_Dynarmic::GetVFPSystemReg(VFPSystemRegister reg) const {
    return {};
}

void ARM_Dynarmic::SetVFPSystemReg(VFPSystemRegister reg, u32 value) {
}

u32 ARM_Dynarmic::GetCPSR() const {
    return jit->Cpsr();
}

void ARM_Dynarmic::SetCPSR(u32 cpsr) {
    jit->Cpsr() = cpsr;
}

u32 ARM_Dynarmic::GetCP15Register(CP15Register reg) {
    return {};
}

void ARM_Dynarmic::SetCP15Register(CP15Register reg, u32 value) {
}

VAddr ARM_Dynarmic::GetTlsAddress() const {
    return jit->TlsAddr();
}

void ARM_Dynarmic::SetTlsAddress(VAddr address) {
    jit->TlsAddr() = address;
}

MICROPROFILE_DEFINE(ARM_Jit, "ARM JIT", "ARM JIT", MP_RGB(255, 64, 64));

void ARM_Dynarmic::ExecuteInstructions(int num_instructions) {
    ASSERT(Memory::GetCurrentPageTable() == current_page_table);
    MICROPROFILE_SCOPE(ARM_Jit);

    std::size_t ticks_executed = jit->Run(static_cast<unsigned>(num_instructions));

    CoreTiming::AddTicks(ticks_executed);
}

void ARM_Dynarmic::SaveContext(ARM_Interface::ThreadContext& ctx) {
    memcpy(ctx.cpu_registers, jit->Regs64().data(), sizeof(ctx.cpu_registers));
    memcpy(ctx.fpu_registers, jit->ExtRegs64().data(), sizeof(ctx.fpu_registers));

    ctx.lr = jit->Regs64()[30];
    ctx.sp = jit->Regs64()[31];
    ctx.pc = jit->Regs64()[32];
    ctx.cpsr = jit->Cpsr();

    // TODO(bunnei): Fix once we have proper support for tpidrro_el0, etc. in the JIT
    ctx.tls_address = jit->TlsAddr();
}

void ARM_Dynarmic::LoadContext(const ARM_Interface::ThreadContext& ctx) {
    memcpy(jit->Regs64().data(), ctx.cpu_registers, sizeof(ctx.cpu_registers));
    memcpy(jit->ExtRegs64().data(), ctx.fpu_registers, sizeof(ctx.fpu_registers));

    jit->Regs64()[30] = ctx.lr;
    jit->Regs64()[31] = ctx.sp;
    jit->Regs64()[32] = ctx.pc;
    jit->Cpsr() = ctx.cpsr;

    // TODO(bunnei): Fix once we have proper support for tpidrro_el0, etc. in the JIT
    jit->TlsAddr() = ctx.tls_address;
}

void ARM_Dynarmic::PrepareReschedule() {
    if (jit->IsExecuting()) {
        jit->HaltExecution();
    }
}

void ARM_Dynarmic::ClearInstructionCache() {
    jit->ClearCache();
}

void ARM_Dynarmic::PageTableChanged() {
    current_page_table = Memory::GetCurrentPageTable();

    auto iter = jits.find(current_page_table);
    if (iter != jits.end()) {
        jit = iter->second.get();
        return;
    }

    jit = new Dynarmic::Jit(GetUserCallbacks(this), Dynarmic::Arch::ARM64);
    jits.emplace(current_page_table, std::unique_ptr<Dynarmic::Jit>(jit));
}
