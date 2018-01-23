// Copyright 2018 yuzu emulator team
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

HLERequestContext::HLERequestContext(SharedPtr<Kernel::Domain> domain) : domain(std::move(domain)) {
    cmd_buf[0] = 0;
}

HLERequestContext::HLERequestContext(SharedPtr<Kernel::ServerSession> server_session)
    : server_session(std::move(server_session)) {
    cmd_buf[0] = 0;
}

HLERequestContext::~HLERequestContext() = default;

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
        if (incoming) {
            // Populate the object lists with the data in the IPC request.
            for (u32 handle = 0; handle < handle_descriptor_header->num_handles_to_copy; ++handle) {
                copy_objects.push_back(Kernel::g_handle_table.GetGeneric(rp.Pop<Handle>()));
            }
            for (u32 handle = 0; handle < handle_descriptor_header->num_handles_to_move; ++handle) {
                move_objects.push_back(Kernel::g_handle_table.GetGeneric(rp.Pop<Handle>()));
            }
        } else {
            // For responses we just ignore the handles, they're empty and will be populated when
            // translating the response.
            rp.Skip(handle_descriptor_header->num_handles_to_copy, false);
            rp.Skip(handle_descriptor_header->num_handles_to_move, false);
        }
    }

    for (unsigned i = 0; i < command_header->num_buf_x_descriptors; ++i) {
        buffer_x_desciptors.push_back(rp.PopRaw<IPC::BufferDescriptorX>());
    }
    for (unsigned i = 0; i < command_header->num_buf_a_descriptors; ++i) {
        buffer_a_desciptors.push_back(rp.PopRaw<IPC::BufferDescriptorABW>());
    }
    for (unsigned i = 0; i < command_header->num_buf_b_descriptors; ++i) {
        buffer_b_desciptors.push_back(rp.PopRaw<IPC::BufferDescriptorABW>());
    }
    for (unsigned i = 0; i < command_header->num_buf_w_descriptors; ++i) {
        buffer_w_desciptors.push_back(rp.PopRaw<IPC::BufferDescriptorABW>());
    }

    buffer_c_offset = rp.GetCurrentOffset() + command_header->data_size;

    // Padding to align to 16 bytes
    rp.AlignWithPadding();

    if (IsDomain() && (command_header->type == IPC::CommandType::Request || !incoming)) {
        // If this is an incoming message, only CommandType "Request" has a domain header
        // All outgoing domain messages have the domain header
        domain_message_header =
            std::make_unique<IPC::DomainMessageHeader>(rp.PopRaw<IPC::DomainMessageHeader>());
    }

    data_payload_header =
        std::make_unique<IPC::DataPayloadHeader>(rp.PopRaw<IPC::DataPayloadHeader>());

    data_payload_offset = rp.GetCurrentOffset();

    if (domain_message_header && domain_message_header->command ==
                                     IPC::DomainMessageHeader::CommandType::CloseVirtualHandle) {
        // CloseVirtualHandle command does not have SFC* or any data
        return;
    }

    if (incoming) {
        ASSERT(data_payload_header->magic == Common::MakeMagic('S', 'F', 'C', 'I'));
    } else {
        ASSERT(data_payload_header->magic == Common::MakeMagic('S', 'F', 'C', 'O'));
    }

    rp.SetCurrentOffset(buffer_c_offset);

    // For Inline buffers, the response data is written directly to buffer_c_offset
    // and in this case we don't have any BufferDescriptorC on the request.
    if (command_header->buf_c_descriptor_flags >
        IPC::CommandHeader::BufferDescriptorCFlag::InlineDescriptor) {
        if (command_header->buf_c_descriptor_flags ==
            IPC::CommandHeader::BufferDescriptorCFlag::OneDescriptor) {
            buffer_c_desciptors.push_back(rp.PopRaw<IPC::BufferDescriptorC>());
        } else {
            unsigned num_buf_c_descriptors =
                static_cast<unsigned>(command_header->buf_c_descriptor_flags.Value()) - 2;

            // This is used to detect possible underflows, in case something is broken
            // with the two ifs above and the flags value is == 0 || == 1.
            ASSERT(num_buf_c_descriptors < 14);

            for (unsigned i = 0; i < num_buf_c_descriptors; ++i) {
                buffer_c_desciptors.push_back(rp.PopRaw<IPC::BufferDescriptorC>());
            }
        }
    }

    rp.SetCurrentOffset(data_payload_offset);

    command = rp.Pop<u32_le>();
    rp.Skip(1, false); // The command is actually an u64, but we don't use the high part.
}

ResultCode HLERequestContext::PopulateFromIncomingCommandBuffer(u32_le* src_cmdbuf,
                                                                Process& src_process,
                                                                HandleTable& src_table) {
    ParseCommandBuffer(src_cmdbuf, true);
    if (command_header->type == IPC::CommandType::Close) {
        // Close does not populate the rest of the IPC header
        return RESULT_SUCCESS;
    }

    // The data_size already includes the payload header, the padding and the domain header.
    size_t size = data_payload_offset + command_header->data_size -
                  sizeof(IPC::DataPayloadHeader) / sizeof(u32) - 4;
    if (domain_message_header)
        size -= sizeof(IPC::DomainMessageHeader) / sizeof(u32);
    std::copy_n(src_cmdbuf, size, cmd_buf.begin());
    return RESULT_SUCCESS;
}

ResultCode HLERequestContext::WriteToOutgoingCommandBuffer(u32_le* dst_cmdbuf, Process& dst_process,
                                                           HandleTable& dst_table) {
    // The header was already built in the internal command buffer. Attempt to parse it to verify
    // the integrity and then copy it over to the target command buffer.
    ParseCommandBuffer(cmd_buf.data(), false);

    // The data_size already includes the payload header, the padding and the domain header.
    size_t size = data_payload_offset + command_header->data_size -
                  sizeof(IPC::DataPayloadHeader) / sizeof(u32) - 4;
    if (domain_message_header)
        size -= sizeof(IPC::DomainMessageHeader) / sizeof(u32);

    std::copy_n(cmd_buf.begin(), size, dst_cmdbuf);

    if (command_header->enable_handle_descriptor) {
        ASSERT_MSG(!move_objects.empty() || !copy_objects.empty(),
                   "Handle descriptor bit set but no handles to translate");
        // We write the translated handles at a specific offset in the command buffer, this space
        // was already reserved when writing the header.
        size_t current_offset =
            (sizeof(IPC::CommandHeader) + sizeof(IPC::HandleDescriptorHeader)) / sizeof(u32);
        ASSERT_MSG(!handle_descriptor_header->send_current_pid, "Sending PID is not implemented");

        ASSERT_MSG(copy_objects.size() == handle_descriptor_header->num_handles_to_copy);
        ASSERT_MSG(move_objects.size() == handle_descriptor_header->num_handles_to_move);

        // We don't make a distinction between copy and move handles when translating since HLE
        // services don't deal with handles directly. However, the guest applications might check
        // for specific values in each of these descriptors.
        for (auto& object : copy_objects) {
            ASSERT(object != nullptr);
            dst_cmdbuf[current_offset++] = Kernel::g_handle_table.Create(object).Unwrap();
        }

        for (auto& object : move_objects) {
            ASSERT(object != nullptr);
            dst_cmdbuf[current_offset++] = Kernel::g_handle_table.Create(object).Unwrap();
        }
    }

    // TODO(Subv): Translate the X/A/B/W buffers.

    if (IsDomain()) {
        ASSERT(domain_message_header->num_objects == domain_objects.size());
        // Write the domain objects to the command buffer, these go after the raw untranslated data.
        // TODO(Subv): This completely ignores C buffers.
        size_t domain_offset = size - domain_message_header->num_objects;
        auto& request_handlers = domain->request_handlers;

        for (auto& object : domain_objects) {
            request_handlers.emplace_back(object);
            dst_cmdbuf[domain_offset++] = static_cast<u32_le>(request_handlers.size());
        }
    }
    return RESULT_SUCCESS;
}

} // namespace Kernel
