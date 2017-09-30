// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/arm/skyeye_common/arm_regformat.h"
#include "core/arm/skyeye_common/vfp/asm_vfp.h"

/// Generic ARM11 CPU interface
class ARM_Interface : NonCopyable {
public:
    virtual ~ARM_Interface() {}

    struct ThreadContext {
        u64 cpu_registers[30];
        u64 lr;
        u64 sp;
        u64 pc;
        u64 cpsr;
        u64 fpu_registers[64];
        u64 fpscr;
        u64 fpexc;

        // TODO(bunnei): Fix once we have proper support for tpidrro_el0, etc. in the JIT
        VAddr tls_address;
    };

    /**
     * Runs the CPU for the given number of instructions
     * @param num_instructions Number of instructions to run
     */
    void Run(int num_instructions) {
        ExecuteInstructions(num_instructions);
        this->num_instructions += num_instructions;
    }

    /// Step CPU by one instruction
    void Step() {
        Run(1);
    }

    /// Clear all instruction cache
    virtual void ClearInstructionCache() = 0;

    /**
     * Set the Program Counter to an address
     * @param addr Address to set PC to
     */
    virtual void SetPC(u64 addr) = 0;

    /*
     * Get the current Program Counter
     * @return Returns current PC
     */
    virtual u64 GetPC() const = 0;

    /**
     * Get an ARM register
     * @param index Register index
     * @return Returns the value in the register
     */
    virtual u64 GetReg(int index) const = 0;

    /**
     * Set an ARM register
     * @param index Register index
     * @param value Value to set register to
     */
    virtual void SetReg(int index, u64 value) = 0;

    /**
     * Gets the value of a VFP register
     * @param index Register index (0-31)
     * @return Returns the value in the register
     */
    virtual u32 GetVFPReg(int index) const = 0;

    /**
     * Sets a VFP register to the given value
     * @param index Register index (0-31)
     * @param value Value to set register to
     */
    virtual void SetVFPReg(int index, u32 value) = 0;

    /**
     * Gets the current value within a given VFP system register
     * @param reg The VFP system register
     * @return The value within the VFP system register
     */
    virtual u32 GetVFPSystemReg(VFPSystemRegister reg) const = 0;

    /**
     * Sets the VFP system register to the given value
     * @param reg   The VFP system register
     * @param value Value to set the VFP system register to
     */
    virtual void SetVFPSystemReg(VFPSystemRegister reg, u32 value) = 0;

    /**
     * Get the current CPSR register
     * @return Returns the value of the CPSR register
     */
    virtual u32 GetCPSR() const = 0;

    /**
     * Set the current CPSR register
     * @param cpsr Value to set CPSR to
     */
    virtual void SetCPSR(u32 cpsr) = 0;

    /**
     * Gets the value stored in a CP15 register.
     * @param reg The CP15 register to retrieve the value from.
     * @return the value stored in the given CP15 register.
     */
    virtual u32 GetCP15Register(CP15Register reg) = 0;

    /**
     * Stores the given value into the indicated CP15 register.
     * @param reg   The CP15 register to store the value into.
     * @param value The value to store into the CP15 register.
     */
    virtual void SetCP15Register(CP15Register reg, u32 value) = 0;

    virtual VAddr GetTlsAddress() const = 0;

    virtual void SetTlsAddress(VAddr address) = 0;

    /**
     * Advance the CPU core by the specified number of ticks (e.g. to simulate CPU execution time)
     * @param ticks Number of ticks to advance the CPU core
     */
    virtual void AddTicks(u64 ticks) = 0;

    /**
     * Saves the current CPU context
     * @param ctx Thread context to save
     */
    virtual void SaveContext(ThreadContext& ctx) = 0;

    /**
     * Loads a CPU context
     * @param ctx Thread context to load
     */
    virtual void LoadContext(const ThreadContext& ctx) = 0;

    /// Prepare core for thread reschedule (if needed to correctly handle state)
    virtual void PrepareReschedule() = 0;

    /// Getter for num_instructions
    u64 GetNumInstructions() const {
        return num_instructions;
    }

    s64 down_count = 0; ///< A decreasing counter of remaining cycles before the next event,
                        /// decreased by the cpu run loop

protected:
    /**
     * Executes the given number of instructions
     * @param num_instructions Number of instructions to executes
     */
    virtual void ExecuteInstructions(int num_instructions) = 0;

private:
    u64 num_instructions = 0; ///< Number of instructions executed
};
