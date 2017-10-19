// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <boost/range/algorithm_ext/erase.hpp>
#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/server_session.h"

namespace Kernel {

void SessionRequestHandler::ClientConnected(SharedPtr<ServerSession> server_session) {
    server_session->SetHleHandler(shared_from_this());
    connected_sessions.push_back(server_session);
}

void SessionRequestHandler::ClientDisconnected(SharedPtr<ServerSession> server_session) {
    server_session->SetHleHandler(nullptr);
    boost::range::remove_erase(connected_sessions, server_session);
}

HLERequestContext::HLERequestContext(SharedPtr<ServerSession> session)
    : session(std::move(session)) {
    cmd_buf[0] = 0;
}

HLERequestContext::~HLERequestContext() = default;

SharedPtr<Object> HLERequestContext::GetIncomingHandle(u32 id_from_cmdbuf) const {
    ASSERT(id_from_cmdbuf < request_handles.size());
    return request_handles[id_from_cmdbuf];
}

u32 HLERequestContext::AddOutgoingHandle(SharedPtr<Object> object) {
    request_handles.push_back(std::move(object));
    return static_cast<u32>(request_handles.size() - 1);
}

void HLERequestContext::ClearIncomingObjects() {
    request_handles.clear();
}

void HLERequestContext::ParseCommandBuffer(u32_le* src_cmdbuf, bool incoming) {
    IPC::RequestParser rp(src_cmdbuf);
    command_header = std::make_unique<IPC::CommandHeader>(rp.PopRaw<IPC::CommandHeader>());

    if (command_header->type == IPC::CommandType::Close) {
        // Close does not populate the rest of the IPC header
        return;
    }

    // If handle descriptor is present, add size of it
    if (command_header->enable_handle_descriptor) {
        handle_descriptor_header =
            std::make_unique<IPC::HandleDescriptorHeader>(rp.PopRaw<IPC::HandleDescriptorHeader>());
        if (handle_descriptor_header->send_current_pid) {
            rp.Skip(2, false);
        }
        rp.Skip(handle_descriptor_header->num_handles_to_copy, false);
        rp.Skip(handle_descriptor_header->num_handles_to_move, false);
    }

    for (int i = 0; i < command_header->num_buf_x_descriptors; ++i) {
        buffer_x_desciptors.push_back(rp.PopRaw<IPC::BufferDescriptorX>());
    }
    for (int i = 0; i < command_header->num_buf_a_descriptors; ++i) {
        buffer_a_desciptors.push_back(rp.PopRaw<IPC::BufferDescriptorABW>());
    }
    for (int i = 0; i < command_header->num_buf_b_descriptors; ++i) {
        buffer_b_desciptors.push_back(rp.PopRaw<IPC::BufferDescriptorABW>());
    }
    for (int i = 0; i < command_header->num_buf_w_descriptors; ++i) {
        buffer_w_desciptors.push_back(rp.PopRaw<IPC::BufferDescriptorABW>());
    }
    if (command_header->buf_c_descriptor_flags !=
        IPC::CommandHeader::BufferDescriptorCFlag::Disabled) {
        UNIMPLEMENTED();
    }

    // Padding to align to 16 bytes
    rp.AlignWithPadding();
    }

    data_payload_header =
        std::make_unique<IPC::DataPayloadHeader>(rp.PopRaw<IPC::DataPayloadHeader>());

    if (incoming) {
        ASSERT(data_payload_header->magic == Common::MakeMagic('S', 'F', 'C', 'I'));
    } else {
        ASSERT(data_payload_header->magic == Common::MakeMagic('S', 'F', 'C', 'O'));
    }

    data_payload_offset = rp.GetCurrentOffset();
    command = rp.Pop<u32_le>();
}

ResultCode HLERequestContext::PopulateFromIncomingCommandBuffer(u32_le* src_cmdbuf,
                                                                Process& src_process,
                                                                HandleTable& src_table) {
    ParseCommandBuffer(src_cmdbuf, true);
    size_t untranslated_size = data_payload_offset + command_header->data_size;
    std::copy_n(src_cmdbuf, untranslated_size, cmd_buf.begin());

    if (command_header->enable_handle_descriptor) {
        if (handle_descriptor_header->num_handles_to_copy ||
            handle_descriptor_header->num_handles_to_move) {
            UNIMPLEMENTED();
        }
    }
    return RESULT_SUCCESS;
}

ResultCode HLERequestContext::WriteToOutgoingCommandBuffer(u32_le* dst_cmdbuf, Process& dst_process,
                                                           HandleTable& dst_table) {
    ParseCommandBuffer(&cmd_buf[0], false);
    size_t untranslated_size = data_payload_offset + command_header->data_size;
    std::copy_n(cmd_buf.begin(), untranslated_size, dst_cmdbuf);

    if (command_header->enable_handle_descriptor) {
        size_t command_size = untranslated_size + handle_descriptor_header->num_handles_to_copy +
                              handle_descriptor_header->num_handles_to_move;
        ASSERT(command_size <= IPC::COMMAND_BUFFER_LENGTH);

        size_t untranslated_index = untranslated_size;
        size_t handle_write_offset = 3;
        while (untranslated_index < command_size) {
            u32 descriptor = cmd_buf[untranslated_index];
            untranslated_index += 1;

            switch (IPC::GetDescriptorType(descriptor)) {
            case IPC::DescriptorType::CopyHandle:
            case IPC::DescriptorType::MoveHandle: {
                // HLE services don't use handles, so we treat both CopyHandle and MoveHandle
                // equally
                u32 num_handles = IPC::HandleNumberFromDesc(descriptor);
                for (u32 j = 0; j < num_handles; ++j) {
                    SharedPtr<Object> object = GetIncomingHandle(cmd_buf[untranslated_index]);
                    Handle handle = 0;
                    if (object != nullptr) {
                        // TODO(yuriks): Figure out the proper error handling for if this fails
                        handle = dst_table.Create(object).Unwrap();
                    }
                    dst_cmdbuf[handle_write_offset++] = handle;
                    untranslated_index++;
                }
                break;
            }
            default:
                UNIMPLEMENTED_MSG("Unsupported handle translation: 0x%08X", descriptor);
            }
        }
    }

    return RESULT_SUCCESS;
}

} // namespace Kernel
