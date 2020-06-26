// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/readable_event.h"
#include "core/hle/kernel/writable_event.h"
#include "core/hle/service/btdrv/btdrv.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::BtDrv {

class Bt final : public ServiceFramework<Bt> {
public:
    explicit Bt(Core::System& system) : ServiceFramework{"bt"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "LeClientReadCharacteristic"},
            {1, nullptr, "LeClientReadDescriptor"},
            {2, nullptr, "LeClientWriteCharacteristic"},
            {3, nullptr, "LeClientWriteDescriptor"},
            {4, nullptr, "LeClientRegisterNotification"},
            {5, nullptr, "LeClientDeregisterNotification"},
            {6, nullptr, "SetLeResponse"},
            {7, nullptr, "LeSendIndication"},
            {8, nullptr, "GetLeEventInfo"},
            {9, &Bt::RegisterBleEvent, "RegisterBleEvent"},
        };
        // clang-format on
        RegisterHandlers(functions);

        auto& kernel = system.Kernel();
        register_event = Kernel::WritableEvent::CreateEventPair(kernel, "BT:RegisterEvent");
    }

private:
    void RegisterBleEvent(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_BTM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(register_event.readable);
    }

    Kernel::EventPair register_event;
};

class BtDrv final : public ServiceFramework<BtDrv> {
public:
    explicit BtDrv() : ServiceFramework{"btdrv"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "InitializeBluetoothDriver"},
            {1, nullptr, "InitializeBluetooth"},
            {2, nullptr, "EnableBluetooth"},
            {3, nullptr, "DisableBluetooth"},
            {4, nullptr, "FinalizeBluetooth"},
            {5, nullptr, "GetAdapterProperties"},
            {6, nullptr, "GetAdapterProperty"},
            {7, nullptr, "SetAdapterProperty"},
            {8, nullptr, "StartInquiry"},
            {9, nullptr, "StopInquiry"},
            {10, nullptr, "CreateBond"},
            {11, nullptr, "RemoveBond"},
            {12, nullptr, "CancelBond"},
            {13, nullptr, "RespondToPinRequest"},
            {14, nullptr, "RespondToSspRequest"},
            {15, nullptr, "GetEventInfo"},
            {16, nullptr, "InitializeHid"},
            {17, nullptr, "OpenHidConnection"},
            {18, nullptr, "CloseHidConnection"},
            {19, nullptr, "WriteHidData"},
            {20, nullptr, "WriteHidData2"},
            {21, nullptr, "SetHidReport"},
            {22, nullptr, "GetHidReport"},
            {23, nullptr, "TriggerConnection"},
            {24, nullptr, "AddPairedDeviceInfo"},
            {25, nullptr, "GetPairedDeviceInfo"},
            {26, nullptr, "FinalizeHid"},
            {27, nullptr, "GetHidEventInfo"},
            {28, nullptr, "SetTsi"},
            {29, nullptr, "EnableBurstMode"},
            {30, nullptr, "SetZeroRetransmission"},
            {31, nullptr, "EnableMcMode"},
            {32, nullptr, "EnableLlrScan"},
            {33, nullptr, "DisableLlrScan"},
            {34, nullptr, "EnableRadio"},
            {35, nullptr, "SetVisibility"},
            {36, nullptr, "EnableTbfcScan"},
            {37, nullptr, "RegisterHidReportEvent"},
            {38, nullptr, "GetHidReportEventInfo"},
            {39, nullptr, "GetLatestPlr"},
            {40, nullptr, "GetPendingConnections"}, // 3.0.0+
            {41, nullptr, "GetChannelMap"}, // 3.0.0+
            {42, nullptr, "EnableTxPowerBoostSetting"}, // 3.0.0+
            {43, nullptr, "IsTxPowerBoostSettingEnabled"}, // 3.0.0+
            {44, nullptr, "EnableAfhSetting"}, // 3.0.0+
            {45, nullptr, "IsAfhSettingEnabled"},
            {46, nullptr, "InitializeBle"}, // 5.0.0+
            {47, nullptr, "EnableBle"}, // 5.0.0+
            {48, nullptr, "DisableBle"}, // 5.0.0+
            {49, nullptr, "FinalizeBle"}, // 5.0.0+
            {50, nullptr, "SetBleVisibility"}, // 5.0.0+
            {51, nullptr, "SetBleConnectionParameter"}, // 5.0.0+
            {52, nullptr, "SetBleDefaultConnectionParameter"}, // 5.0.0+
            {53, nullptr, "SetBleAdvertiseData"}, // 5.0.0+
            {54, nullptr, "SetBleAdvertiseParameter"}, // 5.0.0+
            {55, nullptr, "StartBleScan"}, // 5.0.0+
            {56, nullptr, "StopBleScan"}, // 5.0.0+
            {57, nullptr, "AddBleScanFilterCondition"}, // 5.0.0+
            {58, nullptr, "DeleteBleScanFilterCondition"}, // 5.0.0+
            {59, nullptr, "DeleteBleScanFilter"}, // 5.0.0+
            {60, nullptr, "ClearBleScanFilters"}, // 5.0.0+
            {61, nullptr, "EnableBleScanFilter"}, // 5.0.0+
            {62, nullptr, "RegisterGattClient"}, // 5.0.0+
            {63, nullptr, "UnregisterGattClient"}, // 5.0.0+
            {64, nullptr, "UnregisterAllGattClients"}, // 5.0.0+
            {65, nullptr, "ConnectGattServer"}, // 5.0.0+
            {66, nullptr, "CancelConnectGattServer"}, // 5.1.0+
            {67, nullptr, "DisconnectGattServer"}, // In 5.0.0 - 5.0.2 this was no. 66
            {68, nullptr, "GetGattAttribute"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 67
            {69, nullptr, "GetGattService"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 68
            {70, nullptr, "ConfigureAttMtu"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 69
            {71, nullptr, "RegisterGattServer"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 70
            {72, nullptr, "UnregisterGattServer"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 71
            {73, nullptr, "ConnectGattClient"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 72
            {74, nullptr, "DisconnectGattClient"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 73
            {75, nullptr, "AddGattService"}, // 5.0.0+
            {76, nullptr, "EnableGattService"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 74
            {77, nullptr, "AddGattCharacteristic"}, // 5.0.0+
            {78, nullptr, "AddGattDescriptor"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 76
            {79, nullptr, "GetBleManagedEventInfo"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 78
            {80, nullptr, "GetGattFirstCharacteristic"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 79
            {81, nullptr, "GetGattNextCharacteristic"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 80
            {82, nullptr, "GetGattFirstDescriptor"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 81
            {83, nullptr, "GetGattNextDescriptor"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 82
            {84, nullptr, "RegisterGattManagedDataPath"}, // 5.0.0+
            {85, nullptr, "UnregisterGattManagedDataPath"}, // 5.0.0+
            {86, nullptr, "RegisterGattHidDataPath"}, // 5.0.0+
            {87, nullptr, "UnregisterGattHidDataPath"}, // 5.0.0+
            {88, nullptr, "RegisterGattDataPath"}, // 5.0.0+
            {89, nullptr, "UnregisterGattDataPath"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 83
            {90, nullptr, "ReadGattCharacteristic"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 89
            {91, nullptr, "ReadGattDescriptor"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 90
            {92, nullptr, "WriteGattCharacteristic"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 91
            {93, nullptr, "WriteGattDescriptor"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 92
            {94, nullptr, "RegisterGattNotification"}, // 5.0.0+
            {95, nullptr, "UnregisterGattNotification"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 93
            {96, nullptr, "GetLeHidEventInfo"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 95
            {97, nullptr, "RegisterBleHidEvent"}, // 5.0.0+; In 5.0.0 - 5.0.2 this was no. 96
            {98, nullptr, "SetBleScanParameter"}, // 5.1.0+
            {99, nullptr, "MoveToSecondaryPiconet"}, // 10.0.0+
            {256, nullptr, "IsManufacturingMode"}, // 5.0.0+
            {257, nullptr, "EmulateBluetoothCrash"}, // 7.0.0+
            {258, nullptr, "GetBleChannelMap"}, // 9.0.0+
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<BtDrv>()->InstallAsService(sm);
    std::make_shared<Bt>(system)->InstallAsService(sm);
}

} // namespace Service::BtDrv
