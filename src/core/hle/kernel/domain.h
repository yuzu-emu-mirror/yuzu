// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <vector>
#include "core/hle/kernel/sync_object.h"
#include "core/hle/result.h"

namespace Kernel {

class Session;
class SessionRequestHandler;

class Domain final : public SyncObject {
public:
    std::string GetTypeName() const override {
        return "Domain";
    }

    static const HandleType HANDLE_TYPE = HandleType::Domain;
    HandleType GetHandleType() const override {
        return HANDLE_TYPE;
    }

    static ResultVal<SharedPtr<Domain>> CreateFromSession(const Session& server);

    ResultCode SendSyncRequest(SharedPtr<Thread> thread) override;

    /// The name of this domain (optional)
    std::string name;

    std::vector<std::shared_ptr<SessionRequestHandler>> request_handlers;

private:
    Domain() = default;
    ~Domain() override = default;

    static ResultVal<SharedPtr<Domain>> Create(std::string name = "Unknown");
};

} // namespace Kernel
