// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/kernel/errors.h"
#include "core/memory.h"

namespace IPC {

/// Size of the command buffer area, in 32-bit words.
constexpr size_t COMMAND_BUFFER_LENGTH = 0x100 / sizeof(u32);

// These errors are commonly returned by invalid IPC translations, so alias them here for
// convenience.
// TODO(yuriks): These will probably go away once translation is implemented inside the kernel.
constexpr auto ERR_INVALID_HANDLE = Kernel::ERR_INVALID_HANDLE_OS;


enum class ControlCommand : u32 {
    ConvertSessionToDomain = 0,
    ConvertDomainToSession = 1,
    DuplicateSession = 2,
    QueryPointerBufferSize = 3,
    DuplicateSessionEx = 4,
    Unspecified,
};

enum class CommandType : u32 {
    Close = 2,
    Request = 4,
    Control = 5,
    Unspecified,
};

struct CommandHeader {
    union {
        u32_le raw_low;
        BitField<0, 16, CommandType> type;
        BitField<16, 4, u32_le> num_buf_x_descriptors;
        BitField<20, 4, u32_le> num_buf_a_descriptors;
        BitField<24, 4, u32_le> num_buf_b_descriptors;
        BitField<28, 4, u32_le> num_buf_w_descriptors;
    };

    enum class BufferDescriptorCFlag : u32 {
        Disabled = 0,
        NoDescriptor = 1,
        TwoDesciptors = 2,
    };

    union {
        u32_le raw_high;
        BitField<0, 10, u32_le> data_size;
        BitField<10, 4, BufferDescriptorCFlag> buf_c_descriptor_flags;
        BitField<31, 1, u32_le> enable_handle_descriptor;
    };
};
static_assert(sizeof(CommandHeader) == 8, "CommandHeader size is incorrect");

union HandleDescriptorHeader {
    u32_le raw_high;
    BitField<0, 1, u32_le> send_current_pid;
    BitField<1, 4, u32_le> num_handles_to_copy;
    BitField<5, 4, u32_le> num_handles_to_move;
};
static_assert(sizeof(HandleDescriptorHeader) == 4, "HandleDescriptorHeader size is incorrect");

struct BufferDescriptorX {
    union {
        BitField<0, 6, u32_le> counter_bits_0_5;
        BitField<6, 3, u32_le> address_bits_36_38;
        BitField<9, 3, u32_le> counter_bits_9_11;
        BitField<12, 4, u32_le> address_bits_32_35;
        BitField<16, 16, u32_le> size;
    };

    u32_le address_bits_0_31;

    u32_le Counter() const {
        u32_le counter{ counter_bits_0_5 };
        counter |= counter_bits_9_11 << 9;
        return counter;
    }

    VAddr Address() const {
        VAddr address{ address_bits_0_31 };
        address |= static_cast<VAddr>(address_bits_32_35) << 32;
        address |= static_cast<VAddr>(address_bits_36_38) << 36;
        return address;
    }
};
static_assert(sizeof(BufferDescriptorX) == 8, "BufferDescriptorX size is incorrect");

struct BufferDescriptorABW {
    u32_le size_bits_0_31;
    u32_le address_bits_0_31;

    union {
        BitField<0, 2, u32_le> flags;
        BitField<2, 3, u32_le> address_bits_36_38;
        BitField<24, 4, u32_le> size_bits_32_35;
        BitField<28, 4, u32_le> address_bits_32_35;
    };

    VAddr Address() const {
        VAddr address{ address_bits_0_31 };
        address |= static_cast<VAddr>(address_bits_32_35) << 32;
        address |= static_cast<VAddr>(address_bits_36_38) << 36;
        return address;
    }

    u64 Size() const {
        u64 size{ size_bits_0_31 };
        size |= static_cast<u64>(size_bits_32_35) << 32;
        return size;
    }
};
static_assert(sizeof(BufferDescriptorABW) == 12, "BufferDescriptorABW size is incorrect");

struct BufferDescriptorC {
    u32_le address_bits_0_31;

    union {
        BitField<0, 16, u32_le> address_bits_32_47;
        BitField<16, 16, u32_le> size;
    };

    VAddr Address() const {
        VAddr address{ address_bits_0_31 };
        address |= static_cast<VAddr>(address_bits_32_47) << 32;
        return address;
    }
};
static_assert(sizeof(BufferDescriptorC) == 8, "BufferDescriptorC size is incorrect");

struct DataPayloadHeader {
    u32_le magic;
    INSERT_PADDING_WORDS(1);
};
static_assert(sizeof(DataPayloadHeader) == 8, "DataPayloadRequest size is incorrect");

struct DomainMessageHeader {
    union {
        BitField<0, 8, u32_le> command;
        BitField<16, 16, u32_le> size;
    };
    u32_le object_id;
    INSERT_PADDING_WORDS(2);
};
static_assert(sizeof(DomainMessageHeader) == 16, "DomainMessageHeader size is incorrect");

enum DescriptorType : u32 {
    // Buffer related desciptors types (mask : 0x0F)
    StaticBuffer = 0x02,
    PXIBuffer = 0x04,
    MappedBuffer = 0x08,
    // Handle related descriptors types (mask : 0x30, but need to check for buffer related
    // descriptors first )
    CopyHandle = 0x00,
    MoveHandle = 0x10,
    CallingPid = 0x20,
};

constexpr u32 MoveHandleDesc(u32 num_handles = 1) {
    return MoveHandle | ((num_handles - 1) << 26);
}

constexpr u32 CopyHandleDesc(u32 num_handles = 1) {
    return CopyHandle | ((num_handles - 1) << 26);
}

constexpr u32 CallingPidDesc() {
    return CallingPid;
}

constexpr bool IsHandleDescriptor(u32 descriptor) {
    return (descriptor & 0xF) == 0x0;
}

constexpr u32 HandleNumberFromDesc(u32 handle_descriptor) {
    return (handle_descriptor >> 26) + 1;
}

union StaticBufferDescInfo {
    u32 raw;
    BitField<0, 4, u32> descriptor_type;
    BitField<10, 4, u32> buffer_id;
    BitField<14, 18, u32> size;
};

inline u32 StaticBufferDesc(size_t size, u8 buffer_id) {
    StaticBufferDescInfo info{};
    info.descriptor_type.Assign(StaticBuffer);
    info.buffer_id.Assign(buffer_id);
    info.size.Assign(static_cast<u32>(size));
    return info.raw;
}

/**
 * @brief Creates a header describing a buffer to be sent over PXI.
 * @param size         Size of the buffer. Max 0x00FFFFFF.
 * @param buffer_id    The Id of the buffer. Max 0xF.
 * @param is_read_only true if the buffer is read-only. If false, the buffer is considered to have
 * read-write access.
 * @return The created PXI buffer header.
 *
 * The next value is a phys-address of a table located in the BASE memregion.
 */
inline u32 PXIBufferDesc(u32 size, unsigned buffer_id, bool is_read_only) {
    u32 type = PXIBuffer;
    if (is_read_only)
        type |= 0x2;
    return type | (size << 8) | ((buffer_id & 0xF) << 4);
}

enum MappedBufferPermissions : u32 {
    R = 1,
    W = 2,
    RW = R | W,
};

union MappedBufferDescInfo {
    u32 raw;
    BitField<0, 4, u32> flags;
    BitField<1, 2, MappedBufferPermissions> perms;
    BitField<4, 28, u32> size;
};

inline u32 MappedBufferDesc(size_t size, MappedBufferPermissions perms) {
    MappedBufferDescInfo info{};
    info.flags.Assign(MappedBuffer);
    info.perms.Assign(perms);
    info.size.Assign(static_cast<u32>(size));
    return info.raw;
}

inline DescriptorType GetDescriptorType(u32 descriptor) {
    // Note: Those checks must be done in this order
    if (IsHandleDescriptor(descriptor))
        return (DescriptorType)(descriptor & 0x30);

    // handle the fact that the following descriptors can have rights
    if (descriptor & MappedBuffer)
        return MappedBuffer;

    if (descriptor & PXIBuffer)
        return PXIBuffer;

    return StaticBuffer;
}

} // namespace IPC
