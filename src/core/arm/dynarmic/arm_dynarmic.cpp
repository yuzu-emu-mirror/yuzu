// Copyright 2018 Yuzu Emulator Team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/arm/dynarmic/arm_dynarmic.h"

ARM_Dynarmic::ARM_Dynarmic() {
    UNIMPLEMENTED();
}

void ARM_Dynarmic::MapBackingMemory(VAddr /*address*/, size_t /*size*/, u8* /*memory*/,
                                    Kernel::VMAPermission /*perms*/) {
    UNIMPLEMENTED();
}

void ARM_Dynarmic::SetPC(u64 /*pc*/) {
    UNIMPLEMENTED();
}

u64 ARM_Dynarmic::GetPC() const {
    UNIMPLEMENTED();
    return {};
}

u64 ARM_Dynarmic::GetReg(int /*index*/) const {
    UNIMPLEMENTED();
    return {};
}

void ARM_Dynarmic::SetReg(int /*index*/, u64 /*value*/) {
    UNIMPLEMENTED();
}

const u128& ARM_Dynarmic::GetExtReg(int /*index*/) const {
    UNIMPLEMENTED();
    static constexpr u128 res{};
    return res;
}

void ARM_Dynarmic::SetExtReg(int /*index*/, u128& /*value*/) {
    UNIMPLEMENTED();
}

u32 ARM_Dynarmic::GetVFPReg(int /*index*/) const {
    UNIMPLEMENTED();
    return {};
}

void ARM_Dynarmic::SetVFPReg(int /*index*/, u32 /*value*/) {
    UNIMPLEMENTED();
}

u32 ARM_Dynarmic::GetCPSR() const {
    UNIMPLEMENTED();
    return {};
}

void ARM_Dynarmic::SetCPSR(u32 /*cpsr*/) {
    UNIMPLEMENTED();
}

VAddr ARM_Dynarmic::GetTlsAddress() const {
    UNIMPLEMENTED();
    return {};
}

void ARM_Dynarmic::SetTlsAddress(VAddr /*address*/) {
    UNIMPLEMENTED();
}

void ARM_Dynarmic::ExecuteInstructions(int /*num_instructions*/) {
    UNIMPLEMENTED();
}

void ARM_Dynarmic::SaveContext(ARM_Interface::ThreadContext& /*ctx*/) {
    UNIMPLEMENTED();
}

void ARM_Dynarmic::LoadContext(const ARM_Interface::ThreadContext& /*ctx*/) {
    UNIMPLEMENTED();
}

void ARM_Dynarmic::PrepareReschedule() {
    UNIMPLEMENTED();
}

void ARM_Dynarmic::ClearInstructionCache() {
    UNIMPLEMENTED();
}

void ARM_Dynarmic::PageTableChanged() {
    UNIMPLEMENTED();
}
