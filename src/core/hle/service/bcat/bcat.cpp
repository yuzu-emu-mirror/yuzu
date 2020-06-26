// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/bcat/bcat.h"

namespace Service::BCAT {

// 2.0.0+
BCAT::BCAT(Core::System& system, std::shared_ptr<Module> module,
           FileSystem::FileSystemController& fsc, const char* name)
    : Interface(system, std::move(module), fsc, name) {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &BCAT::CreateBcatService, "CreateBcatService"},
        {1, &BCAT::CreateDeliveryCacheStorageService, "CreateDeliveryCacheStorageService"},
        {2, &BCAT::CreateDeliveryCacheStorageServiceWithApplicationId, "CreateDeliveryCacheStorageServiceWithApplicationId"},
        {3, nullptr, "CreateDeliveryCacheProgressService"}, // 2.0.0 - 2.3.0
        {4, nullptr, "CreateDeliveryCacheProgressServiceWithApplicationId"}, // 2.0.0 - 2.3.0
    };
    // clang-format on
    RegisterHandlers(functions);
}

BCAT::~BCAT() = default;
} // namespace Service::BCAT
