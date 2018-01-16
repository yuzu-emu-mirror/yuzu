// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <unicorn/arm64.h>
#include "common/assert.h"
#include "common/microprofile.h"
#include "core/arm/unicorn/arm_unicorn.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/kernel/svc.h"

// Load Unicorn DLL once on Windows using RAII
#ifdef _MSC_VER
#include <unicorn_dynload.h>
struct LoadDll {
private:
    LoadDll() {
        ASSERT(uc_dyn_load(NULL, 0));
    }
    ~LoadDll() {
        ASSERT(uc_dyn_free());
    }
    static LoadDll g_load_dll;
};
LoadDll LoadDll::g_load_dll;
#endif

#define CHECKED(expr)                                                                              \
    do {                                                                                           \
        if (auto _cerr = (expr)) {                                                                 \
            ASSERT_MSG(false, "Call " #expr " failed with error: %u (%s)\n", _cerr,                \
                       uc_strerror(_cerr));                                                        \
        }                                                                                          \
    } while (0)

static void InterruptHook(uc_engine* uc, u32 intNo, void* user_data) {
    u32 esr{};
    CHECKED(uc_reg_read(uc, UC_ARM64_REG_ESR, &esr));

    auto ec = esr >> 26;
    auto iss = esr & 0xFFFFFF;

    switch (ec) {
    case 0x15: // SVC
        Kernel::CallSVC(iss);
        break;
    }
}

static bool UnmappedMemoryHook(uc_engine* uc, uc_mem_type type, u64 addr, int size, u64 value,
                               void* user_data) {
    ARM_Interface::ThreadContext ctx{};
    Core::CPU().SaveContext(ctx);
    ASSERT_MSG(false, "Attempted to read from unmapped memory: 0x%llx", addr);
    return {};
}

ARM_Unicorn::ARM_Unicorn() {
    CHECKED(uc_open(UC_ARCH_ARM64, UC_MODE_ARM, &uc));

    auto fpv = 3 << 20;
    CHECKED(uc_reg_write(uc, UC_ARM64_REG_CPACR_EL1, &fpv));

    uc_hook hook{};
    CHECKED(uc_hook_add(uc, &hook, UC_HOOK_INTR, (void*)InterruptHook, this, 0, -1));
    CHECKED(uc_hook_add(uc, &hook, UC_HOOK_MEM_INVALID, (void*)UnmappedMemoryHook, this, 0, -1));
}

ARM_Unicorn::~ARM_Unicorn() {
    CHECKED(uc_close(uc));
}

void ARM_Unicorn::MapBackingMemory(VAddr address, size_t size, u8* memory,
                                   Kernel::VMAPermission perms) {
    CHECKED(uc_mem_map_ptr(uc, address, size, static_cast<u32>(perms), memory));
}

void ARM_Unicorn::SetPC(u64 pc) {
    CHECKED(uc_reg_write(uc, UC_ARM64_REG_PC, &pc));
}

u64 ARM_Unicorn::GetPC() const {
    u64 val{};
    CHECKED(uc_reg_read(uc, UC_ARM64_REG_PC, &val));
    return val;
}

u64 ARM_Unicorn::GetReg(int regn) const {
    u64 val{};
    auto treg = UC_ARM64_REG_SP;
    if (regn <= 28) {
        treg = (uc_arm64_reg)(UC_ARM64_REG_X0 + regn);
    } else if (regn < 31) {
        treg = (uc_arm64_reg)(UC_ARM64_REG_X29 + regn - 29);
    }
    CHECKED(uc_reg_read(uc, treg, &val));
    return val;
}

void ARM_Unicorn::SetReg(int regn, u64 val) {
    auto treg = UC_ARM64_REG_SP;
    if (regn <= 28) {
        treg = (uc_arm64_reg)(UC_ARM64_REG_X0 + regn);
    } else if (regn < 31) {
        treg = (uc_arm64_reg)(UC_ARM64_REG_X29 + regn - 29);
    }
    CHECKED(uc_reg_write(uc, treg, &val));
}

u128 ARM_Unicorn::GetExtReg(int /*index*/) const {
    UNIMPLEMENTED();
    static constexpr u128 res{};
    return res;
}

void ARM_Unicorn::SetExtReg(int /*index*/, u128 /*value*/) {
    UNIMPLEMENTED();
}

u32 ARM_Unicorn::GetVFPReg(int /*index*/) const {
    UNIMPLEMENTED();
    return {};
}

void ARM_Unicorn::SetVFPReg(int /*index*/, u32 /*value*/) {
    UNIMPLEMENTED();
}

u32 ARM_Unicorn::GetCPSR() const {
    u64 nzcv{};
    CHECKED(uc_reg_read(uc, UC_ARM64_REG_NZCV, &nzcv));
    return static_cast<u32>(nzcv);
}

void ARM_Unicorn::SetCPSR(u32 cpsr) {
    u64 nzcv = cpsr;
    CHECKED(uc_reg_write(uc, UC_ARM64_REG_NZCV, &nzcv));
}

VAddr ARM_Unicorn::GetTlsAddress() const {
    u64 base{};
    CHECKED(uc_reg_read(uc, UC_ARM64_REG_TPIDRRO_EL0, &base));
    return base;
}

void ARM_Unicorn::SetTlsAddress(VAddr base) {
    CHECKED(uc_reg_write(uc, UC_ARM64_REG_TPIDRRO_EL0, &base));
}

MICROPROFILE_DEFINE(ARM_Jit, "ARM JIT", "ARM JIT", MP_RGB(255, 64, 64));

void ARM_Unicorn::ExecuteInstructions(int num_instructions) {
    MICROPROFILE_SCOPE(ARM_Jit);
    CHECKED(uc_emu_start(uc, GetPC(), 1ULL << 63, 0, num_instructions));
    CoreTiming::AddTicks(num_instructions);
}

void ARM_Unicorn::SaveContext(ARM_Interface::ThreadContext& ctx) {
    int uregs[32];
    void* tregs[32];

    CHECKED(uc_reg_read(uc, UC_ARM64_REG_SP, &ctx.sp));
    CHECKED(uc_reg_read(uc, UC_ARM64_REG_PC, &ctx.pc));
    CHECKED(uc_reg_read(uc, UC_ARM64_REG_NZCV, &ctx.cpsr));

    for (auto i = 0; i < 29; ++i) {
        uregs[i] = UC_ARM64_REG_X0 + i;
        tregs[i] = &ctx.cpu_registers[i];
    }
    uregs[29] = UC_ARM64_REG_X29;
    tregs[29] = (void*)&ctx.cpu_registers[29];
    uregs[30] = UC_ARM64_REG_X30;
    tregs[30] = (void*)&ctx.cpu_registers[30];

    CHECKED(uc_reg_read_batch(uc, uregs, tregs, 31));

    ctx.tls_address = GetTlsAddress();

    for (int i = 0; i < 32; ++i) {
        uregs[i] = UC_ARM64_REG_Q0 + i;
        tregs[i] = &ctx.fpu_registers[i];
    }

    CHECKED(uc_reg_read_batch(uc, uregs, tregs, 32));
}

void ARM_Unicorn::LoadContext(const ARM_Interface::ThreadContext& ctx) {
    int uregs[32];
    void* tregs[32];

    CHECKED(uc_reg_write(uc, UC_ARM64_REG_SP, &ctx.sp));
    CHECKED(uc_reg_write(uc, UC_ARM64_REG_PC, &ctx.pc));
    CHECKED(uc_reg_write(uc, UC_ARM64_REG_NZCV, &ctx.cpsr));

    for (int i = 0; i < 29; ++i) {
        uregs[i] = UC_ARM64_REG_X0 + i;
        tregs[i] = (void*)&ctx.cpu_registers[i];
    }
    uregs[29] = UC_ARM64_REG_X29;
    tregs[29] = (void*)&ctx.cpu_registers[29];
    uregs[30] = UC_ARM64_REG_X30;
    tregs[30] = (void*)&ctx.cpu_registers[30];

    CHECKED(uc_reg_write_batch(uc, uregs, tregs, 31));

    SetTlsAddress(ctx.tls_address);

    for (auto i = 0; i < 32; ++i) {
        uregs[i] = UC_ARM64_REG_Q0 + i;
        tregs[i] = (void*)&ctx.fpu_registers[i];
    }

    CHECKED(uc_reg_write_batch(uc, uregs, tregs, 32));
}

void ARM_Unicorn::PrepareReschedule() {
    CHECKED(uc_emu_stop(uc));
}

void ARM_Unicorn::ClearInstructionCache() {}
