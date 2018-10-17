// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "core/hle/kernel/event.h"
#include "core/hle/service/service.h"

namespace Service::NFP {

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(std::shared_ptr<Module> module, const char* name);
        ~Interface() override;

        void CreateUserInterface(Kernel::HLERequestContext& ctx);
        void LoadAmiibo(const std::vector<u8> amiibo);
        const Kernel::SharedPtr<Kernel::Event>& GetNFCEvent() const;
        const std::vector<u8>& GetAmiiboBuffer() const;

    private:
        Kernel::SharedPtr<Kernel::Event> nfc_tag_load{};
        std::vector<u8> amiibo_buffer{};

    protected:
        std::shared_ptr<Module> module;
    };
};

void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Service::NFP
