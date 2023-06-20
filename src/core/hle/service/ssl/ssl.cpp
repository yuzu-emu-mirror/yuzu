// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/string_util.h"

#include "core/core.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"
#include "core/hle/service/sockets/bsd.h"
#include "core/hle/service/ssl/ssl.h"
#include "core/hle/service/ssl/ssl_backend.h"
#include "core/internal_network/network.h"
#include "core/internal_network/sockets.h"

namespace Service::SSL {

// This is nn::ssl::sf::CertificateFormat
enum class CertificateFormat : u32 {
    Pem = 1,
    Der = 2,
};

// This is nn::ssl::sf::ContextOption
enum class ContextOption : u32 {
    None = 0,
    CrlImportDateCheckEnable = 1,
};

// This is nn::ssl::Connection::IoMode
enum class IoMode : u32 {
    Blocking = 1,
    NonBlocking = 2,
};

// This is nn::ssl::sf::OptionType
enum class OptionType : u32 {
    DoNotCloseSocket = 0,
    GetServerCertChain = 1,
};

// This is nn::ssl::sf::SslVersion
struct SslVersion {
    union {
        u32 raw{};

        BitField<0, 1, u32> tls_auto;
        BitField<3, 1, u32> tls_v10;
        BitField<4, 1, u32> tls_v11;
        BitField<5, 1, u32> tls_v12;
        BitField<6, 1, u32> tls_v13;
        BitField<24, 7, u32> api_version;
    };
};

struct SslContextSharedData {
    u32 connection_count = 0;
};

class ISslConnection final : public ServiceFramework<ISslConnection> {
public:
    explicit ISslConnection(Core::System& system_, SslVersion version,
                            std::shared_ptr<SslContextSharedData>& shared_data,
                            std::unique_ptr<SSLConnectionBackend>&& backend)
        : ServiceFramework{system_, "ISslConnection"}, ssl_version{version},
          shared_data_{shared_data}, backend_{std::move(backend)} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &ISslConnection::SetSocketDescriptor, "SetSocketDescriptor"},
            {1, &ISslConnection::SetHostName, "SetHostName"},
            {2, &ISslConnection::SetVerifyOption, "SetVerifyOption"},
            {3, &ISslConnection::SetIoMode, "SetIoMode"},
            {4, nullptr, "GetSocketDescriptor"},
            {5, nullptr, "GetHostName"},
            {6, nullptr, "GetVerifyOption"},
            {7, nullptr, "GetIoMode"},
            {8, &ISslConnection::DoHandshake, "DoHandshake"},
            {9, &ISslConnection::DoHandshakeGetServerCert, "DoHandshakeGetServerCert"},
            {10, &ISslConnection::Read, "Read"},
            {11, &ISslConnection::Write, "Write"},
            {12, &ISslConnection::Pending, "Pending"},
            {13, nullptr, "Peek"},
            {14, nullptr, "Poll"},
            {15, nullptr, "GetVerifyCertError"},
            {16, nullptr, "GetNeededServerCertBufferSize"},
            {17, &ISslConnection::SetSessionCacheMode, "SetSessionCacheMode"},
            {18, nullptr, "GetSessionCacheMode"},
            {19, nullptr, "FlushSessionCache"},
            {20, nullptr, "SetRenegotiationMode"},
            {21, nullptr, "GetRenegotiationMode"},
            {22, &ISslConnection::SetOption, "SetOption"},
            {23, nullptr, "GetOption"},
            {24, nullptr, "GetVerifyCertErrors"},
            {25, nullptr, "GetCipherInfo"},
            {26, nullptr, "SetNextAlpnProto"},
            {27, nullptr, "GetNextAlpnProto"},
            {28, nullptr, "SetDtlsSocketDescriptor"},
            {29, nullptr, "GetDtlsHandshakeTimeout"},
            {30, nullptr, "SetPrivateOption"},
            {31, nullptr, "SetSrtpCiphers"},
            {32, nullptr, "GetSrtpCipher"},
            {33, nullptr, "ExportKeyingMaterial"},
            {34, nullptr, "SetIoTimeout"},
            {35, nullptr, "GetIoTimeout"},
        };
        // clang-format on

        RegisterHandlers(functions);

        shared_data->connection_count++;
    }

    ~ISslConnection() {
        shared_data_->connection_count--;
        if (fd_to_close_.has_value()) {
            s32 fd = *fd_to_close_;
            if (!do_not_close_socket_) {
                LOG_ERROR(Service_SSL,
                          "do_not_close_socket was changed after setting socket; is this right?");
            } else {
                auto bsd = system.ServiceManager().GetService<Service::Sockets::BSD>("bsd:u");
                if (bsd) {
                    auto err = bsd->CloseImpl(fd);
                    if (err != Service::Sockets::Errno::SUCCESS) {
                        LOG_ERROR(Service_SSL, "failed to close duplicated socket: {}", err);
                    }
                }
            }
        }
    }

private:
    SslVersion ssl_version;
    std::shared_ptr<SslContextSharedData> shared_data_;
    std::unique_ptr<SSLConnectionBackend> backend_;
    std::optional<int> fd_to_close_;
    bool do_not_close_socket_ = false;
    bool get_server_cert_chain_ = false;
    std::shared_ptr<Network::SocketBase> socket_;
    bool did_set_host_name_ = false;
    bool did_handshake_ = false;

    ResultVal<s32> SetSocketDescriptorImpl(s32 fd) {
        LOG_DEBUG(Service_SSL, "called, fd={}", fd);
        ASSERT(!did_handshake_);
        auto bsd = system.ServiceManager().GetService<Service::Sockets::BSD>("bsd:u");
        ASSERT_OR_EXECUTE(bsd, { return ResultInternalError; });
        s32 ret_fd;
        // Based on https://switchbrew.org/wiki/SSL_services#SetSocketDescriptor
        if (do_not_close_socket_) {
            auto res = bsd->DuplicateSocketImpl(fd);
            if (!res.has_value()) {
                LOG_ERROR(Service_SSL, "failed to duplicate socket");
                return ResultInvalidSocket;
            }
            fd = *res;
            fd_to_close_ = fd;
            ret_fd = fd;
        } else {
            ret_fd = -1;
        }
        std::optional<std::shared_ptr<Network::SocketBase>> sock = bsd->GetSocket(fd);
        if (!sock.has_value()) {
            LOG_ERROR(Service_SSL, "invalid socket fd {}", fd);
            return ResultInvalidSocket;
        }
        socket_ = std::move(*sock);
        backend_->SetSocket(socket_);
        return ret_fd;
    }

    Result SetHostNameImpl(const std::string& hostname) {
        LOG_DEBUG(Service_SSL, "SetHostNameImpl({})", hostname);
        ASSERT(!did_handshake_);
        Result res = backend_->SetHostName(hostname);
        if (res == ResultSuccess) {
            did_set_host_name_ = true;
        }
        return res;
    }

    Result SetVerifyOptionImpl(u32 option) {
        ASSERT(!did_handshake_);
        LOG_WARNING(Service_SSL, "(STUBBED) called. option={}", option);
        return ResultSuccess;
    }

    Result SetIOModeImpl(u32 _mode) {
        auto mode = static_cast<IoMode>(_mode);
        ASSERT(mode == IoMode::Blocking || mode == IoMode::NonBlocking);
        ASSERT_OR_EXECUTE(socket_, { return ResultNoSocket; });

        bool non_block = mode == IoMode::NonBlocking;
        Network::Errno e = socket_->SetNonBlock(non_block);
        if (e != Network::Errno::SUCCESS) {
            LOG_ERROR(Service_SSL, "Failed to set native socket non-block flag to {}", non_block);
        }
        return ResultSuccess;
    }

    Result SetSessionCacheModeImpl(u32 mode) {
        ASSERT(!did_handshake_);
        LOG_WARNING(Service_SSL, "(STUBBED) called. value={}", mode);
        return ResultSuccess;
    }

    Result DoHandshakeImpl() {
        ASSERT_OR_EXECUTE(!did_handshake_ && socket_, { return ResultNoSocket; });
        ASSERT_OR_EXECUTE_MSG(
            did_set_host_name_, { return ResultInternalError; },
            "Expected SetHostName before DoHandshake");
        Result res = backend_->DoHandshake();
        did_handshake_ = res.IsSuccess();
        return res;
    }

    std::vector<u8> SerializeServerCerts(const std::vector<std::vector<u8>>& certs) {
        struct Header {
            u64 magic;
            u32 count;
            u32 pad;
        };
        struct EntryHeader {
            u32 size;
            u32 offset;
        };
        if (!get_server_cert_chain_) {
            // Just return the first one, unencoded.
            ASSERT_OR_EXECUTE_MSG(
                !certs.empty(), { return {}; }, "Should be at least one server cert");
            return certs[0];
        }
        std::vector<u8> ret;
        Header header{0x4E4D684374726543, static_cast<u32>(certs.size()), 0};
        ret.insert(ret.end(), reinterpret_cast<u8*>(&header), reinterpret_cast<u8*>(&header + 1));
        size_t data_offset = sizeof(Header) + certs.size() * sizeof(EntryHeader);
        for (auto& cert : certs) {
            EntryHeader entry_header{static_cast<u32>(cert.size()), static_cast<u32>(data_offset)};
            data_offset += cert.size();
            ret.insert(ret.end(), reinterpret_cast<u8*>(&entry_header),
                       reinterpret_cast<u8*>(&entry_header + 1));
        }
        for (auto& cert : certs) {
            ret.insert(ret.end(), cert.begin(), cert.end());
        }
        return ret;
    }

    ResultVal<std::vector<u8>> ReadImpl(size_t size) {
        ASSERT_OR_EXECUTE(did_handshake_, { return ResultInternalError; });
        std::vector<u8> res(size);
        ResultVal<size_t> actual = backend_->Read(res);
        if (actual.Failed()) {
            return actual.Code();
        }
        res.resize(*actual);
        return res;
    }

    ResultVal<size_t> WriteImpl(std::span<const u8> data) {
        ASSERT_OR_EXECUTE(did_handshake_, { return ResultInternalError; });
        return backend_->Write(data);
    }

    ResultVal<s32> PendingImpl() {
        LOG_WARNING(Service_SSL, "(STUBBED) called.");
        return 0;
    }

    void SetSocketDescriptor(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const s32 fd = rp.Pop<s32>();
        const ResultVal<s32> res = SetSocketDescriptorImpl(fd);
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(res.Code());
        rb.Push<s32>(res.ValueOr(-1));
    }

    void SetHostName(HLERequestContext& ctx) {
        const std::string hostname = Common::StringFromBuffer(ctx.ReadBuffer());
        const Result res = SetHostNameImpl(hostname);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res);
    }

    void SetVerifyOption(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u32 option = rp.Pop<u32>();
        const Result res = SetVerifyOptionImpl(option);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res);
    }

    void SetIoMode(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u32 mode = rp.Pop<u32>();
        const Result res = SetIOModeImpl(mode);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res);
    }

    void DoHandshake(HLERequestContext& ctx) {
        const Result res = DoHandshakeImpl();
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res);
    }

    void DoHandshakeGetServerCert(HLERequestContext& ctx) {
        Result res = DoHandshakeImpl();
        u32 certs_count = 0;
        u32 certs_size = 0;
        if (res == ResultSuccess) {
            auto certs = backend_->GetServerCerts();
            if (certs.Succeeded()) {
                std::vector<u8> certs_buf = SerializeServerCerts(*certs);
                ctx.WriteBuffer(certs_buf);
                certs_count = static_cast<u32>(certs->size());
                certs_size = static_cast<u32>(certs_buf.size());
            }
        }
        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(res);
        rb.Push(certs_size);
        rb.Push(certs_count);
    }

    void Read(HLERequestContext& ctx) {
        const ResultVal<std::vector<u8>> res = ReadImpl(ctx.GetWriteBufferSize());
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(res.Code());
        if (res.Succeeded()) {
            rb.Push(static_cast<u32>(res->size()));
            ctx.WriteBuffer(*res);
        } else {
            rb.Push(static_cast<u32>(0));
        }
    }

    void Write(HLERequestContext& ctx) {
        const ResultVal<size_t> res = WriteImpl(ctx.ReadBuffer());
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(res.Code());
        rb.Push(static_cast<u32>(res.ValueOr(0)));
    }

    void Pending(HLERequestContext& ctx) {
        const ResultVal<s32> res = PendingImpl();
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(res.Code());
        rb.Push<s32>(res.ValueOr(0));
    }

    void SetSessionCacheMode(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u32 mode = rp.Pop<u32>();
        const Result res = SetSessionCacheModeImpl(mode);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res);
    }

    void SetOption(HLERequestContext& ctx) {
        struct Parameters {
            OptionType option;
            s32 value;
        };
        static_assert(sizeof(Parameters) == 0x8, "Parameters is an invalid size");

        IPC::RequestParser rp{ctx};
        const auto parameters = rp.PopRaw<Parameters>();

        switch (parameters.option) {
        case OptionType::DoNotCloseSocket:
            do_not_close_socket_ = static_cast<bool>(parameters.value);
            break;
        case OptionType::GetServerCertChain:
            get_server_cert_chain_ = static_cast<bool>(parameters.value);
            break;
        default:
            LOG_WARNING(Service_SSL, "unrecognized option={}, value={}", parameters.option,
                        parameters.value);
        }

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }
};

class ISslContext final : public ServiceFramework<ISslContext> {
public:
    explicit ISslContext(Core::System& system_, SslVersion version)
        : ServiceFramework{system_, "ISslContext"}, ssl_version{version},
          shared_data_{std::make_shared<SslContextSharedData>()} {
        static const FunctionInfo functions[] = {
            {0, &ISslContext::SetOption, "SetOption"},
            {1, nullptr, "GetOption"},
            {2, &ISslContext::CreateConnection, "CreateConnection"},
            {3, &ISslContext::GetConnectionCount, "GetConnectionCount"},
            {4, &ISslContext::ImportServerPki, "ImportServerPki"},
            {5, &ISslContext::ImportClientPki, "ImportClientPki"},
            {6, nullptr, "RemoveServerPki"},
            {7, nullptr, "RemoveClientPki"},
            {8, nullptr, "RegisterInternalPki"},
            {9, nullptr, "AddPolicyOid"},
            {10, nullptr, "ImportCrl"},
            {11, nullptr, "RemoveCrl"},
            {12, nullptr, "ImportClientCertKeyPki"},
            {13, nullptr, "GeneratePrivateKeyAndCert"},
        };
        RegisterHandlers(functions);
    }

private:
    SslVersion ssl_version;
    std::shared_ptr<SslContextSharedData> shared_data_;

    void SetOption(HLERequestContext& ctx) {
        struct Parameters {
            ContextOption option;
            s32 value;
        };
        static_assert(sizeof(Parameters) == 0x8, "Parameters is an invalid size");

        IPC::RequestParser rp{ctx};
        const auto parameters = rp.PopRaw<Parameters>();

        LOG_WARNING(Service_SSL, "(STUBBED) called. option={}, value={}", parameters.option,
                    parameters.value);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void CreateConnection(HLERequestContext& ctx) {
        LOG_WARNING(Service_SSL, "called");

        auto backend_res = CreateSSLConnectionBackend();

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(backend_res.Code());
        if (backend_res.Succeeded()) {
            rb.PushIpcInterface<ISslConnection>(system, ssl_version, shared_data_,
                                                std::move(*backend_res));
        }
    }

    void GetConnectionCount(HLERequestContext& ctx) {
        LOG_WARNING(Service_SSL, "connection_count={}", shared_data_->connection_count);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(shared_data_->connection_count);
    }

    void ImportServerPki(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto certificate_format = rp.PopEnum<CertificateFormat>();
        [[maybe_unused]] const auto pkcs_12_certificates = ctx.ReadBuffer(0);

        constexpr u64 server_id = 0;

        LOG_WARNING(Service_SSL, "(STUBBED) called, certificate_format={}", certificate_format);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push(server_id);
    }

    void ImportClientPki(HLERequestContext& ctx) {
        [[maybe_unused]] const auto pkcs_12_certificate = ctx.ReadBuffer(0);
        [[maybe_unused]] const auto ascii_password = [&ctx] {
            if (ctx.CanReadBuffer(1)) {
                return ctx.ReadBuffer(1);
            }

            return std::span<const u8>{};
        }();

        constexpr u64 client_id = 0;

        LOG_WARNING(Service_SSL, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push(client_id);
    }
};

class ISslService final : public ServiceFramework<ISslService> {
public:
    explicit ISslService(Core::System& system_) : ServiceFramework{system_, "ssl"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &ISslService::CreateContext, "CreateContext"},
            {1, nullptr, "GetContextCount"},
            {2, nullptr, "GetCertificates"},
            {3, nullptr, "GetCertificateBufSize"},
            {4, nullptr, "DebugIoctl"},
            {5, &ISslService::SetInterfaceVersion, "SetInterfaceVersion"},
            {6, nullptr, "FlushSessionCache"},
            {7, nullptr, "SetDebugOption"},
            {8, nullptr, "GetDebugOption"},
            {8, nullptr, "ClearTls12FallbackFlag"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void CreateContext(HLERequestContext& ctx) {
        struct Parameters {
            SslVersion ssl_version;
            INSERT_PADDING_BYTES(0x4);
            u64 pid_placeholder;
        };
        static_assert(sizeof(Parameters) == 0x10, "Parameters is an invalid size");

        IPC::RequestParser rp{ctx};
        const auto parameters = rp.PopRaw<Parameters>();

        LOG_WARNING(Service_SSL, "(STUBBED) called, api_version={}, pid_placeholder={}",
                    parameters.ssl_version.api_version, parameters.pid_placeholder);

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<ISslContext>(system, parameters.ssl_version);
    }

    void SetInterfaceVersion(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        u32 ssl_version = rp.Pop<u32>();

        LOG_DEBUG(Service_SSL, "called, ssl_version={}", ssl_version);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("ssl", std::make_shared<ISslService>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::SSL
