// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>
#include <vector>
#include <boost/container/small_vector.hpp>
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/ipc.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/server_session.h"

namespace Service {
class ServiceFrameworkBase;
}

namespace Kernel {

class Domain;
class HandleTable;
class HLERequestContext;
class Process;

/**
 * Interface implemented by HLE Session handlers.
 * This can be provided to a ServerSession in order to hook into several relevant events
 * (such as a new connection or a SyncRequest) so they can be implemented in the emulator.
 */
class SessionRequestHandler : public std::enable_shared_from_this<SessionRequestHandler> {
public:
    virtual ~SessionRequestHandler() = default;

    /**
     * Handles a sync request from the emulated application.
     * @param server_session The ServerSession that was triggered for this sync request,
     * it should be used to differentiate which client (As in ClientSession) we're answering to.
     * TODO(Subv): Use a wrapper structure to hold all the information relevant to
     * this request (ServerSession, Originator thread, Translated command buffer, etc).
     * @returns ResultCode the result code of the translate operation.
     */
    virtual ResultCode HandleSyncRequest(Kernel::HLERequestContext& context) = 0;

    /**
     * Signals that a client has just connected to this HLE handler and keeps the
     * associated ServerSession alive for the duration of the connection.
     * @param server_session Owning pointer to the ServerSession associated with the connection.
     */
    void ClientConnected(SharedPtr<ServerSession> server_session);

    /**
     * Signals that a client has just disconnected from this HLE handler and releases the
     * associated ServerSession.
     * @param server_session ServerSession associated with the connection.
     */
    void ClientDisconnected(SharedPtr<ServerSession> server_session);

protected:
    /// List of sessions that are connected to this handler.
    /// A ServerSession whose server endpoint is an HLE implementation is kept alive by this list
    // for the duration of the connection.
    std::vector<SharedPtr<ServerSession>> connected_sessions;
};

/**
 * Class containing information about an in-flight IPC request being handled by an HLE service
 * implementation. Services should avoid using old global APIs (e.g. Kernel::GetCommandBuffer()) and
 * when possible use the APIs in this class to service the request.
 *
 * HLE handle protocol
 * ===================
 *
 * To avoid needing HLE services to keep a separate handle table, or having to directly modify the
 * requester's table, a tweaked protocol is used to receive and send handles in requests. The kernel
 * will decode the incoming handles into object pointers and insert a id in the buffer where the
 * handle would normally be. The service then calls GetIncomingHandle() with that id to get the
 * pointer to the object. Similarly, instead of inserting a handle into the command buffer, the
 * service calls AddOutgoingHandle() and stores the returned id where the handle would normally go.
 *
 * The end result is similar to just giving services their own real handle tables, but since these
 * ids are local to a specific context, it avoids requiring services to manage handles for objects
 * across multiple calls and ensuring that unneeded handles are cleaned up.
 */
class HLERequestContext {
public:
    HLERequestContext(SharedPtr<Kernel::Domain> domain);
    HLERequestContext(SharedPtr<Kernel::ServerSession> session);
    ~HLERequestContext();

    /// Returns a pointer to the IPC command buffer for this request.
    u32* CommandBuffer() {
        return cmd_buf.data();
    }

    /**
     * Returns the domain through which this request was made.
     */
    const SharedPtr<Kernel::Domain>& Domain() const {
        return domain;
    }

    /**
     * Returns the session through which this request was made. This can be used as a map key to
     * access per-client data on services.
     */
    const SharedPtr<Kernel::ServerSession>& ServerSession() const {
        return server_session;
    }

    void ParseCommandBuffer(u32_le* src_cmdbuf, bool incoming);

    /// Populates this context with data from the requesting process/thread.
    ResultCode PopulateFromIncomingCommandBuffer(u32_le* src_cmdbuf, Process& src_process,
                                                 HandleTable& src_table);
    /// Writes data from this context back to the requesting process/thread.
    ResultCode WriteToOutgoingCommandBuffer(u32_le* dst_cmdbuf, Process& dst_process,
                                            HandleTable& dst_table);

    u32_le GetCommand() const {
        return command;
    }

    IPC::CommandType GetCommandType() const {
        return command_header->type;
    }

    unsigned GetDataPayloadOffset() const {
        return data_payload_offset;
    }

    const std::vector<IPC::BufferDescriptorX>& BufferDescriptorX() const {
        return buffer_x_desciptors;
    }

    const std::vector<IPC::BufferDescriptorABW>& BufferDescriptorA() const {
        return buffer_a_desciptors;
    }

    const std::vector<IPC::BufferDescriptorABW>& BufferDescriptorB() const {
        return buffer_b_desciptors;
    }

    const std::unique_ptr<IPC::DomainMessageHeader>& GetDomainMessageHeader() const {
        return domain_message_header;
    }

    bool IsDomain() const {
        return domain != nullptr;
    }

    template<typename T>
    SharedPtr<T> GetCopyObject(size_t index) {
        ASSERT(index < copy_objects.size());
        return DynamicObjectCast<T>(copy_objects[index]);
    }

    template<typename T>
    SharedPtr<T> GetMoveObject(size_t index) {
        ASSERT(index < move_objects.size());
        return DynamicObjectCast<T>(move_objects[index]);
    }

    void AddMoveObject(SharedPtr<Object> object) {
        move_objects.emplace_back(std::move(object));
    }

    void AddCopyObject(SharedPtr<Object> object) {
        copy_objects.emplace_back(std::move(object));
    }

    void AddDomainObject(std::shared_ptr<SessionRequestHandler> object) {
        domain_objects.emplace_back(std::move(object));
    }

private:
    std::array<u32, IPC::COMMAND_BUFFER_LENGTH> cmd_buf;
    SharedPtr<Kernel::Domain> domain;
    SharedPtr<Kernel::ServerSession> server_session;
    // TODO(yuriks): Check common usage of this and optimize size accordingly
    boost::container::small_vector<SharedPtr<Object>, 8> move_objects;
    boost::container::small_vector<SharedPtr<Object>, 8> copy_objects;
    boost::container::small_vector<std::shared_ptr<SessionRequestHandler>, 8> domain_objects;

    std::unique_ptr<IPC::CommandHeader> command_header;
    std::unique_ptr<IPC::HandleDescriptorHeader> handle_descriptor_header;
    std::unique_ptr<IPC::DataPayloadHeader> data_payload_header;
    std::unique_ptr<IPC::DomainMessageHeader> domain_message_header;
    std::vector<IPC::BufferDescriptorX> buffer_x_desciptors;
    std::vector<IPC::BufferDescriptorABW> buffer_a_desciptors;
    std::vector<IPC::BufferDescriptorABW> buffer_b_desciptors;
    std::vector<IPC::BufferDescriptorABW> buffer_w_desciptors;

    unsigned data_payload_offset{};
    u32_le command{};
};

} // namespace Kernel
