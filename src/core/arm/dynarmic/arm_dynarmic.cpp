// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include <memory>
#include <dynarmic/A64/a64.h>
#include <dynarmic/A64/config.h>
#include "core/arm/dynarmic/arm_dynarmic.h"
#include "core/core_timing.h"
#include "core/hle/kernel/memory.h"
#include "core/hle/kernel/svc.h"
#include "core/memory.h"

using Vector = Dynarmic::A64::Vector;

class ARM_Dynarmic_Callbacks : public Dynarmic::A64::UserCallbacks {
public:
    explicit ARM_Dynarmic_Callbacks(ARM_Dynarmic& parent) : parent(parent) {}
    ~ARM_Dynarmic_Callbacks() = default;

    u8 MemoryRead8(u64 vaddr) override {
        return Memory::Read8(vaddr);
    }
    u16 MemoryRead16(u64 vaddr) override {
        return Memory::Read16(vaddr);
    }
    u32 MemoryRead32(u64 vaddr) override {
        return Memory::Read32(vaddr);
    }
    u64 MemoryRead64(u64 vaddr) override {
        return Memory::Read64(vaddr);
    }
    Vector MemoryRead128(u64 vaddr) override {
        return {Memory::Read64(vaddr), Memory::Read64(vaddr + 8)};
    }

    void MemoryWrite8(u64 vaddr, u8 value) override {
        Memory::Write8(vaddr, value);
    }
    void MemoryWrite16(u64 vaddr, u16 value) override {
        Memory::Write16(vaddr, value);
    }
    void MemoryWrite32(u64 vaddr, u32 value) override {
        Memory::Write32(vaddr, value);
    }
    void MemoryWrite64(u64 vaddr, u64 value) override {
        Memory::Write64(vaddr, value);
    }
    void MemoryWrite128(u64 vaddr, Vector value) override {
        Memory::Write64(vaddr, value[0]);
        Memory::Write64(vaddr + 8, value[1]);
    }

    void InterpreterFallback(u64 pc, size_t num_instructions) override {
        ARM_Interface::ThreadContext ctx;
        parent.SaveContext(ctx);
        parent.inner_unicorn.LoadContext(ctx);
        parent.inner_unicorn.ExecuteInstructions(static_cast<int>(num_instructions));
        parent.inner_unicorn.SaveContext(ctx);
        parent.LoadContext(ctx);
        num_interpreted_instructions += num_instructions;
    }

    void ExceptionRaised(u64 pc, Dynarmic::A64::Exception exception) override {
        ASSERT_MSG(false, "ExceptionRaised(exception = %zu, pc = %" PRIx64 ")",
                   static_cast<size_t>(exception), pc);
    }

    void CallSVC(u32 swi) override {
        Kernel::CallSVC(swi);
    }

    void AddTicks(u64 ticks) override {
        if (ticks > ticks_remaining) {
            ticks_remaining = 0;
            return;
        }
        ticks -= ticks_remaining;
    }
    u64 GetTicksRemaining() override {
        return ticks_remaining;
    }

    ARM_Dynarmic& parent;
    size_t ticks_remaining = 0;
    size_t num_interpreted_instructions = 0;
    u64 tpidrr0_el0 = 0;
};

std::unique_ptr<Dynarmic::A64::Jit> MakeJit(const std::unique_ptr<ARM_Dynarmic_Callbacks>& cb) {
    Dynarmic::A64::UserConfig config{cb.get()};
    return std::make_unique<Dynarmic::A64::Jit>(config);
}

ARM_Dynarmic::ARM_Dynarmic()
    : cb(std::make_unique<ARM_Dynarmic_Callbacks>(*this)), jit(MakeJit(cb)) {
    ARM_Interface::ThreadContext ctx;
    inner_unicorn.SaveContext(ctx);
    LoadContext(ctx);
}

ARM_Dynarmic::~ARM_Dynarmic() = default;

void ARM_Dynarmic::MapBackingMemory(u64 address, size_t size, u8* memory,
                                    Kernel::VMAPermission perms) {
    inner_unicorn.MapBackingMemory(address, size, memory, perms);
}

void ARM_Dynarmic::SetPC(u64 pc) {
    jit->SetPC(pc);
}

u64 ARM_Dynarmic::GetPC() const {
    return jit->GetPC();
}

u64 ARM_Dynarmic::GetReg(int index) const {
    return jit->GetRegister(index);
}

void ARM_Dynarmic::SetReg(int index, u64 value) {
    jit->SetRegister(index, value);
}

u128 ARM_Dynarmic::GetExtReg(int index) const {
    return jit->GetVector(index);
}

void ARM_Dynarmic::SetExtReg(int index, u128 value) {
    jit->SetVector(index, value);
}

u32 ARM_Dynarmic::GetVFPReg(int /*index*/) const {
    UNIMPLEMENTED();
    return {};
}

void ARM_Dynarmic::SetVFPReg(int /*index*/, u32 /*value*/) {
    UNIMPLEMENTED();
}

u32 ARM_Dynarmic::GetCPSR() const {
    return jit->GetPstate();
}

void ARM_Dynarmic::SetCPSR(u32 cpsr) {
    jit->SetPstate(cpsr);
}

u64 ARM_Dynarmic::GetTlsAddress() const {
    return cb->tpidrr0_el0;
}

void ARM_Dynarmic::SetTlsAddress(u64 address) {
    cb->tpidrr0_el0 = address;
}

void ARM_Dynarmic::ExecuteInstructions(int num_instructions) {
    cb->ticks_remaining = num_instructions;
    jit->Run();
    CoreTiming::AddTicks(num_instructions - cb->num_interpreted_instructions);
    cb->num_interpreted_instructions = 0;
}

void ARM_Dynarmic::SaveContext(ARM_Interface::ThreadContext& ctx) {
    ctx.cpu_registers = jit->GetRegisters();
    ctx.sp = jit->GetSP();
    ctx.pc = jit->GetPC();
    ctx.cpsr = jit->GetPstate();
    ctx.fpu_registers = jit->GetVectors();
    ctx.fpscr = jit->GetFpcr();
    ctx.tls_address = cb->tpidrr0_el0;
}

void ARM_Dynarmic::LoadContext(const ARM_Interface::ThreadContext& ctx) {
    jit->SetRegisters(ctx.cpu_registers);
    jit->SetSP(ctx.sp);
    jit->SetPC(ctx.pc);
    jit->SetPstate(static_cast<u32>(ctx.cpsr));
    jit->SetVectors(ctx.fpu_registers);
    jit->SetFpcr(static_cast<u32>(ctx.fpscr));
    cb->tpidrr0_el0 = ctx.tls_address;
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
    jit = MakeJit(cb);
}
