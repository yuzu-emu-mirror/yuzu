// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <utility>
#include "common/logging/log.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/gdbstub/gdbstub.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/controller.h"
#include "core/hle/service/sm/sm.h"
#include "core/hw/hw.h"
#include "core/loader/loader.h"
#include "core/memory_setup.h"
#include "core/settings.h"
#include "video_core/video_core.h"

namespace Core {

/*static*/ System System::s_instance;

System::~System() = default;

System::ResultStatus System::RunLoop(bool tight_loop) {
    status = ResultStatus::Success;

    if (GDBStub::IsServerEnabled()) {
        GDBStub::HandlePacket();

        // If the loop is halted and we want to step, use a tiny (1) number of instructions to
        // execute. Otherwise, get out of the loop function.
        if (GDBStub::GetCpuHaltFlag()) {
            if (GDBStub::GetCpuStepFlag()) {
                GDBStub::SetCpuStepFlag(false);
                tight_loop = false;
            } else {
                return ResultStatus::Success;
            }
        }
    }

    cpu_cores[0]->RunLoop(tight_loop);

    return status;
}

System::ResultStatus System::SingleStep() {
    return RunLoop(false);
}

System::ResultStatus System::Load(EmuWindow* emu_window, const std::string& filepath) {
    app_loader = Loader::GetLoader(filepath);

    if (!app_loader) {
        NGLOG_CRITICAL(Core, "Failed to obtain loader for {}!", filepath);
        return ResultStatus::ErrorGetLoader;
    }
    std::pair<boost::optional<u32>, Loader::ResultStatus> system_mode =
        app_loader->LoadKernelSystemMode();

    if (system_mode.second != Loader::ResultStatus::Success) {
        NGLOG_CRITICAL(Core, "Failed to determine system mode (Error {})!",
                       static_cast<int>(system_mode.second));

        switch (system_mode.second) {
        case Loader::ResultStatus::ErrorEncrypted:
            return ResultStatus::ErrorLoader_ErrorEncrypted;
        case Loader::ResultStatus::ErrorInvalidFormat:
            return ResultStatus::ErrorLoader_ErrorInvalidFormat;
        case Loader::ResultStatus::ErrorUnsupportedArch:
            return ResultStatus::ErrorUnsupportedArch;
        default:
            return ResultStatus::ErrorSystemMode;
        }
    }

    ResultStatus init_result{Init(emu_window, system_mode.first.get())};
    if (init_result != ResultStatus::Success) {
        NGLOG_CRITICAL(Core, "Failed to initialize system (Error {})!",
                       static_cast<int>(init_result));
        System::Shutdown();
        return init_result;
    }

    const Loader::ResultStatus load_result{app_loader->Load(current_process)};
    if (Loader::ResultStatus::Success != load_result) {
        NGLOG_CRITICAL(Core, "Failed to load ROM (Error {})!", static_cast<int>(load_result));
        System::Shutdown();

        switch (load_result) {
        case Loader::ResultStatus::ErrorEncrypted:
            return ResultStatus::ErrorLoader_ErrorEncrypted;
        case Loader::ResultStatus::ErrorInvalidFormat:
            return ResultStatus::ErrorLoader_ErrorInvalidFormat;
        case Loader::ResultStatus::ErrorUnsupportedArch:
            return ResultStatus::ErrorUnsupportedArch;
        default:
            return ResultStatus::ErrorLoader;
        }
    }
    status = ResultStatus::Success;
    return status;
}

void System::PrepareReschedule() {
    cpu_cores[0]->PrepareReschedule();
}

PerfStats::Results System::GetAndResetPerfStats() {
    return perf_stats.GetAndResetStats(CoreTiming::GetGlobalTimeUs());
}

System::ResultStatus System::Init(EmuWindow* emu_window, u32 system_mode) {
    NGLOG_DEBUG(HW_Memory, "initialized OK");

    CoreTiming::Init();

    current_process = Kernel::Process::Create("main");

    for (auto& cpu_core : cpu_cores) {
        cpu_core = std::make_unique<Cpu>();
    }

    gpu_core = std::make_unique<Tegra::GPU>();

    telemetry_session = std::make_unique<Core::TelemetrySession>();

    service_manager = std::make_shared<Service::SM::ServiceManager>();

    HW::Init();
    Kernel::Init(system_mode);
    Service::Init(service_manager);
    GDBStub::Init();

    if (!VideoCore::Init(emu_window)) {
        return ResultStatus::ErrorVideoCore;
    }

    NGLOG_DEBUG(Core, "Initialized OK");

    // Reset counters and set time origin to current frame
    GetAndResetPerfStats();
    perf_stats.BeginSystemFrame();

    return ResultStatus::Success;
}

void System::Shutdown() {
    // Log last frame performance stats
    auto perf_results = GetAndResetPerfStats();
    Telemetry().AddField(Telemetry::FieldType::Performance, "Shutdown_EmulationSpeed",
                         perf_results.emulation_speed * 100.0);
    Telemetry().AddField(Telemetry::FieldType::Performance, "Shutdown_Framerate",
                         perf_results.game_fps);
    Telemetry().AddField(Telemetry::FieldType::Performance, "Shutdown_Frametime",
                         perf_results.frametime * 1000.0);

    // Shutdown emulation session
    VideoCore::Shutdown();
    GDBStub::Shutdown();
    Service::Shutdown();
    Kernel::Shutdown();
    HW::Shutdown();
    service_manager.reset();
    telemetry_session.reset();
    gpu_core.reset();

    for (auto& cpu_core : cpu_cores) {
        cpu_core.reset();
    }

    CoreTiming::Shutdown();

    app_loader.reset();

    NGLOG_DEBUG(Core, "Shutdown OK");
}

Service::SM::ServiceManager& System::ServiceManager() {
    return *service_manager;
}

const Service::SM::ServiceManager& System::ServiceManager() const {
    return *service_manager;
}

} // namespace Core
