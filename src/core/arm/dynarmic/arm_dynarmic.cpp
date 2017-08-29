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
    //ARMul_State* state = static_cast<ARMul_State*>(user_arg);

    //state->Reg = jit->Regs();
    //state->Cpsr = jit->Cpsr();
    //state->Reg[15] = static_cast<u32>(pc);
    //state->ExtReg = jit->ExtRegs();
    //state->VFP[VFP_FPSCR] = jit->Fpscr();
    //state->NumInstrsToExecute = 1;

    //InterpreterMainLoop(state);

    //bool is_thumb = (state->Cpsr & (1 << 5)) != 0;
    //state->Reg[15] &= (is_thumb ? 0xFFFFFFFE : 0xFFFFFFFC);

    //jit->Regs() = state->Reg;
    //jit->Cpsr() = state->Cpsr;
    //jit->ExtRegs() = state->ExtReg;
    //jit->SetFpscr(state->VFP[VFP_FPSCR]);
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

static Dynarmic::UserCallbacks GetUserCallbacks(
    const std::shared_ptr<ARMul_State>& interpeter_state) {
    Dynarmic::UserCallbacks user_callbacks{};
    //user_callbacks.InterpreterFallback = &InterpreterFallback;
    //user_callbacks.user_arg = static_cast<void*>(interpeter_state.get());
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
    user_callbacks.coprocessors[15] = std::make_shared<DynarmicCP15>(interpeter_state);
    return user_callbacks;
}

ARM_Dynarmic::ARM_Dynarmic(PrivilegeMode initial_mode) {
    interpreter_state = std::make_shared<ARMul_State>(initial_mode);
    jit = std::make_unique<Dynarmic::Jit>(GetUserCallbacks(interpreter_state), Dynarmic::Arch::ARM64);
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

u32 ARM_Dynarmic::GetVFPReg(int index) const {
    return jit->ExtRegs()[index];
}

void ARM_Dynarmic::SetVFPReg(int index, u32 value) {
    jit->ExtRegs()[index] = value;
}

u32 ARM_Dynarmic::GetVFPSystemReg(VFPSystemRegister reg) const {
    if (reg == VFP_FPSCR) {
        return jit->Fpscr();
    }

    // Dynarmic does not implement and/or expose other VFP registers, fallback to interpreter state
    return interpreter_state->VFP[reg];
}

void ARM_Dynarmic::SetVFPSystemReg(VFPSystemRegister reg, u32 value) {
    if (reg == VFP_FPSCR) {
        jit->SetFpscr(value);
    }

    // Dynarmic does not implement and/or expose other VFP registers, fallback to interpreter state
    interpreter_state->VFP[reg] = value;
}

u32 ARM_Dynarmic::GetCPSR() const {
    return jit->Cpsr();
}

void ARM_Dynarmic::SetCPSR(u32 cpsr) {
    jit->Cpsr() = cpsr;
}

u32 ARM_Dynarmic::GetCP15Register(CP15Register reg) {
    return interpreter_state->CP15[reg];
}

void ARM_Dynarmic::SetCP15Register(CP15Register reg, u32 value) {
    interpreter_state->CP15[reg] = value;
}

void ARM_Dynarmic::AddTicks(u64 ticks) {
    down_count -= ticks;
    if (down_count < 0) {
        CoreTiming::Advance();
    }
}

MICROPROFILE_DEFINE(ARM_Jit, "ARM JIT", "ARM JIT", MP_RGB(255, 64, 64));

void ARM_Dynarmic::ExecuteInstructions(int num_instructions) {
    MICROPROFILE_SCOPE(ARM_Jit);

    unsigned ticks_executed = jit->Run(1 /*static_cast<unsigned>(num_instructions)*/);

    AddTicks(ticks_executed);
}

void ARM_Dynarmic::SaveContext(ARM_Interface::ThreadContext& ctx) {
    memcpy(ctx.cpu_registers, jit->Regs64().data(), sizeof(ctx.cpu_registers));
    //memcpy(ctx.fpu_registers, jit->ExtRegs().data(), sizeof(ctx.fpu_registers));

    ctx.lr = jit->Regs64()[30];
    ctx.sp = jit->Regs64()[31];
    ctx.pc = jit->Regs64()[32];
    ctx.cpsr = jit->Cpsr();

    ctx.fpscr = jit->Fpscr();
    ctx.fpexc = interpreter_state->VFP[VFP_FPEXC];
}

void ARM_Dynarmic::LoadContext(const ARM_Interface::ThreadContext& ctx) {
    memcpy(jit->Regs64().data(), ctx.cpu_registers, sizeof(ctx.cpu_registers));
    //memcpy(jit->ExtRegs().data(), ctx.fpu_registers, sizeof(ctx.fpu_registers));

    jit->Regs64()[30] = ctx.lr;
    jit->Regs64()[31] = ctx.sp;
    jit->Regs64()[32] = ctx.pc;
    jit->Cpsr() = ctx.cpsr;

    jit->SetFpscr(ctx.fpscr);
    interpreter_state->VFP[VFP_FPEXC] = ctx.fpexc;
}

void ARM_Dynarmic::PrepareReschedule() {
    if (jit->IsExecuting()) {
        jit->HaltExecution();
    }
}

void ARM_Dynarmic::ClearInstructionCache() {
    jit->ClearCache();
}
